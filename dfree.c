/*
 * dfree.c
 * grok wrote this based on the original dfree.sh script.
 *
 * Copyright (c) 2024-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <mntent.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH    256
#define MAX_LINE    1024
#define MAX_MOUNTS  128

typedef struct {
	char    device[MAX_PATH];
	char    mount_point[MAX_PATH];
	unsigned long long total_bytes;
	unsigned long long used_bytes;
	unsigned long long avail_bytes;
	unsigned long long f_bavail_bytes;
} Filesystem;

const char *allowed_types[] =
	{ "ext4", "xfs", "ext2", "ext3", "nfs", "cifs", "ntfs3", "ntfs", NULL };

int is_allowed_type(const char *type)
{
	for(const char **p = allowed_types; *p; p++) {
		if(strstr(type, *p))
			return 1;
	}
	return 0;
}

// Converts bytes into human readable format into buf (thread-safe)
void human_readable(char *buf, size_t buflen, unsigned long long bytes)
{
	const char *units[] = { "B", "K", "M", "G", "T", "P", "E" };
	int     unit = 0;
	double  val = (double)bytes;
	while(val >= 1024 && unit < 6) {
		val /= 1024;
		unit++;
	}
	snprintf(buf, buflen, "%.1f%s", val, units[unit]);
}

// Calculates '% used' in the way that `df` calculates it
// Used = total - f_bfree
// Avail = f_bavail (as per statvfs)
// % = used / (used + avail)
void calc_percentage(char *buf, size_t buflen,
					 unsigned long long used, unsigned long long avail)
{
	if((used + avail) == 0) {
		snprintf(buf, buflen, "0.00%%");
	} else {
		double  percent = (double)used * 100.0 / (used + avail);
		snprintf(buf, buflen, "%.2f%%", percent);
	}
}

// Check if a path is a directory
int is_directory(const char *path)
{
	struct stat st;
	if(stat(path, &st) != 0)
		return 0;
	return S_ISDIR(st.st_mode);
}

// Print usage help
void show_usage(const char *progname)
{
	printf("Usage: %s [mount_point1] [mount_point2] ...\n", progname);
	printf("       %s                 # Show all mounted filesystems\n", progname);
	printf("\nExamples:\n");
	printf("  %s                      # Show all filesystems\n", progname);
	printf("  %s /home                # Show only /home filesystem\n", progname);
	printf("  %s / /home /var         # Show root, home, and var filesystems\n",
		   progname);
}

// Get size stats for a mount
int get_fs_info(const char *mount_point,
				unsigned long long *total,
				unsigned long long *used,
				unsigned long long *avail, unsigned long long *f_bavail_raw)
{
	struct statvfs fs;
	if(statvfs(mount_point, &fs) != 0) {
		fprintf(stderr, "Error accessing filesystem %s: %s\n", mount_point,
				strerror(errno));
		return 1;
	}
	unsigned long long block_size = fs.f_frsize ? fs.f_frsize : fs.f_bsize;

	*total = fs.f_blocks * block_size;
	*f_bavail_raw = fs.f_bavail * block_size;
	*avail = fs.f_bavail * block_size;
	*used = (fs.f_blocks - fs.f_bfree) * block_size;

	return 0;
}

// Get mount for each user-specified path
int get_specific_mounts(int argc, char *argv[], Filesystem *fs_list, int *fs_count)
{
	int     i;
	for(i = 1; i < argc; i++) {
		char    resolved_path[PATH_MAX];
		if(realpath(argv[i], resolved_path) == NULL) {
			fprintf(stderr, "Warning: Cannot resolve path '%s': %s\n", argv[i],
					strerror(errno));
			continue;
		}
		if(!is_directory(resolved_path)) {
			fprintf(stderr, "Warning: Path '%s' does not exist or is not accessible\n",
					argv[i]);
			continue;
		}
		// Look up mount point
		FILE   *mnt = setmntent("/proc/mounts", "r");
		if(!mnt) {
			perror("Error opening /proc/mounts");
			return 1;
		}
		struct mntent *ent;
		char    best_match_mount[PATH_MAX] = "";
		char    best_match_device[PATH_MAX] = "";
		size_t  best_match_len = 0;
		int     found = 0;
		while((ent = getmntent(mnt))) {
			size_t  mlen = strlen(ent->mnt_dir);
			if((strcmp(ent->mnt_dir, "/") == 0 && resolved_path[0] == '/') ||
			   (strncmp(resolved_path, ent->mnt_dir, mlen) == 0 &&
				(resolved_path[mlen] == '\0' || resolved_path[mlen] == '/'))) {
				if(mlen > best_match_len) {
					strncpy(best_match_mount, ent->mnt_dir, PATH_MAX - 1);
					best_match_mount[PATH_MAX - 1] = '\0';
					strncpy(best_match_device, ent->mnt_fsname, PATH_MAX - 1);
					best_match_device[PATH_MAX - 1] = '\0';
					best_match_len = mlen;
					found = 1;
				}
			}
		}
		endmntent(mnt);

		if(found) {
			if(*fs_count < MAX_MOUNTS) {
				strncpy(fs_list[*fs_count].device, best_match_device, MAX_PATH - 1);
				fs_list[*fs_count].device[MAX_PATH - 1] = '\0';
				strncpy(fs_list[*fs_count].mount_point, best_match_mount, MAX_PATH - 1);
				fs_list[*fs_count].mount_point[MAX_PATH - 1] = '\0';
				(*fs_count)++;
			}
		} else {
			fprintf(stderr, "Warning: '%s' is not on a mounted filesystem\n", argv[i]);
		}
	}
	return 0;
}

// Get all relevant mounts (filtering types and "undesirables")
int get_all_mounts(Filesystem *fs_list, int *fs_count)
{
	FILE   *mnt = setmntent("/proc/mounts", "r");
	if(!mnt) {
		perror("Error opening /proc/mounts");
		return 1;
	}
	struct mntent *ent;
	while((ent = getmntent(mnt))) {
		if(!is_allowed_type(ent->mnt_type)) {
			continue;
		}
		// Exclude loop devices and snap mounts
		if(strncmp(ent->mnt_fsname, "/dev/loop", 9) == 0)
			continue;
		if(strncmp(ent->mnt_dir, "/snap", 5) == 0 ||
		   strncmp(ent->mnt_dir, "/var/snap", 9) == 0)
			continue;

		if(*fs_count < MAX_MOUNTS) {
			strncpy(fs_list[*fs_count].device, ent->mnt_fsname, MAX_PATH - 1);
			fs_list[*fs_count].device[MAX_PATH - 1] = '\0';
			strncpy(fs_list[*fs_count].mount_point, ent->mnt_dir, MAX_PATH - 1);
			fs_list[*fs_count].mount_point[MAX_PATH - 1] = '\0';
			(*fs_count)++;
		}
	}
	endmntent(mnt);
	return 0;
}

int main(int argc, char *argv[])
{
	Filesystem fs_list[MAX_MOUNTS];
	int     fs_count = 0;
	int     i;

	// Help
	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ||
		   strcmp(argv[i], "-?") == 0) {
			show_usage(argv[0]);
			return 0;
		}
	}
	// Gather filesystem info
	if(argc == 1) {
		if(get_all_mounts(fs_list, &fs_count) != 0)
			return 1;
	} else {
		if(get_specific_mounts(argc, argv, fs_list, &fs_count) != 0)
			return 1;
	}
	if(fs_count == 0) {
		printf("No filesystems found to display.\n");
		return 0;
	}
	// Calculate max width of device column
	int     max_fs_width = 10;
	for(i = 0; i < fs_count; i++) {
		int     len = strlen(fs_list[i].device);
		if(len > max_fs_width)
			max_fs_width = len;
	}
	max_fs_width++;

	// Print header
	printf("%-*s %10s %10s %10s %7s %s\n",
		   max_fs_width, "Filesystem", "Size", "Used", "Avail", "Use%", "Mounted on");

	// Print data
	for(i = 0; i < fs_count; i++) {
		if(!is_directory(fs_list[i].mount_point))
			continue;

		unsigned long long f_bavail_bytes = 0;
		if(get_fs_info(fs_list[i].mount_point, &fs_list[i].total_bytes,
					   &fs_list[i].used_bytes, &fs_list[i].avail_bytes,
					   &f_bavail_bytes) != 0)
			continue;
		fs_list[i].f_bavail_bytes = f_bavail_bytes;

		char    total_human[32], used_human[32], avail_human[32], use_percent[16];
		human_readable(total_human, sizeof(total_human), fs_list[i].total_bytes);
		human_readable(used_human, sizeof(used_human), fs_list[i].used_bytes);
		human_readable(avail_human, sizeof(avail_human), fs_list[i].avail_bytes);

		// Use correct percentage: used/(used+avail) -- note avail is f_bavail
		calc_percentage(use_percent, sizeof(use_percent),
						fs_list[i].used_bytes, fs_list[i].f_bavail_bytes);

		printf("%-*s %10s %10s %10s %7s %s\n",
			   max_fs_width, fs_list[i].device, total_human, used_human, avail_human,
			   use_percent, fs_list[i].mount_point);
	}
	return 0;
}

// vim: set tabstop=4 shiftwidth=4 expandtab:
