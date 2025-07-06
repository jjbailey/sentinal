/*
 * sql.c
 * Most, but not all of the sqlite work.
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 *
 * Note: All SQLite access must be serialized if using threads.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>
#include "sentinal.h"

#define RETRY_COUNT			3
#define RETRY_DELAY_USEC	100000
#define SQL_DIR_FMT			"DROP TABLE IF EXISTS %s_dir;"
#define SQL_FILE_FMT		"DROP TABLE IF EXISTS %s_file;"
#define SQL_CREATE_DIR_FMT \
	"CREATE TABLE IF NOT EXISTS %s_dir (db_id INT NOT NULL, db_dir VARCHAR(255) NOT NULL, db_empty BOOLEAN NOT NULL);"
#define SQL_CREATE_FILE_FMT \
	"CREATE TABLE IF NOT EXISTS %s_file (db_dirid INT NOT NULL, db_file VARCHAR(255) NOT NULL, db_time INT NOT NULL, db_size UNSIGNED BIG INT NOT NULL);"
#define SQL_INDEX_DIR_FMT	"CREATE INDEX IF NOT EXISTS idx_%s_dir ON %s_dir (db_id);"
#define SQL_INDEX_FILE_FMT	"CREATE INDEX IF NOT EXISTS idx_%s_file ON %s_file (db_time);"
#define SQL_COUNT_DIR_FMT	"SELECT COUNT(*) FROM %s_dir WHERE db_empty = 1;"
#define SQL_COUNT_FILE_FMT	"SELECT COUNT(*) FROM %s_dir, %s_file WHERE db_dirid = db_id;"
#define SQL_EMPTYDIRS_FMT	"SELECT db_dir FROM %s_dir WHERE db_empty = 1 ORDER BY db_dir DESC;"

static bool execute_sql(const struct thread_info *ti, sqlite3 *db, const char *desc,
						const char *format, va_list ap)
{
	char    stmtbuf[BUFSIZ];						/* statement buffer */
	int     rc;										/* return code */

	vsnprintf(stmtbuf, sizeof(stmtbuf), format, ap);

	for(int tries = 0; tries < RETRY_COUNT; ++tries) {
		rc = sqlite3_exec(db, stmtbuf, NULL, NULL, NULL);

		if(rc == SQLITE_OK || rc == SQLITE_ABORT)
			return (true);

		if(rc == SQLITE_LOCKED) {
			usleep(RETRY_DELAY_USEC);
			continue;
		}

		fprintf(stderr, "%s: sqlite3_exec (%s): %s\n",
				ti->ti_section, desc, sqlite3_errmsg(db));

		return (false);
	}

	return (false);
}

bool sqlexec(struct thread_info *ti, sqlite3 *db, char *desc, char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	bool    result = execute_sql(ti, db, desc, format, ap);

	va_end(ap);
	return (result);
}

static inline bool run_sql_fmt(const struct thread_info *ti, sqlite3 *db,
							   const char *desc, const char *sql_fmt, const char *arg1,
							   const char *arg2)
{
	char    stmtbuf[BUFSIZ];						/* statement buffer */

	if(arg2)
		snprintf(stmtbuf, sizeof(stmtbuf), sql_fmt, arg1, arg2);
	else
		snprintf(stmtbuf, sizeof(stmtbuf), sql_fmt, arg1);

	return (sqlexec((struct thread_info *)ti, db, (char *)desc, "%s", stmtbuf));
}

bool drop_table(struct thread_info *ti, sqlite3 *db)
{
	return (run_sql_fmt(ti, db, "drop table", SQL_DIR_FMT, ti->ti_task, NULL) &&
			run_sql_fmt(ti, db, "drop table", SQL_FILE_FMT, ti->ti_task, NULL));
}

bool create_table(struct thread_info *ti, sqlite3 *db)
{
	return (run_sql_fmt(ti, db, "create table", SQL_CREATE_DIR_FMT, ti->ti_task, NULL) &&
			run_sql_fmt(ti, db, "create table", SQL_CREATE_FILE_FMT, ti->ti_task, NULL));
}

bool create_index(struct thread_info *ti, sqlite3 *db)
{
	return (run_sql_fmt
			(ti, db, "create index", SQL_INDEX_DIR_FMT, ti->ti_task, ti->ti_task) &&
			run_sql_fmt(ti, db, "create index", SQL_INDEX_FILE_FMT, ti->ti_task,
						ti->ti_task));
}

bool journal_mode(struct thread_info *ti, sqlite3 *db)
{
	return (sqlexec(ti, db, "disable journal", "PRAGMA journal_mode = OFF;"));
}

bool sync_commit(struct thread_info *ti, sqlite3 *db)
{
	return (sqlexec(ti, db, "synchronous commit", "PRAGMA synchronous = NORMAL;"));
}

static uint32_t get_count(const struct thread_info *ti, sqlite3 *db, const char *sql_fmt,
						  const char *arg1, const char *arg2)
{
	char    stmtbuf[BUFSIZ];						/* statement buffer */
	int     rc;										/* return code */
	sqlite3_stmt *pstmt = NULL;						/* prepared statement */
	uint32_t count = 0;

	// Format SQL using one or two arguments

	if(arg2)
		snprintf(stmtbuf, sizeof(stmtbuf), sql_fmt, arg1, arg2);
	else
		snprintf(stmtbuf, sizeof(stmtbuf), sql_fmt, arg1);

	rc = sqlite3_prepare_v2(db, stmtbuf, -1, &pstmt, NULL);

	if(rc == SQLITE_OK) {
		if(sqlite3_step(pstmt) == SQLITE_ROW)
			count = (uint32_t) sqlite3_column_int(pstmt, 0);

		sqlite3_finalize(pstmt);
	} else {
		fprintf(stderr, "%s: sqlite3_prepare_v2 (count): %s\n",
				ti->ti_section, sqlite3_errmsg(db));
	}

	return (count);
}

uint32_t count_dirs(struct thread_info *ti, sqlite3 *db)
{
	return (get_count(ti, db, SQL_COUNT_DIR_FMT, ti->ti_task, NULL));
}

uint32_t count_files(struct thread_info *ti, sqlite3 *db)
{
	return (get_count(ti, db, SQL_COUNT_FILE_FMT, ti->ti_task, ti->ti_task));
}

void process_dirs(struct thread_info *ti, sqlite3 *db)
{
	char    stmtbuf[BUFSIZ];						/* statement buffer */
	extern bool dryrun;								/* dry run flag */
	int     drcount = 0;							/* dry run count */
	int     rc;										/* return code */
	sqlite3_stmt *pstmt = NULL;						/* prepared statement */
	uint32_t removed = 0;							/* directories removed */

	if(count_dirs(ti, db) < 1)
		return;

	snprintf(stmtbuf, sizeof(stmtbuf), SQL_EMPTYDIRS_FMT, ti->ti_task);
	rc = sqlite3_prepare_v2(db, stmtbuf, -1, &pstmt, NULL);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "%s: sqlite3_prepare_v2: %s\n", ti->ti_section,
				sqlite3_errmsg(db));

		return;
	}

	while((rc = sqlite3_step(pstmt)) == SQLITE_ROW) {
		if(dryrun && ++drcount > 10) {
			if(!ti->ti_terse)
				fprintf(stderr, "%s: ...\n", ti->ti_section);

			break;
		}

		const char *db_dir = (const char *)sqlite3_column_text(pstmt, 0);

		if(db_dir && *db_dir) {
			char    filename[PATH_MAX];

			snprintf(filename, sizeof(filename), "%s/%s", ti->ti_dirname, db_dir);

			if(rmfile(ti, filename, "rmdir"))
				removed++;
		}
	}

	if(rc != SQLITE_DONE && rc != SQLITE_ERROR)
		fprintf(stderr, "%s: sqlite3_step: %s\n", ti->ti_section, sqlite3_errmsg(db));

	sqlite3_finalize(pstmt);

	if(removed > 0)
		fprintf(stderr, "%s: %u empty %s removed\n", ti->ti_section,
				removed, removed == 1 ? "directory" : "directories");
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
