/*
 * dfsthread.c
 * Filesystem monitor thread.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"

#define	FLOORF(v)		floorf(v * 100.0) / 100.0
#define	PERCENT(x,y)	FLOORF(((long double)x / (long double)y) * 100.0)
#define	SCANRATE	15

void   *dfsthread(void *arg)
{
	char    mountdir[PATH_MAX];
	char    oldfile[PATH_MAX];
	char    task[TASK_COMM_LEN];
	int     fc;
	int     interval = SCANRATE;
	int     rptlowres = TRUE;
	int     rptstatus = FALSE;
	int     skip = FALSE;
	long double pc_bfree = 0.0;
	long double pc_ffree = 0.0;
	struct statvfs svbuf;
	struct thread_info *ti = arg;
	time_t  curtime;
	time_t  oldtime;

	if(threadcheck(ti, "dfs") == FALSE)			/* should not be here */
		return ((void *)0);

	pthread_detach(pthread_self());
	pthread_setname_np(pthread_self(), threadname(ti->ti_section, "dfs", task));

	findmnt(ti->ti_dirname, mountdir);			/* actual mountpoint */
	memset(&svbuf, '\0', sizeof(svbuf));

	if(statvfs(mountdir, &svbuf) == -1) {
		fprintf(stderr, "%s: cannot stat: %s\n", ti->ti_section, mountdir);
		return ((void *)0);
	}

	if(ti->ti_diskfree) {
		fprintf(stderr, "%s: monitor disk: %s for %.2Lf%% free\n",
				ti->ti_section, mountdir, ti->ti_diskfree);

		if(svbuf.f_blocks == 0) {
			/* test for zero blocks reported */

			fprintf(stderr, "%s: no block support: %s\n", ti->ti_section, mountdir);
			ti->ti_diskfree = 0;
		} else {
			/* initial report for unprivileged users */

			pc_bfree = PERCENT(svbuf.f_bavail, svbuf.f_blocks);

			fprintf(stderr, "%s: %s: %.2Lf%% blocks free\n",
					ti->ti_section, ti->ti_dirname, pc_bfree);
		}
	}

	if(ti->ti_inofree) {
		fprintf(stderr, "%s: monitor inode: %s for %.2Lf%% free\n",
				ti->ti_section, mountdir, ti->ti_inofree);

		if(svbuf.f_files == 0) {
			/* test for zero inodes reported (e.g. AWS EFS) */

			fprintf(stderr, "%s: no inode support: %s\n", ti->ti_section, mountdir);
			ti->ti_inofree = 0;
		} else {
			/* initial report for unprivileged users */

			pc_ffree = PERCENT(svbuf.f_favail, svbuf.f_files);

			fprintf(stderr, "%s: %s: %.2Lf%% inodes free\n",
					ti->ti_section, ti->ti_dirname, pc_ffree);
		}
	}

	if(ti->ti_diskfree == 0 && ti->ti_inofree == 0) {
		/* nothing to do here */
		return ((void *)0);
	}

	for(;;) {
		sleep(interval);						/* filesystem monitor rate */

		if(skip == FALSE) {
			/* performance: halve the statvfs and math work */

			if(statvfs(mountdir, &svbuf) == -1) {
				fprintf(stderr, "%s: statvfs failed: %s\n", ti->ti_section, mountdir);
				break;
			}

			if(ti->ti_diskfree)
				pc_bfree = PERCENT(svbuf.f_bavail, svbuf.f_blocks);

			if(ti->ti_inofree)
				pc_ffree = PERCENT(svbuf.f_favail, svbuf.f_files);
		}

		skip = !skip;

		if(pc_bfree >= ti->ti_diskfree && pc_ffree >= ti->ti_inofree) {
			/* desired state */

			if(rptstatus == TRUE) {
				if(ti->ti_diskfree)
					fprintf(stderr, "%s: %s: %.2Lf%% blocks free\n",
							ti->ti_section, ti->ti_dirname, pc_bfree);

				if(ti->ti_inofree)
					fprintf(stderr, "%s: %s: %.2Lf%% inodes free\n",
							ti->ti_section, ti->ti_dirname, pc_ffree);

				rptstatus = FALSE;				/* quiet */
				rptlowres = TRUE;				/* reset alert */
			}

			if(interval != SCANRATE)
				interval = SCANRATE;			/* return to normal */

			continue;
		} else {
			/* low resources */

			if(rptlowres == TRUE) {
				if(ti->ti_diskfree && pc_bfree < ti->ti_diskfree)
					fprintf(stderr, "%s: low free blocks %s: %.2Lf%% < %.2Lf%%\n",
							ti->ti_section, ti->ti_dirname, pc_bfree, ti->ti_diskfree);

				if(ti->ti_inofree && pc_ffree < ti->ti_inofree)
					fprintf(stderr, "%s: low free inodes %s: %.2Lf%% < %.2Lf%%\n",
							ti->ti_section, ti->ti_dirname, pc_ffree, ti->ti_inofree);

				rptlowres = FALSE;				/* quiet */
			}

			interval = 0;						/* 1 sec is too slow for inodes */
		}

		/* low space, remove oldest file */
		*oldfile = oldtime = 0;

		/* full path to oldest file and its time */
		fc = oldestfile(ti, TRUE, ti->ti_dirname, oldfile, &oldtime);

		if(!fc || (ti->ti_retmin && fc <= ti->ti_retmin)) {
			/* desired state */
			interval = SCANRATE;				/* return to normal */
			continue;
		}

		if(time(&curtime) - oldtime < ONE_MINUTE) {
			/* wait for another thread to remove a file older than this one */
			continue;
		}

		fprintf(stderr, "%s: remove %s\n", ti->ti_section, base(oldfile));
		remove(oldfile);
		rptstatus = TRUE;
	}

	return ((void *)0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
