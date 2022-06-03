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
#include <time.h>
#include "sentinal.h"

int oldestfile(struct thread_info *ti, short top, char *dir, struct dir_info *di)
{
	DIR    *dirp;
	char    filename[PATH_MAX];
	int     matches;								/* matching files */
	int     subfound;								/* matching files in subdir */
	short   anyfound;								/* existing files flag */
	short   wantoldest = FALSE;						/* conditional search */
	struct dirent *dp;
	struct stat stbuf;
	time_t  curtime;

	if((dirp = opendir(dir)) == NULL)
		return (0);

	rewinddir(dirp);

	if(top) {										/* reset */
		*di->di_file = '\0';
		di->di_time = 0;
		di->di_bytes = 0;
		anyfound = matches = FALSE;
	}

	if(ti->ti_dirlimit || ti->ti_diskfree || ti->ti_inofree ||
	   ti->ti_retmin || ti->ti_retmax) {
		/*
		 * if one of these conditions is set, search for the oldest file,
		 * else any regex-matched file qualifies
		 */
		wantoldest = TRUE;
	}

	time(&curtime);

	while(dp = readdir(dirp)) {
		if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		fullpath(dir, dp->d_name, filename);

		if(stat(filename, &stbuf) == -1)
			continue;

		if(S_ISDIR(stbuf.st_mode) && ti->ti_subdirs) {
			/* search subdirectory */

			subfound = oldestfile(ti, FALSE, filename, di);

			if(subfound == EOF)						/* empty dir removed */
				continue;

			anyfound = TRUE;

			if(wantoldest == TRUE)					/* continue searching */
				continue;
		}

		anyfound = TRUE;

		if(!S_ISREG(stbuf.st_mode))
			continue;

		if(!mylogfile(dp->d_name, ti->ti_pcrecmp))
			continue;

		if(ti->ti_loglimit && stbuf.st_size < ti->ti_loglimit) {
			/* match, but below the specified expire size */
			continue;
		}

		/* match */

		if(wantoldest == FALSE) {
			/* if expired, remove now and continue searching */

			if(stbuf.st_mtim.tv_sec + ti->ti_expire < curtime) {
				if(!ti->ti_terse)
					fprintf(stderr, "%s: expired %s\n", ti->ti_section, filename);

				remove(filename);
			}

			continue;
		}

		if(ti->ti_dirlimit)							/* request total size of files found */
			di->di_bytes += stbuf.st_size;

		if(di->di_time == 0 || stbuf.st_mtim.tv_sec < di->di_time) {
			/* save the oldest file */
			strlcpy(di->di_file, filename, PATH_MAX);
			di->di_time = stbuf.st_mtim.tv_sec;
		}

		matches++;
	}

	closedir(dirp);

	if(anyfound == FALSE && ti->ti_expire && !top) {
		/* expire thread, directory is empty */

		if(!ti->ti_terse)
			fprintf(stderr, "%s: rmdir %s\n", ti->ti_section, dir);

		remove(dir);
		return (EOF);
	}

	return (matches);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
