/*
 * pcrefind.c
 * Program to find files matching Perl-compatible regular expressions.
 *
 * Copyright (c) 2021, 2022 jjb
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
#include <string.h>
#include "sentinal.h"
#include "basename.h"

short   pcrefind(struct thread_info *, short, char *);
short   pcrematch(struct thread_info *, char *);

int main(int argc, char *argv[])
{
	struct thread_info ti;
	char   *myname;
	int     i;
	extern int errno;

	myname = base(argv[0]);

	if(argc > 1 && strcmp(argv[1], "-V") == 0) {
		fprintf(stdout, "%s: version %s\n", myname, VERSION_STRING);
		exit(EXIT_SUCCESS);
	}

	if(argc < 3 || IS_NULL(argv[1])) {
		fprintf(stderr, "Usage: %s <pcre> <dir> [ <dir> ... ]\n", myname);
		exit(EXIT_FAILURE);
	}

	memset(&ti, '\0', sizeof(struct thread_info));

	ti.ti_section = myname;
	ti.ti_pcrestr = argv[1];

	/* we could just skip these tests in pcrefind() */
	ti.ti_subdirs = TRUE;
	ti.ti_symlinks = FALSE;

	if(pcrecompile(&ti) == FALSE)
		exit(EXIT_FAILURE);

	for(i = 2; i < argc; i++)
		pcrefind(&ti, TRUE, argv[i]);

	exit(EXIT_SUCCESS);
}

short pcrefind(struct thread_info *ti, short top, char *dir)
{
	DIR    *dirp;
	char    filename[PATH_MAX];
	long    matches;
	struct dirent *dp;
	struct stat stbuf;

	if(top) {										/* reset */
		if(stat(dir, &stbuf) == -1) {
			fprintf(stderr, "%s: cannot stat: %s\n", ti->ti_section, dir);
			return (0);
		}

		ti->ti_dev = stbuf.st_dev;					/* save mountpoint device */
		matches = 0;
	}

	/* test the directory itself */

	if(pcrematch(ti, dir)) {
		fprintf(stdout, "%s\n", dir);
		matches++;
	}

	if((dirp = opendir(dir)) == NULL) {
		if(errno == EACCES)
			perror(dir);

		return (matches);
	}

	/* test the files */

	while(dp = readdir(dirp)) {
		if(MY_DIR(dp->d_name) || MY_PARENT(dp->d_name))
			continue;

		snprintf(filename, PATH_MAX, "%s/%s", dir, dp->d_name);

		if(lstat(filename, &stbuf) == -1)
			continue;

		if(S_ISLNK(stbuf.st_mode) && !ti->ti_symlinks)
			continue;

		if(stat(filename, &stbuf) == -1)
			continue;

		if(S_ISDIR(stbuf.st_mode) && ti->ti_subdirs) {
			/* search subdirectory */

			if(stbuf.st_dev != ti->ti_dev)			/* don't cross filesystems */
				continue;

			matches += pcrefind(ti, FALSE, filename);
		}

		if(pcrematch(ti, filename)) {
			fprintf(stdout, "%s\n", filename);
			matches++;
		}
	}

	closedir(dirp);
	return (matches);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
