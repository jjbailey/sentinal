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
#include <string.h>
#include <unistd.h>
#include "sentinal.h"

#define	FLOORF(v)		floorf(v * 100.0) / 100.0
#define	PERCENT(x,y)	FLOORF(((long double)x / (long double)y) * 100.0)
#define	SCANRATE		10							/* minimum (fastest) monitor rate */

#define	MINIMUM(a,b)	(a < b ? a : b)
#define	MAXIMUM(a,b)	(a > b ? a : b)

#define	LOW_RES(target,avail)	(target && avail < target)

static int itimer(int, int, int);

void   *dfsthread(void *arg)
{
	char    mountdir[PATH_MAX];
	char    task[TASK_COMM_LEN];
	extern short dryrun;
	int     interval;
	long    matches;
	long double pc_bfree;
	long double pc_ffree;
	short   lowres = FALSE;
	short   rptstatus = TRUE;
	struct dir_info dinfo;
	struct statvfs svbuf;
	struct thread_info *ti = arg;

	/*
	 * this thread requires:
	 *  - ti_pcrecmp
	 *  - at least one of:
	 *    - ti_diskfree
	 *    - ti_inofree
	 */

	pthread_setname_np(pthread_self(), threadname(ti->ti_section, _DFS_THR, task));

	findmnt(ti->ti_dirname, mountdir);				/* actual mountpoint */
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
		}
	}

	if(ti->ti_inofree) {
		fprintf(stderr, "%s: monitor inode: %s for %.2Lf%% free\n",
				ti->ti_section, mountdir, ti->ti_inofree);

		if(svbuf.f_files == 0) {
			/* test for zero inodes reported (e.g. AWS EFS) */
			fprintf(stderr, "%s: no inode support: %s\n", ti->ti_section, mountdir);
			ti->ti_inofree = 0;
		}
	}

	if(ti->ti_diskfree == 0 && ti->ti_inofree == 0) {
		/* nothing to do here */
		return ((void *)0);
	}

	/* monitor filesystem based on available space */

	interval = dryrun ? 1 : SCANRATE >> 2;			/* faster initial start */

	for(;;) {
		sleep(interval);							/* filesystem monitor rate */

		if(statvfs(mountdir, &svbuf) == -1) {
			fprintf(stderr, "%s: statvfs failed: %s\n", ti->ti_section, mountdir);
			break;
		}

		if(ti->ti_diskfree)
			pc_bfree = PERCENT(svbuf.f_bavail, svbuf.f_blocks);

		if(ti->ti_inofree)
			pc_ffree = PERCENT(svbuf.f_favail, svbuf.f_files);

		if(!LOW_RES(ti->ti_diskfree, pc_bfree) && !LOW_RES(ti->ti_inofree, pc_ffree)) {
			/* desired state */

			if(lowres) {							/* toggle status and reporting */
				lowres = FALSE;
				rptstatus = TRUE;
			}

			if(rptstatus) {
				if(ti->ti_diskfree)
					fprintf(stderr, "%s: %s: %.2Lf%% blocks free\n",
							ti->ti_section, ti->ti_dirname, pc_bfree);

				if(ti->ti_inofree)
					fprintf(stderr, "%s: %s: %.2Lf%% inodes free\n",
							ti->ti_section, ti->ti_dirname, pc_ffree);

				rptstatus = FALSE;					/* mute status alert */

				/* recompute the monitor rate */
				interval = itimer((int)pc_bfree, (int)pc_ffree, SCANRATE);
			}

			continue;
		}

		/* low resources */

		if(!lowres) {								/* toggle status and reporting */
			lowres = TRUE;
			rptstatus = TRUE;
		}

		if(rptstatus) {
			if(LOW_RES(ti->ti_diskfree, pc_bfree))
				fprintf(stderr, "%s: low free blocks %s: %.2Lf%% < %.2Lf%%\n",
						ti->ti_section, ti->ti_dirname, pc_bfree, ti->ti_diskfree);

			if(LOW_RES(ti->ti_inofree, pc_ffree))
				fprintf(stderr, "%s: low free inodes %s: %.2Lf%% < %.2Lf%%\n",
						ti->ti_section, ti->ti_dirname, pc_ffree, ti->ti_inofree);

			rptstatus = FALSE;						/* mute status alert */
		}

		/* low space, remove oldest file */

		matches = findfile(ti, TRUE, ti->ti_dirname, &dinfo);

		if(matches < 1 || (ti->ti_retmin && matches <= ti->ti_retmin)) {
			/* no matches, or matches below the retention count */
			/* recompute the monitor rate */
			interval = itimer((int)pc_bfree, (int)pc_ffree, SCANRATE);
		} else {
			/* match */
			rmfile(ti, dinfo.di_file, "remove");
			interval = 0;							/* 1 sec is too slow for inodes */
		}
	}

	/* notreached */
	return ((void *)0);
}

static int itimer(int a, int b, int c)
{
	/*
	 * return the lesser of a, b, but no less than c
	 * neither a nor b can be 0
	 */

	if(a && !b)
		b = a;

	else if(!a && b)
		a = b;

	return (MAXIMUM(MINIMUM(a, b), c));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
