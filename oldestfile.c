/*
 * oldestfile.c
 * Check dir and possibly subdirs for the oldest file matching pcrestr (pcrecmp).
 * Returns the number of files in dir.
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
#include <string.h>
#include "sentinal.h"

int oldestfile(struct thread_info *ti, short top, char *dir, struct dir_info *di)
{
	DIR    *dirp;
	char    filename[PATH_MAX];
	int     entries = 0;
	int     fc = 0;
	struct dirent *dp;
	struct stat stbuf;

	if((dirp = opendir(dir)) == NULL)
		return (0);

	rewinddir(dirp);

	if(top) {										/* reset */
		*di->di_file = '\0';
		di->di_time = 0;
		di->di_bytes = 0;
	}

	while(dp = readdir(dirp)) {
		if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		fullpath(dir, dp->d_name, filename);

		if(stat(filename, &stbuf) == -1)
			continue;

		entries++;

		if(S_ISDIR(stbuf.st_mode) && ti->ti_subdirs) {
			/* search subdirectory */
			fc += oldestfile(ti, FALSE, filename, di);
			continue;
		}

		if(!S_ISREG(stbuf.st_mode))
			continue;

		if(!mylogfile(dp->d_name, ti->ti_pcrecmp))
			continue;

		di->di_bytes += stbuf.st_size;				/* total size of files found */

		if(di->di_time == 0 || stbuf.st_mtim.tv_sec < di->di_time) {
			/* save the oldest log */
			strlcpy(di->di_file, filename, PATH_MAX);
			di->di_time = stbuf.st_mtim.tv_sec;
		}

		fc++;
	}

	closedir(dirp);

	if(entries == 0 && ti->ti_expire && !top) {
		/* expire thread, directory is empty */

		if(!ti->ti_terse)
			fprintf(stderr, "%s: rmdir %s\n", ti->ti_section, dir);

		remove(dir);
	}

	return (fc);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
