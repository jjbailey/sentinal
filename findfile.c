/*
 * findfile.c
 * Check dir and possibly subdirs for files matching pcrestr (pcrecmp).
 * Return the number of files matching pcrestr.
 *
 * Expiration policy included here to improve performance: when we find expired
 * files, don't wait for the calling function to remove them -- remove them now.
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
#include <unistd.h>
#include "sentinal.h"

long findfile(struct thread_info *ti, short top, char *dir, struct dir_info *di)
{
	DIR    *dirp;
	char    filename[PATH_MAX];
	long    direntries = 0;							/* directory entries */
	short   expbysize;								/* consider expire size */
	short   expbytime;								/* consider expire time */
	short   expthrflag;								/* is expire thread */
	struct dirent *dp;
	struct stat stbuf;
	time_t  curtime;

	if((dirp = opendir(dir)) == NULL)
		return (0);

	if(top) {										/* reset */
		if(stat(dir, &stbuf) == -1) {
			fprintf(stderr, "%s: cannot stat: %s\n", ti->ti_section, dir);
			return (0);
		}

		memset(di, '\0', sizeof(struct dir_info));
		ti->ti_dev = stbuf.st_dev;					/* save mountpoint device */
	}

	expthrflag = strcmp(strrchr(ti->ti_task, '\0') - 3, _EXP_THR) == 0;

	while(dp = readdir(dirp)) {
		if(MY_DIR(dp->d_name) || MY_PARENT(dp->d_name))
			continue;

		fullpath(dir, dp->d_name, filename);
		direntries++;

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

			if(findfile(ti, FALSE, filename, di) == EOF) {
				/* empty dir removed */

				if(direntries)
					direntries--;

				continue;
			}
		}

		if(!S_ISREG(stbuf.st_mode) || !mylogfile(ti, dp->d_name))
			continue;

		/* match */

		if(expthrflag) {
			/*
			 * dfsthread and expthread can race to remove the same file
			 * run in expthread only
			 */

			if(ti->ti_expire && !ti->ti_retmin) {
				/*
				 * ok to expire now
				 * if expiresiz is set, use it, else true
				 */

				time(&curtime);

				expbysize = !ti->ti_expiresiz || stbuf.st_size > ti->ti_expiresiz;
				expbytime = stbuf.st_mtim.tv_sec + ti->ti_expire < curtime;

				if(expbysize && expbytime) {
					if(rmfile(ti, filename, "expire"))
						if(direntries)
							direntries--;

					continue;						/* continue searching */
				}
			}
		}

		/* save the oldest file */

		if(di->di_time == 0 || stbuf.st_mtim.tv_sec < di->di_time) {
			strlcpy(di->di_file, filename, PATH_MAX);
			di->di_time = stbuf.st_mtim.tv_sec;
			di->di_size = stbuf.st_size;
		}

		if(ti->ti_dirlimit)							/* request total size of files found */
			di->di_bytes += stbuf.st_size;

		di->di_matches++;
	}

	closedir(dirp);

	if(!top)
		if(direntries == 0 && ti->ti_rmdir) {		/* ok to remove empty dir */
			if(!expthrflag)							/* avoid dfs/exp race */
				usleep((useconds_t) 100000);

			if(rmfile(ti, dir, "rmdir"))
				return (EOF);
		}

	return (di->di_matches);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
