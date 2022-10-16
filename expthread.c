/*
 * expthread.c
 * File expiration thread.  Remove files older than expire time.
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
#include <sys/stat.h>
#include <pthread.h>
#include <sqlite3.h>
#include <unistd.h>
#include "sentinal.h"

#define	SCANRATE		(ONE_MINUTE * 10)
#define	DRYSCAN			30							/* scanrate for dryrun */

static char *sql_selectfiles = "SELECT db_dir, db_file, db_size\n \
	FROM  %s_dir, %s_file\n \
	WHERE db_dirid = db_id\n \
	ORDER BY db_time;";

static char *sql_selectbytes = "SELECT SUM(db_size)\n \
	FROM  %s_file;";

static void process_files(struct thread_info *, sqlite3 *);

void   *expthread(void *arg)
{
	char    ebuf[BUFSIZ];							/* expire buffer */
	extern short dryrun;							/* dry run bool */
	extern sqlite3 *db;								/* db handle */
	short   loop1;
	struct thread_info *ti = arg;
	uint32_t nextid = 1;							/* db_id, db_dirid */

	/*
	 * this thread requires:
	 *  - ti_pcrecmp
	 *  - at least one of:
	 *    - ti_dirlimit
	 *    - ti_expire
	 *    - ti_retmax
	 */

	pthread_setname_np(pthread_self(), threadname(ti, _EXP_THR));

	if(ti->ti_dirlimit)
		fprintf(stderr, "%s: monitor directory: %s for dirlimit %ldMiB\n",
				ti->ti_section, ti->ti_dirname, MiB(ti->ti_dirlimit));

	if(ti->ti_expire)
		fprintf(stderr, "%s: monitor file: %s for expiration %s\n",
				ti->ti_section, ti->ti_pcrestr, convexpire(ti->ti_expire, ebuf));

	if(ti->ti_retmin)
		fprintf(stderr, "%s: monitor file: %s for retmin %d\n",
				ti->ti_section, ti->ti_pcrestr, ti->ti_retmin);

	if(ti->ti_retmax)
		fprintf(stderr, "%s: monitor file: %s for retmax %d\n",
				ti->ti_section, ti->ti_pcrestr, ti->ti_retmax);

	/* monitor expiration times */
	/* force a quick initial scan */

	loop1 = TRUE;

	for(;;) {
		sleep(dryrun || loop1 ? DRYSCAN : SCANRATE);
		loop1 = FALSE;

		/* find files in dirname */

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

static void process_files(struct thread_info *ti, sqlite3 *db)
{
	char    filename[PATH_MAX];						/* full pathname */
	char    stmt[BUFSIZ];							/* statement buffer */
	char   *db_dir;
	char   *db_file;
	char   *reason;
	extern short dryrun;							/* dry run bool */
	short   expbysize;								/* consider expire size */
	short   expbytime;								/* consider expire time */
	sqlite3_stmt *pstmt;							/* prepared statement */
	struct stat stbuf;								/* file status */
	time_t  curtime;
	uint32_t db_size;
	uint32_t filecount;								/* matching files */
	unsigned long long dirbytes;					/* dirsize in bytes */

	/* count all files */

	time(&curtime);

	if((filecount = count_files(ti, db)) < 1)
		return;

	/* count bytes in dir */

	snprintf(stmt, BUFSIZ, sql_selectbytes, ti->ti_task);
	sqlite3_prepare_v2(db, stmt, -1, &pstmt, NULL);
	sqlite3_step(pstmt);
	dirbytes = (unsigned long long)sqlite3_column_int64(pstmt, 0);
	sqlite3_finalize(pstmt);

	/* process expired files */

	snprintf(stmt, BUFSIZ, sql_selectfiles, ti->ti_task, ti->ti_task);
	sqlite3_prepare_v2(db, stmt, -1, &pstmt, NULL);

	for(;;) {
		if(ti->ti_retmin && filecount <= ti->ti_retmin)
			break;

		if(sqlite3_step(pstmt) != SQLITE_ROW)
			break;

		db_dir = (char *)sqlite3_column_text(pstmt, 0);
		db_file = (char *)sqlite3_column_text(pstmt, 1);
		db_size = (uint32_t) sqlite3_column_int(pstmt, 3);

		/* assemble filename: ti_dirname + / + db_dir + / + db_file */

		if(IS_NULL(db_dir))
			snprintf(filename, PATH_MAX, "%s/%s", ti->ti_dirname, db_file);
		else
			snprintf(filename, PATH_MAX, "%s/%s/%s", ti->ti_dirname, db_dir, db_file);

		if(stat(filename, &stbuf) == -1)			/* check for changes since db load */
			continue;

		/*
		 * if expiresiz is set, use it, else true
		 * if expire is set, use it, else false
		 */

		expbysize = !ti->ti_expiresiz || stbuf.st_size > ti->ti_expiresiz;
		expbytime = ti->ti_expire && stbuf.st_mtim.tv_sec + ti->ti_expire < curtime;

		/* files are sorted by time */

		if(ti->ti_retmax && filecount > ti->ti_retmax)
			reason = "retmax";						/* too many */

		else if(ti->ti_dirlimit && dirbytes > ti->ti_dirlimit)
			reason = "dirlimit";					/* too much */

		else if(expbysize && expbytime)
			reason = "expire";						/* too old */

		else if(!expbytime)							/* done with the list */
			break;

		else										/* none of the above */
			continue;

		if(rmfile(ti, filename, reason)) {
			filecount--;
			dirbytes -= db_size;
		}
	}

	sqlite3_finalize(pstmt);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
