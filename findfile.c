/*
 * findfile.c
 * Check dir and possibly subdirs for files matching pcrestr (pcrecmp).
 * Return the number of files matching pcrestr.
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
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <sqlite3.h>
#include <string.h>
#include <errno.h>
#include "sentinal.h"

#define INSERT_DIR_SQL	"INSERT INTO %s_dir VALUES(?, ?, ?);"
#define INSERT_FILE_SQL	"INSERT INTO %s_file VALUES(?, ?, ?, ?);"
#define UPDATE_DIR_SQL	"UPDATE %s_dir SET db_empty = 1 WHERE db_id = ?;"

uint32_t findfile(struct thread_info *ti, bool top, uint32_t *nextid,
				  char *dir, sqlite3 *db)
{
	DIR    *dirp;
	char    fullpath[PATH_MAX];						/* full pathname */
	char    relpath[PATH_MAX];						/* relative pathname */
	char    stmtbuf[BUFSIZ];						/* statement buffer */
	int     rc;										/* return code */
	sqlite3_stmt *insert_dir_stmt = NULL;
	sqlite3_stmt *insert_file_stmt = NULL;
	sqlite3_stmt *update_dir_stmt = NULL;
	struct dirent *dp;
	struct stat st;									/* file status */
	uint32_t entries = 0;							/* file entries */
	uint32_t rowid = *nextid;						/* db_id, db_dirid */

	if((dirp = opendir(dir)) == NULL)
		return (0);

	if(top) {
		if(stat(dir, &st) == -1) {
			fprintf(stderr, "%s: cannot stat: %s: %s\n", ti->ti_section, dir,
					strerror(errno));

			closedir(dirp);
			return (0);
		}

		if(!drop_table(ti, db) || !create_table(ti, db) ||
		   !journal_mode(ti, db) || !sync_commit(ti, db)) {
			closedir(dirp);
			return (0);
		}

		ti->ti_dev = st.st_dev;						/* save mountpoint device */
		rowid = *nextid = 1;						/* starting over */

		sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
	}

	snprintf(stmtbuf, sizeof(stmtbuf), INSERT_DIR_SQL, ti->ti_task);

	if((rc = sqlite3_prepare_v2(db, stmtbuf, -1, &insert_dir_stmt, NULL)) != SQLITE_OK) {
		fprintf(stderr, "%s: sqlite3_prepare_v2 insert_dir: %s\n",
				ti->ti_section, sqlite3_errmsg(db));

		goto cleanup;
	}

	if(top) {
		*relpath = '\0';
	} else {
		size_t  prefix_len = strlen(ti->ti_dirname);
		snprintf(relpath, sizeof(relpath), "%s", dir + prefix_len + 1);
	}

	sqlite3_bind_int(insert_dir_stmt, 1, rowid);
	sqlite3_bind_text(insert_dir_stmt, 2, relpath, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(insert_dir_stmt, 3, 0);
	sqlite3_step(insert_dir_stmt);
	sqlite3_finalize(insert_dir_stmt);
	insert_dir_stmt = NULL;

	snprintf(stmtbuf, sizeof(stmtbuf), INSERT_FILE_SQL, ti->ti_task);

	if((rc = sqlite3_prepare_v2(db, stmtbuf, -1, &insert_file_stmt, NULL)) != SQLITE_OK) {
		fprintf(stderr, "%s: sqlite3_prepare_v2 insert_file: %s\n",
				ti->ti_section, sqlite3_errmsg(db));

		goto cleanup;
	}

	while((dp = readdir(dirp)) != NULL) {
		if(MY_DIR(dp->d_name) || MY_PARENT(dp->d_name))
			continue;

		if(snprintf(fullpath, sizeof(fullpath),
					"%s/%s", dir, dp->d_name) >= sizeof(fullpath)) {
			fprintf(stderr, "%s: path too long: %s/%s\n", ti->ti_section, dir,
					dp->d_name);

			continue;
		}

		if(lstat(fullpath, &st) == -1)
			continue;

		if(S_ISLNK(st.st_mode)) {
			if(!ti->ti_symlinks) {					/* count this entry */
				entries++;
				continue;
			}
		}

		if(st.st_dev != ti->ti_dev)					/* never cross filesystems */
			continue;

		if(S_ISDIR(st.st_mode)) {					/* next db_dirid */
			if(ti->ti_subdirs) {
				(*nextid)++;
				entries += findfile(ti, false, nextid, fullpath, db);
			}

			continue;
		}

		entries++;

		if(!S_ISREG(st.st_mode) || !namematch(ti, dp->d_name))
			continue;

		sqlite3_reset(insert_file_stmt);
		sqlite3_bind_int(insert_file_stmt, 1, rowid);
		sqlite3_bind_text(insert_file_stmt, 2, dp->d_name, -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(insert_file_stmt, 3, (int)st.st_mtim.tv_sec);
		sqlite3_bind_int(insert_file_stmt, 4, (int)st.st_size);
		sqlite3_step(insert_file_stmt);
	}

  cleanup:
	if(insert_file_stmt)
		sqlite3_finalize(insert_file_stmt);

	closedir(dirp);

	/*
	 * directory entries update
	 * we are interested only in empty directories
	 */

	if(entries == 0) {
		snprintf(stmtbuf, sizeof(stmtbuf), UPDATE_DIR_SQL, ti->ti_task);

		if(sqlite3_prepare_v2(db, stmtbuf, -1, &update_dir_stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_int(update_dir_stmt, 1, rowid);
			sqlite3_step(update_dir_stmt);
			sqlite3_finalize(update_dir_stmt);
		} else {
			fprintf(stderr, "%s: sqlite3_prepare_v2 update_dir: %s\n",
					ti->ti_section, sqlite3_errmsg(db));
		}
	}

	if(top) {										/* indexes */
		create_index(ti, db);
		sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
	}

	return (entries);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
