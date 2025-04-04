/*
 * sentinal.h
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	VERSION_STRING	"2.1.7"

#ifndef _STDBOOL_H
# include <stdbool.h>
#endif

#ifndef _SYS_TYPES_H
# include <sys/types.h>
#endif

#ifndef _PTHREAD_H
# include <pthread.h>
#endif

#ifndef SQLITE3_H
# include <sqlite3.h>
#endif

#ifndef PCRE2_H_IDEMPOTENT_GUARD
# define	PCRE2_CODE_UNIT_WIDTH	8
# include <pcre2.h>
#endif

#define	MAXARGS		32
#define	MAXSECT		16								/* arbitrary, can be more */

#define	MAXFILES	128								/* max open files */

#ifndef PATH_MAX
# define	PATH_MAX	255
#endif

#ifndef	TASK_COMM_LEN
# define	TASK_COMM_LEN	16						/* task command name length */
#endif

#define	ONE_MINUTE	60								/* m */
#define	ONE_HOUR	(ONE_MINUTE * 60)				/* H or h */
#define	ONE_DAY		(ONE_HOUR * 24)					/* D or d */
#define	ONE_WEEK	(ONE_DAY * 7)					/* W or w */
#define	ONE_MONTH	(ONE_DAY * 30)					/* M */
#define	ONE_YEAR	(ONE_DAY * 365)					/* Y or y */

#define	FIFOSIZ		(64 << 20)						/* 64MiB, better size for I/O */

#define	NOT_NULL(s)	((s) && *(s))
#define	IS_NULL(s)	!((s) && *(s))
#define	SLOWEXIT(code)		{ sleep(5); exit(code); }

/* alternatives to often-used strcmp() */

#define	MY_DIR(d)		(*d == '.' && *(d + 1) == '\0')
#define	MY_PARENT(d)	(*d == '.' && *(d + 1) == '.' && *(d + 2) == '\0')

/* postcmd tokens */

#define	_HOST_TOK	"%host"							/* hostname token */
#define	_PATH_TOK	"%path"							/* dirname (path) token */
#define	_FILE_TOK	"%file"							/* filename token */
#define	_SECT_TOK	"%sect"							/* INI section token */

/* thread names */

#define	_DFS_THR	"dfs"							/* filesystem free space */
#define	_EXP_THR	"exp"							/* file expiration, retention */
#define	_SLM_THR	"slm"							/* simple log monitor */
#define	_WRK_THR	"wrk"							/* worker (log ingestion) thread */

/* execution environment */

#define	ENV			"/usr/bin/env"
#define	BASH		"/bin/bash"
#define	PATH		"/usr/bin:/usr/sbin:/bin"

struct thread_info {
	pthread_t dfs_tid;								/* dfs thread id */
	pthread_t exp_tid;								/* exp thread id */
	pthread_t slm_tid;								/* slm thread id */
	pthread_t wrk_tid;								/* wrk thread id */
	bool    dfs_active;								/* pthread_t is opaque */
	bool    exp_active;								/* pthread_t is opaque */
	bool    slm_active;								/* pthread_t is opaque */
	bool    wrk_active;								/* pthread_t is opaque */
	char   *ti_task;								/* pthread_self */
	char   *ti_section;								/* section name */
	char   *ti_command;								/* thread command */
	int     ti_argc;								/* number of args in command */
	char   *ti_path;								/* path to command */
	char   *ti_argv[MAXARGS];						/* args in command */
	char   *ti_dirname;								/* directory name */
	char   *ti_mountdir;							/* mountpoint */
	dev_t   ti_dev;									/* ID of device containing file */
	char   *ti_dirlimstr;							/* directory size limit string */
	off_t   ti_dirlimit;							/* directory size limit */
	bool    ti_subdirs;								/* subdirectory recursion flag */
	char   *ti_pipename;							/* FIFO name */
	char   *ti_template;							/* file template */
	char   *ti_pcrestr;								/* pcre for file match */
	pcre2_code *ti_pcrecmp;							/* compiled pcre */
	char   *ti_filename;							/* output file name */
	pid_t   ti_pid;									/* thread pid */
	uid_t   ti_uid;									/* thread uid */
	gid_t   ti_gid;									/* thread gid */
	int     ti_wfd;									/* worker stdin or EOF */
	int     ti_sig;									/* signal number received */
	char   *ti_rotatestr;							/* logfile rotate size string */
	off_t   ti_rotatesiz;							/* logfile rotate size */
	char   *ti_expirestr;							/* logfile expire size string */
	off_t   ti_expiresiz;							/* logfile expire size */
	float   ti_diskfree;							/* desired percent blocks free */
	float   ti_inofree;								/* desired percent inodes free */
	int     ti_expire;								/* file expiration */
	char   *ti_retminstr;							/* file retention minimum string */
	int     ti_retmin;								/* file retention minimum */
	char   *ti_retmaxstr;							/* file retention maximum string */
	int     ti_retmax;								/* file retention maximum */
	bool    ti_terse;								/* notify file removal flag */
	bool    ti_rmdir;								/* remove empty dirs */
	bool    ti_symlinks;							/* follow symlinks */
	char   *ti_postcmd;								/* command to run after log closes */
	bool    ti_truncate;							/* truncate slm-managed files */
};

bool    namematch(struct thread_info *, char *);
bool    pcrecompile(struct thread_info *);
bool    rmfile(struct thread_info *, const char *, const char *);
bool    threadtype(struct thread_info *, char *);
bool    validdbname(char *);
char   *convexpire(int, char *);
char   *findmnt(char *, char *);
char   *fullpath(const char *, const char *, char *);
char   *logname(char *, char *);
char   *threadname(struct thread_info *, char *);
gid_t   verifygid(const char *);
int     logretention(char *);
int     postcmd(struct thread_info *, char *);
int     workcmd(int, char **, char **);
off_t   logsize(char *);
size_t  strlcat(char *, const char *, size_t);
size_t  strlcpy(char *, const char *, size_t);
uid_t   verifyuid(const char *);
uint32_t findfile(struct thread_info *, bool, uint32_t *, char *, sqlite3 *);
void    activethreads(struct thread_info *);
void   *dfsthread(void *);
void   *expthread(void *);
void    parentsignals(void);
void    rlimit(int);
void   *slmthread(void *);
void    strreplace(char *, const char *, const char *, size_t);
void   *workthread(void *);

/* sqlite */

#define	SQLMEMDB	":memory:"						/* pure in-memory database */
#define	QUERYLIM	100000							/* max return for dfs and exp */

bool    create_index(struct thread_info *, sqlite3 *);
bool    create_table(struct thread_info *, sqlite3 *);
bool    drop_table(struct thread_info *, sqlite3 *);
bool    journal_mode(struct thread_info *, sqlite3 *);
bool    sqlexec(struct thread_info *, sqlite3 *, char *, char *, ...);
bool    sync_commit(struct thread_info *, sqlite3 *);
uint32_t count_dirs(struct thread_info *, sqlite3 *);
uint32_t count_files(struct thread_info *, sqlite3 *);
void    process_dirs(struct thread_info *, sqlite3 *);

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
