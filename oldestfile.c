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

int oldestfile(struct thread_info *ti, short top, char *dir, char *oldfile,
			   time_t * oldtime)
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

	while(dp = readdir(dirp)) {
		if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		entries++;
		fullpath(dir, dp->d_name, filename);

		if(stat(filename, &stbuf) == -1)
			continue;

		if(S_ISDIR(stbuf.st_mode) && *ti->ti_subdirs == 't') {
			/* search subdirectory */
			fc += oldestfile(ti, FALSE, filename, oldfile, oldtime);
			continue;
		}

		if(!S_ISREG(stbuf.st_mode))
			continue;

		if(!mylogfile(dp->d_name, ti->ti_pcrecmp))
			continue;

		if(*oldtime == 0 || stbuf.st_mtim.tv_sec < *oldtime) {
			/* save the oldest log */
			*oldtime = stbuf.st_mtim.tv_sec;
			strlcpy(oldfile, filename, PATH_MAX);
		}

		fc++;
	}

	closedir(dirp);

	if(entries == 0 && !top)					/* directory is empty */
		remove(dir);

	return (fc);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
