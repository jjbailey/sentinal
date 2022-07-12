/*
 * lpcthread.c
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"

#define SCANRATE        (ONE_MINUTE * 4)			/* may need revisiting */

static void scandirectory(struct thread_info *, char *);

void   *lpcthread(void *arg)
{
	char    task[TASK_COMM_LEN];
	struct thread_info *ti = arg;

	/*
	 * this thread requires:
	 *  - ti_pcrestr = null
	 *  - ti_pcrecm = null
	 */

	if(NOT_NULL(ti->ti_pcrestr) || ti->ti_pcrecmp)
		return ((void *)0);

	pthread_setname_np(pthread_self(), threadname(ti->ti_section, _LPC_THR, task));
	fprintf(stderr, "%s: cache directory: %s\n", ti->ti_section, ti->ti_dirname);

	for(;;) {
		scandirectory(ti, ti->ti_dirname);
		sleep(SCANRATE);
	}

	/* notreached */
	return ((void *)0);
}

static void scandirectory(struct thread_info *ti, char *dir)
{
	DIR    *dirp;
	char    filename[PATH_MAX];
	struct dirent *dp;
	struct stat stbuf;

	if((dirp = opendir(dir)) == NULL)
		return;

	rewinddir(dirp);

	while(dp = readdir(dirp)) {
		if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		fullpath(dir, dp->d_name, filename);

		if(lstat(filename, &stbuf) == -1)
			continue;

		if(S_ISDIR(stbuf.st_mode) && ti->ti_subdirs)
			scandirectory(ti, filename);
	}

	closedir(dirp);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
