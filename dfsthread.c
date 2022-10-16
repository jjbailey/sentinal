/*
 * dfsthread.c
 * Filesystem monitor thread.  Remove files to create the desired free space.
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
#include <sqlite3.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"

#define	FLOORF(v)		floorf(v * 100.0) / 100.0
#define	PERCENT(x,y)	FLOORF(((long double)x / (long double)y) * 100.0)

#define	SCANRATE        (ONE_MINUTE)
#define	DRYSCAN         30							/* scanrate for dryrun */

#define	LOW_RES(target,avail)	(target && avail < target)

static char *sql_selectfiles = "SELECT db_dir, db_file\n \
	FROM  %s_dir, %s_file\n \
	WHERE db_dirid = db_id\n \
	ORDER BY db_time;";

static char mountdir[PATH_MAX];						/* mount point */

static void process_files(struct thread_info *, sqlite3 *);
static void resreport(struct thread_info *, short, long double, long double);

void   *dfsthread(void *arg)
{
	extern short dryrun;							/* dry run bool */
	extern sqlite3 *db;								/* db handle */
	long double pc_bfree = 0;						/* blocks free */
	long double pc_ffree = 0;						/* files free */
	short   lowres = FALSE;							/* low resources bool */
	struct statvfs svbuf;							/* filesystem status */
	struct thread_info *ti = arg;
	uint32_t nextid = 1;							/* db_id, db_dirid */

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

	/* monitor filesystem usage */
	/* force an initial report */

	lowres = !(LOW_RES(ti->ti_diskfree, pc_bfree) || LOW_RES(ti->ti_inofree, pc_ffree));

	for(;;) {
		if(statvfs(mountdir, &svbuf) == -1) {
			fprintf(stderr, "%s: statvfs failed: %s\n", ti->ti_section, mountdir);
			return ((void *)0);
		}

		if(ti->ti_diskfree)
			pc_bfree = PERCENT(svbuf.f_bavail, svbuf.f_blocks);

		if(ti->ti_inofree)
			pc_ffree = PERCENT(svbuf.f_favail, svbuf.f_files);

		if(LOW_RES(ti->ti_diskfree, pc_bfree) || LOW_RES(ti->ti_inofree, pc_ffree)) {
			/* low resources */

			if(!lowres)
				resreport(ti, lowres = TRUE, pc_bfree, pc_ffree);
		} else {
			/* desired state */

			if(lowres)
				resreport(ti, lowres = FALSE, pc_bfree, pc_ffree);
		}

		sleep(dryrun ? DRYSCAN : SCANRATE);

		if(!lowres)
			continue;

		/* low resources, find files in dirname */

		if(findfile(ti, TRUE, &nextid, ti->ti_dirname, db) == 0)
			continue;

		/* process directories emptied by previous run */

		if(ti->ti_rmdir)
			process_dirs(ti, db);

		/* process files matching criterion */

		process_files(ti, db);
	}

	/* notreached */
	return ((void *)0);
}

static void resreport(struct thread_info *ti, short lowres,
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

static void process_files(struct thread_info *ti, sqlite3 *db)
{
	char    filename[PATH_MAX];						/* full pathname */
	char    stmt[BUFSIZ];							/* statement buffer */
	char   *db_dir;
	char   *db_file;
	extern short dryrun;							/* dry run bool */
	long double pc_bfree = 0;						/* blocks free */
	long double pc_ffree = 0;						/* files free */
	sqlite3_stmt *pstmt;							/* prepared statement */
	struct statvfs svbuf;							/* filesystem status */
	uint32_t filecount;								/* matching files */

	/* count all files */

	if((filecount = count_files(ti, db)) < 1)
		return;

	/* process all files */

	snprintf(stmt, BUFSIZ, sql_selectfiles, ti->ti_task, ti->ti_task);
	sqlite3_prepare_v2(db, stmt, -1, &pstmt, NULL);

	for(;;) {
		if(ti->ti_retmin && filecount <= ti->ti_retmin)
			break;

		if(sqlite3_step(pstmt) != SQLITE_ROW)
			break;

		db_dir = (char *)sqlite3_column_text(pstmt, 0);
		db_file = (char *)sqlite3_column_text(pstmt, 1);

		/* assemble filename: ti_dirname + / + db_dir + / + db_file */

		if(IS_NULL(db_dir))
			snprintf(filename, PATH_MAX, "%s/%s", ti->ti_dirname, db_file);
		else
			snprintf(filename, PATH_MAX, "%s/%s/%s", ti->ti_dirname, db_dir, db_file);

		if(rmfile(ti, filename, "remove"))
			filecount--;

		if(statvfs(mountdir, &svbuf) == -1) {
			fprintf(stderr, "%s: statvfs failed: %s\n", ti->ti_section, mountdir);
			break;
		}

		if(ti->ti_diskfree)
			pc_bfree = PERCENT(svbuf.f_bavail, svbuf.f_blocks);

		if(ti->ti_inofree)
			pc_ffree = PERCENT(svbuf.f_favail, svbuf.f_files);

		if(!(LOW_RES(ti->ti_diskfree, pc_bfree) || LOW_RES(ti->ti_inofree, pc_ffree)))
			break;
	}

	sqlite3_finalize(pstmt);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
