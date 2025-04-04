/*
 * sql.c
 * Most, but not all of the sqlite work.
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>
#include "sentinal.h"

bool sqlexec(struct thread_info *ti, sqlite3 *db, char *desc, char *format, ...)
{
	char    stmt[BUFSIZ];							/* statement buffer */
	int     tries;									/* lock retries */
	va_list ap;

	va_start(ap, format);
	va_end(ap);
	vsnprintf(stmt, BUFSIZ, format, ap);

	for(tries = 0; tries < 3; tries++) {
		switch (sqlite3_exec(db, stmt, NULL, 0, NULL)) {

		case SQLITE_OK:								/* no error */
		case SQLITE_ABORT:							/* ignore */
			return (true);

		case SQLITE_LOCKED:							/* retry */
			usleep((useconds_t) 100000);
			continue;

		default:
			break;
		}
	}

	fprintf(stderr, "%s: sqlite3_exec: %s: %s\n",
			ti->ti_section, desc, sqlite3_errmsg(db));

	return (false);
}

bool drop_table(struct thread_info *ti, sqlite3 *db)
{
	/* drop the directory table */
	char   *sql_dir = "DROP TABLE IF EXISTS %s_dir;";

	/* drop the file table */
	char   *sql_file = "DROP TABLE IF EXISTS %s_file;";

	if(sqlexec(ti, db, "drop table", sql_dir, ti->ti_task) == false)
		return (false);

	if(sqlexec(ti, db, "drop table", sql_file, ti->ti_task) == false)
		return (false);

	return (true);
}

bool create_table(struct thread_info *ti, sqlite3 *db)
{
	/* VARCHAR -- see sqlite3 faq #9 */

	char   *sql_dir = "CREATE TABLE IF NOT EXISTS %s_dir (\n \
		db_id     INT NOT NULL,\n \
		db_dir    VARCHAR(255) NOT NULL,\n \
		db_empty  BOOLEAN NOT NULL);";

	char   *sql_file = "CREATE TABLE IF NOT EXISTS %s_file (\n \
		db_dirid  INT NOT NULL,\n \
		db_file   VARCHAR(255) NOT NULL,\n \
		db_time   INT NOT NULL,\n \
		db_size   UNSIGNED BIG INT NOT NULL);";

	if(sqlexec(ti, db, "create table", sql_dir, ti->ti_task) == false)
		return (false);

	if(sqlexec(ti, db, "create table", sql_file, ti->ti_task) == false)
		return (false);

	return (true);
}

bool create_index(struct thread_info *ti, sqlite3 *db)
{
	/* create an index on the directory table */
	char   *sql_dir = "CREATE INDEX IF NOT EXISTS idx_%s_dir ON %s_dir (db_id);";

	/* create an index on the file table */
	char   *sql_file = "CREATE INDEX IF NOT EXISTS idx_%s_file ON %s_file (db_time);";

	if(sqlexec(ti, db, "create index", sql_dir, ti->ti_task, ti->ti_task) == false)
		return (false);

	if(sqlexec(ti, db, "create index", sql_file, ti->ti_task, ti->ti_task) == false)
		return (false);

	return (true);
}

bool journal_mode(struct thread_info *ti, sqlite3 *db)
{
	char   *sql_journal = "PRAGMA journal_mode = OFF;";

	if(sqlexec(ti, db, "disable journal", sql_journal) == false)
		return (false);

	return (true);
}

bool sync_commit(struct thread_info *ti, sqlite3 *db)
{
	char   *sql_sync = "PRAGMA synchronous = NORMAL;";

	if(sqlexec(ti, db, "synchronous commit", sql_sync) == false)
		return (false);

	return (true);
}

uint32_t count_dirs(struct thread_info *ti, sqlite3 *db)
{
	char    stmt[BUFSIZ];							/* statement buffer */
	sqlite3_stmt *pstmt;							/* prepared statement */
	uint32_t dircount;

	char   *sql_dircount = "SELECT COUNT(*)\n \
		FROM  %s_dir\n \
		WHERE db_empty = 1;";

	snprintf(stmt, BUFSIZ, sql_dircount, ti->ti_task);
	sqlite3_prepare_v2(db, stmt, -1, &pstmt, NULL);
	sqlite3_step(pstmt);
	dircount = (uint32_t) sqlite3_column_int(pstmt, 0);
	sqlite3_finalize(pstmt);
	return (dircount);
}

uint32_t count_files(struct thread_info *ti, sqlite3 *db)
{
	char    stmt[BUFSIZ];							/* statement buffer */
	sqlite3_stmt *pstmt;							/* prepared statement */
	uint32_t filecount;

	char   *sql_filecount = "SELECT COUNT(*)\n \
		FROM  %s_dir, %s_file\n \
		WHERE db_dirid = db_id;";

	snprintf(stmt, BUFSIZ, sql_filecount, ti->ti_task, ti->ti_task);
	sqlite3_prepare_v2(db, stmt, -1, &pstmt, NULL);
	sqlite3_step(pstmt);
	filecount = (uint32_t) sqlite3_column_int(pstmt, 0);
	sqlite3_finalize(pstmt);
	return (filecount);
}

void process_dirs(struct thread_info *ti, sqlite3 *db)
{
	char    filename[PATH_MAX];						/* full pathname */
	char    stmt[BUFSIZ];							/* statement buffer */
	char   *db_dir;									/* sql data */
	extern bool dryrun;								/* dry run flag */
	int     drcount = 0;							/* dryrun count */
	sqlite3_stmt *pstmt;							/* prepared statement */
	uint32_t removed = 0;							/* directories removed */

	char   *sql_emptydirs = "SELECT db_dir\n \
		FROM  %s_dir\n \
		WHERE db_empty = 1\n \
		ORDER BY db_dir DESC;";

	if(count_dirs(ti, db) < 1)
		return;

	snprintf(stmt, BUFSIZ, sql_emptydirs, ti->ti_task);
	sqlite3_prepare_v2(db, stmt, -1, &pstmt, NULL);

	for(;;) {
		if(sqlite3_step(pstmt) != SQLITE_ROW)
			break;

		if(dryrun && drcount++ == 10) {				/* dryrun doesn't remove anything */
			if(!ti->ti_terse)
				fprintf(stderr, "%s: ...\n", ti->ti_section);

			break;
		}

		db_dir = (char *)sqlite3_column_text(pstmt, 0);

		if(NOT_NULL(db_dir)) {
			/* assemble dirname: ti_dirname + / + db_dir */

			snprintf(filename, PATH_MAX, "%s/%s", ti->ti_dirname, db_dir);

			if(rmfile(ti, filename, "rmdir"))
				removed++;
		}
	}

	sqlite3_finalize(pstmt);

	if(removed)
		fprintf(stderr, "%s: %u empty %s removed\n", ti->ti_section,
				removed, removed == 1 ? "directory" : "directories");
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
