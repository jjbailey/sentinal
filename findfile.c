/*
 * findfile.c
 * Check dir and possibly subdirs for files matching pcrestr (pcrecmp).
 * Return the number of files matching pcrestr.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <sqlite3.h>
#include <string.h>
#include "sentinal.h"

static char *sql_insertdir = "INSERT INTO %s_dir VALUES(%u, '%s', 0);";
static char *sql_insertfile = "INSERT INTO %s_file VALUES(?, ?, ?, ?);";
static char *sql_updatedir = "UPDATE %s_dir SET db_empty = 1 WHERE db_id = %d;";

uint32_t findfile(struct thread_info *ti, short top, uint32_t *nextid,
				  char *dir, sqlite3 *db)
{
	DIR    *dirp;
	char    filename[PATH_MAX];						/* full pathname */
	char    stmt[BUFSIZ];							/* statement buffer */
	sqlite3_stmt *pstmt;							/* prepared statement */
	struct dirent *dp;
	struct stat stbuf;								/* file status */
	uint32_t entries = 0;							/* file entries */
	uint32_t rowid = *nextid;						/* db_id, db_dirid */

	if((dirp = opendir(dir)) == NULL)
		return (0);

	if(top) {
		if(stat(dir, &stbuf) == -1) {
			fprintf(stderr, "%s: cannot stat: %s\n", ti->ti_section, dir);
			return (0);
		}

		if(drop_table(ti, db) == FALSE)
			return (0);

		if(create_table(ti, db) == FALSE)
			return (0);

		if(journal_mode(ti, db) == FALSE)
			return (0);

		if(sync_commit(ti, db) == FALSE)
			return (0);

		ti->ti_dev = stbuf.st_dev;					/* save mountpoint device */
		rowid = *nextid = 1;						/* starting over */
	}

	/* directory insert */

	sqlexec(ti, db, "insert", sql_insertdir,
			ti->ti_task, rowid, top ? "" : dir + strlen(ti->ti_dirname) + 1);

	/* prepare file insert */

	snprintf(stmt, BUFSIZ, sql_insertfile, ti->ti_task);
	sqlite3_prepare_v2(db, stmt, -1, &pstmt, NULL);

	while((dp = readdir(dirp))) {
		if(MY_DIR(dp->d_name) || MY_PARENT(dp->d_name))
			continue;

		snprintf(filename, PATH_MAX, "%s/%s", dir, dp->d_name);

		if(lstat(filename, &stbuf) == -1)
			continue;

		if(S_ISLNK(stbuf.st_mode) && !ti->ti_symlinks)
			continue;

		if(stat(filename, &stbuf) == -1)
			continue;

		if(stbuf.st_dev != ti->ti_dev)				/* never cross filesystems */
			continue;

		if(S_ISDIR(stbuf.st_mode)) {				/* next db_dirid */
			(*nextid)++;
			entries += findfile(ti, FALSE, nextid, filename, db);
			continue;
		}

		entries++;

		if(!S_ISREG(stbuf.st_mode) || !namematch(ti, dp->d_name))
			continue;

		sqlite3_reset(pstmt);
		sqlite3_bind_int(pstmt, 1, rowid);
		sqlite3_bind_text(pstmt, 2, dp->d_name, -1, NULL);
		sqlite3_bind_int(pstmt, 3, stbuf.st_mtim.tv_sec);
		sqlite3_bind_int(pstmt, 4, stbuf.st_size);
		sqlite3_step(pstmt);
	}

	sqlite3_finalize(pstmt);
	closedir(dirp);

	/*
	 * directory entries update
	 * we are interested only in empty directories
	 */

	if(!entries)
		sqlexec(ti, db, "update", sql_updatedir, ti->ti_task, rowid);

	if(top)											/* indexes */
		create_index(ti, db);

	/* sqlite3_db_cacheflush(db); */
	return (entries);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
