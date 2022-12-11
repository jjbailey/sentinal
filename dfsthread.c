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
#include <fcntl.h>
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

#define	LOW_RES(target,avail)	(target > 0 && avail < target)

/* subtract from avail for extra space, reduce flapping */
#define	PADDING			(float)0.195

/* externals */
extern pthread_mutex_t dfslock;						/* thread lock */

/* test LIMIT to see if it improves performance on big directory trees */
static char *sql_selectfiles = "SELECT db_dir, db_file\n \
	FROM  %s_dir, %s_file\n \
	WHERE db_dirid = db_id\n \
	ORDER BY db_time\n \
	LIMIT 1000000;";

static short getvfsstats(struct thread_info *, long double *, long double *);
static void process_files(struct thread_info *, sqlite3 *);
static void resreport(struct thread_info *, short, long double, long double);

void   *dfsthread(void *arg)
{
	extern short dryrun;							/* dry run bool */
	extern sqlite3 *db;								/* db handle */
	long double pc_bfree = 0;						/* blocks free */
	long double pc_ffree = 0;						/* files free */
	short   lowres = FALSE;							/* low resources bool */
	short   runreport = TRUE;						/* for resreport */
	struct statvfs svbuf;							/* filesystem status */
	struct thread_info *ti = arg;					/* thread settings */
	uint32_t nextid = 1;							/* db_id, db_dirid */

	/*
	 * this thread requires:
	 *  - ti_pcrecmp
	 *  - at least one of:
	 *    - ti_diskfree
	 *    - ti_inofree
	 *
	 * optional:
	 *  - ti_retmin
	 */

	pthread_setname_np(pthread_self(), threadname(ti, _DFS_THR));

	findmnt(ti->ti_dirname, ti->ti_mountdir);		/* actual mountpoint */
	memset(&svbuf, '\0', sizeof(svbuf));

	if(statvfs(ti->ti_mountdir, &svbuf) == -1) {
		fprintf(stderr, "%s: cannot stat: %s\n", ti->ti_section, ti->ti_mountdir);
		return ((void *)0);
	}

	if(ti->ti_diskfree > 0) {
		fprintf(stderr, "%s: monitor disk: %s for %.2Lf%% free\n",
				ti->ti_section, ti->ti_mountdir, ti->ti_diskfree);

		if(svbuf.f_blocks == 0) {
			/* test for zero blocks reported */
			fprintf(stderr, "%s: no block support: %s\n", ti->ti_section,
					ti->ti_mountdir);
			ti->ti_diskfree = 0;
		}
	}

	if(ti->ti_inofree > 0) {
		fprintf(stderr, "%s: monitor inode: %s for %.2Lf%% free\n",
				ti->ti_section, ti->ti_mountdir, ti->ti_inofree);

		if(svbuf.f_files == 0) {
			/* test for zero inodes reported (e.g. AWS EFS) */
			fprintf(stderr, "%s: no inode support: %s\n", ti->ti_section,
					ti->ti_mountdir);
			ti->ti_inofree = 0;
		}
	}

	if(ti->ti_diskfree == 0 && ti->ti_inofree == 0) {
		/* nothing to do here */
		return ((void *)0);
	}

	if(ti->ti_retmin)
		fprintf(stderr, "%s: monitor file: %s for retmin %d\n",
				ti->ti_section, ti->ti_pcrestr, ti->ti_retmin);

	/* monitor filesystem usage */
	/* let's not start all the same thread types at once */

	srand(pthread_self());
	usleep((useconds_t) rand() & 0xffff);

	for(;;) {
		if(getvfsstats(ti, &pc_bfree, &pc_ffree) == FALSE)
			return ((void *)0);

		lowres = LOW_RES(ti->ti_diskfree, pc_bfree) || LOW_RES(ti->ti_inofree, pc_ffree);

		if(lowres || runreport) {
			resreport(ti, lowres, pc_bfree, pc_ffree);
			runreport = FALSE;
		}

		if(lowres) {
			pthread_mutex_lock(&dfslock);

			if(findfile(ti, TRUE, &nextid, ti->ti_dirname, db) > 0) {
				/* process directories emptied by previous run */

				if(ti->ti_rmdir)
					process_dirs(ti, db);

				/* process matching files */

				process_files(ti, db);
				runreport = TRUE;
			}

			pthread_mutex_unlock(&dfslock);
			continue;
		}

		sleep(dryrun ? DRYSCAN : SCANRATE);
	}

	/* notreached */
	return ((void *)0);
}

static void resreport(struct thread_info *ti, short lowres,
					  long double blk, long double ino)
{
	if(lowres == FALSE) {							/* initial/recovery report */
		if(ti->ti_diskfree > 0)
			fprintf(stderr, "%s: %s: %.2Lf%% blocks free\n",
					ti->ti_section, ti->ti_dirname, blk);

		if(ti->ti_inofree > 0)
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

	fflush(stderr);
}

static void process_files(struct thread_info *ti, sqlite3 *db)
{
	char    filename[PATH_MAX];						/* full pathname */
	char    stmt[BUFSIZ];							/* statement buffer */
	char   *db_dir;									/* sql data */
	char   *db_file;								/* sql data */
	extern short dryrun;							/* dry run bool */
	int     dfd;									/* dirname fd */
	int     drcount = 0;							/* dryrun count */
	long double pc_bfree = 0;						/* blocks free */
	long double pc_ffree = 0;						/* files free */
	sqlite3_stmt *pstmt;							/* prepared statement */
	uint32_t filecount;								/* matching files */
	uint32_t removed = 0;							/* matching files removed */

	/* count all files */

	if((filecount = count_files(ti, db)) < 1)
		return;

	/* process all files */

	snprintf(stmt, BUFSIZ, sql_selectfiles, ti->ti_task, ti->ti_task);
	sqlite3_prepare_v2(db, stmt, -1, &pstmt, NULL);

	for(;;) {
		if(ti->ti_retmin && filecount <= ti->ti_retmin)
			break;

		if(dryrun && drcount++ == 10) {				/* dryrun doesn't remove anything */
			if(!ti->ti_terse)
				fprintf(stderr, "%s: ...\n", ti->ti_section);

			break;
		}

		/* check if usage dropped before we got here */

		if(getvfsstats(ti, &pc_bfree, &pc_ffree) == FALSE)
			break;

		/*
		 * subtract padding from available space to
		 * provide a bit more space than the configured value
		 */

		if(!LOW_RES(ti->ti_diskfree, pc_bfree - PADDING) &&
		   !LOW_RES(ti->ti_inofree, pc_ffree - PADDING))
			break;

		if(sqlite3_step(pstmt) != SQLITE_ROW)
			break;

		db_dir = (char *)sqlite3_column_text(pstmt, 0);
		db_file = (char *)sqlite3_column_text(pstmt, 1);

		if(NOT_NULL(db_dir))
			snprintf(filename, PATH_MAX, "%s/%s/%s", ti->ti_dirname, db_dir, db_file);
		else
			snprintf(filename, PATH_MAX, "%s/%s", ti->ti_dirname, db_file);

		if(rmfile(ti, filename, "remove")) {
			removed++;
			filecount--;
		}
	}

	sqlite3_finalize(pstmt);

	/* modified buffer cache pages */

	if((dfd = open(ti->ti_dirname, R_OK)) > 0) {
		fdatasync(dfd);
		close(dfd);
	}

	if(removed)
		fprintf(stderr, "%s: %u %s removed\n", ti->ti_section,
				removed, removed == 1 ? "file" : "files");
}

static short getvfsstats(struct thread_info *ti, long double *blk, long double *ino)
{
	struct statvfs svbuf;							/* filesystem status */

	if(statvfs(ti->ti_mountdir, &svbuf) == -1) {
		fprintf(stderr, "%s: cannot stat: %s\n", ti->ti_section, ti->ti_mountdir);
		return (FALSE);
	}

	if(ti->ti_diskfree > 0)
		*blk = PERCENT(svbuf.f_bavail, svbuf.f_blocks);

	if(ti->ti_inofree > 0)
		*ino = PERCENT(svbuf.f_favail, svbuf.f_files);

	return (TRUE);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
