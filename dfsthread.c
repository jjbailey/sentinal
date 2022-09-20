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
static void resourcestat(struct thread_info *, short, long double, long double);

void   *dfsthread(void *arg)
{
	char    mountdir[PATH_MAX];
	extern short dryrun;
	int     interval;
	long double pc_bfree = 0;
	long double pc_ffree = 0;
	short   lowres = FALSE;
	short   rptstatus = FALSE;
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

	pthread_setname_np(pthread_self(), threadname(ti, _DFS_THR));

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
		} else {
			/* supported */
			pc_bfree = PERCENT(svbuf.f_bavail, svbuf.f_blocks);
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
			/* supported */
			pc_ffree = PERCENT(svbuf.f_favail, svbuf.f_files);
		}
	}

	if(ti->ti_diskfree == 0 && ti->ti_inofree == 0) {
		/* nothing to do here */
		return ((void *)0);
	}

	/* initial report */

	resourcestat(ti, FALSE, pc_bfree, pc_ffree);

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

		if(LOW_RES(ti->ti_diskfree, pc_bfree) || LOW_RES(ti->ti_inofree, pc_ffree)) {
			/* low resources */

			if(!lowres) {
				lowres = TRUE;
				rptstatus = TRUE;					/* lowres report */
			}
		} else {
			/* desired state */

			if(lowres) {
				lowres = FALSE;
				rptstatus = TRUE;					/* recovery report */
			}
		}

		if(rptstatus) {
			resourcestat(ti, lowres, pc_bfree, pc_ffree);
			rptstatus = FALSE;						/* mute until state change */
		}

		if(!lowres)
			continue;

		/* low resources, remove oldest file */

		dinfo.di_matches = findfile(ti, TRUE, ti->ti_dirname, &dinfo);

		if(dinfo.di_matches < 1 || (ti->ti_retmin && dinfo.di_matches <= ti->ti_retmin)) {
			/* no matches, or matches below the retention count */
			/* recompute the monitor rate */
			interval = itimer((int)pc_bfree, (int)pc_ffree, SCANRATE);
		} else {
			/* match */
			rmfile(ti, dinfo.di_file, "remove");
			interval = 0;							/* 1 sec is too slow for inodes */
			interval = 30;
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

static void resourcestat(struct thread_info *ti, short lowres,
						 long double blk, long double ino)
{
	if(lowres == FALSE) {							/* initial/recovery report */
		if(ti->ti_diskfree)
			fprintf(stderr, "%s: %s: %.2Lf%% blocks free\n",
					ti->ti_section, ti->ti_dirname, blk);

		if(ti->ti_inofree)
			fprintf(stderr, "%s: %s: %.2Lf%% inodes free\n",
					ti->ti_section, ti->ti_dirname, ino);
	} else {										/* low resource report */
		if(LOW_RES(ti->ti_diskfree, blk))
			fprintf(stderr, "%s: low free blocks %s: %.2Lf%% < %.2Lf%%\n",
					ti->ti_section, ti->ti_dirname, blk, ti->ti_diskfree);

		if(LOW_RES(ti->ti_inofree, ino))
			fprintf(stderr, "%s: low free inodes %s: %.2Lf%% < %.2Lf%%\n",
					ti->ti_section, ti->ti_dirname, ino, ti->ti_inofree);
	}
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
