/*
 * pcrefind.c
 * Program to find files matching Perl-compatible regular expressions.
 *
 * Copyright (c) 2021-2023 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include "sentinal.h"
#include "basename.h"

static struct option long_options[] = {
	{ "dirs", no_argument, NULL, 'd' },
	{ "files", no_argument, NULL, 'f' },
	{ "version", no_argument, NULL, 'V' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 }
};

static short list_dirs = FALSE;
static short list_files = FALSE;

static void help(char *);
short   pcrematch(struct thread_info *, char *);
uint32_t pcrefind(struct thread_info *, short, char *);

int main(int argc, char *argv[])
{
	char   *myname;
	int     c;
	int     index = 0;
	struct thread_info ti;							/* so we can use pcrecompile.c */

	myname = base(argv[0]);

	while(1) {
		c = getopt_long(argc, argv, "dfVh?", long_options, &index);

		if(c == -1)									/* end of options */
			break;

		switch (c) {

		case 'd':									/* list directorries */
			list_dirs = TRUE;
			break;

		case 'f':									/* list files */
			list_files = TRUE;
			break;

		case 'V':									/* print version */
			fprintf(stdout, "%s: version %s\n", myname, VERSION_STRING);
			exit(EXIT_SUCCESS);

		case 'h':									/* print usage */
		case '?':
			help(argv[0]);
			exit(EXIT_SUCCESS);
		}
	}

	if(optind >= argc) {
		help(argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(&ti, '\0', sizeof(struct thread_info));

	ti.ti_section = myname;
	ti.ti_pcrestr = argv[optind++];

	if(IS_NULL(ti.ti_pcrestr)) {
		help(argv[0]);
		exit(EXIT_FAILURE);
	}

	if(optind >= argc) {
		help(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* find everything if not given specifics */
	if(!(list_dirs || list_files))
		list_dirs = list_files = TRUE;

	/* we could just skip these tests in pcrefind() */
	ti.ti_subdirs = TRUE;
	ti.ti_symlinks = FALSE;

	if(pcrecompile(&ti) == FALSE)
		exit(EXIT_FAILURE);

	while(optind < argc)
		pcrefind(&ti, TRUE, argv[optind++]);

	exit(EXIT_SUCCESS);
}

uint32_t pcrefind(struct thread_info *ti, short top, char *dir)
{
	DIR    *dirp;
	char    filename[PATH_MAX];						/* full pathname */
	struct dirent *dp;
	struct stat stbuf;								/* file status */
	uint32_t entries = 0;							/* file entries */

	if(IS_NULL(dir))
		return (0);

	if(*dir == '/' && *(dir + 1) == '/')			/* find started at / */
		dir++;

	if(top) {										/* reset */
		if(stat(dir, &stbuf) == -1) {
			fprintf(stderr, "%s: cannot stat: %s\n", ti->ti_section, dir);
			return (0);
		}

		ti->ti_dev = stbuf.st_dev;					/* save mountpoint device */
	}

	/* test the directory itself */

	if(list_dirs)
		if(pcrematch(ti, dir)) {
			fprintf(stdout, "%s\n", dir);
			entries++;
		}

	if((dirp = opendir(dir)) == NULL) {
		if(errno == EACCES)
			perror(dir);

		return (entries);
	}

	/* test the files */

	while((dp = readdir(dirp))) {
		if(MY_DIR(dp->d_name) || MY_PARENT(dp->d_name))
			continue;

		snprintf(filename, PATH_MAX, "%s/%s", dir, dp->d_name);

		if(lstat(filename, &stbuf) == -1)
			continue;

		if(S_ISLNK(stbuf.st_mode) && !ti->ti_symlinks)
			continue;

		if(stat(filename, &stbuf) == -1)
			continue;

		if(S_ISDIR(stbuf.st_mode)) {
			if(ti->ti_subdirs && stbuf.st_dev == ti->ti_dev)
				entries += pcrefind(ti, FALSE, filename);

			continue;
		}

		if(list_files)
			if(pcrematch(ti, filename)) {
				fprintf(stdout, "%s\n", filename);
				entries++;
			}
	}

	closedir(dirp);
	return (entries);
}

static void help(char *prog)
{
	char   *p = base(prog);

	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "%s <pcre> <dir> [ <dir> ... ]\n", p);
	fprintf(stderr, "%s [ -d|--dirs ] <pcre> <dir> [ <dir> ... ]\n", p);
	fprintf(stderr, "%s [ -f|--files ] <pcre> <dir> [ <dir> ... ]\n\n", p);
	fprintf(stderr, "Print the program version, exit:\n");
	fprintf(stderr, "%s -V|--version\n\n", p);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
