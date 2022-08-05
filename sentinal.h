/*
 * sentinal.h
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	VERSION_STRING	"1.5.2"

#ifndef _SYS_TYPES_H
# include <sys/types.h>
#endif

#ifndef _PTHREAD_H
# include <pthread.h>
#endif

#ifndef PCRE2_H_IDEMPOTENT_GUARD
# define	PCRE2_CODE_UNIT_WIDTH	8
# include <pcre2.h>
#endif

#define	TRUE		(short)1
#define	FALSE		(short)0
#define	MAXARGS		32
#define	MAXSECT		16								/* arbitrary, can be more */

#define	MAXFILES	128								/* max open files */

#ifndef PATH_MAX
# define	PATH_MAX	256
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

#define	ONE_KiB		1024							/* K or k */
#define	ONE_MiB		(ONE_KiB << 10)					/* M or m */
#define	ONE_GiB		(ONE_MiB << 10)					/* G or g */

#define	KiB(n)		((size_t) (n) >> 10)			/* convert bytes to KiB */
#define	MiB(n)		((size_t) (n) >> 20)			/* convert bytes to MiB */

#define	FIFOSIZ		(64L * ONE_MiB)					/* tunable, just a guess */

#define NOT_NULL(s)	((s) && *(s))
#define IS_NULL(s)	!((s) && *(s))

/* alternative to often-used strcmp() */

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

#define	ENV		"/usr/bin/env"
#define	BASH	"/bin/bash"
#define	PATH	"/usr/bin:/usr/sbin:/bin"

struct thread_info {
	pthread_t dfs_tid;								/* dfs thread id */
	pthread_t exp_tid;								/* exp thread id */
	pthread_t slm_tid;								/* slm thread id */
	pthread_t wrk_tid;								/* wrk thread id */
	char    ti_task[TASK_COMM_LEN];					/* pthread_self */
	char   *ti_section;								/* section name */
	char   *ti_command;								/* thread command */
	int     ti_argc;								/* number of args in command */
	char   *ti_path;								/* path to command */
	char   *ti_argv[MAXARGS];						/* args in command */
	char   *ti_dirname;								/* directory name */
	long double ti_dirlimit;						/* directory size limit */
	short   ti_subdirs;								/* subdirectory recursion flag */
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
	off_t   ti_loglimit;							/* deprecated in v1.5.0 */
	off_t   ti_rotatesiz;							/* logfile rotate size */
	off_t   ti_expiresiz;							/* logfile expire size */
	long double ti_diskfree;						/* desired percent blocks free */
	long double ti_inofree;							/* desired percent inodes free */
	int     ti_expire;								/* file expiration */
	int     ti_retmin;								/* file retention minimum */
	int     ti_retmax;								/* file retention maximum */
	short   ti_terse;								/* notify file removal flag */
	short   ti_rmdir;								/* remove empty dirs */
	short   ti_symlinks;							/* follow symlinks */
	char   *ti_postcmd;								/* command to run after log closes */
	long    ti_matches;								/* matching files */
};

struct dir_info {
	char    di_file[PATH_MAX];						/* oldest file in directory */
	time_t  di_time;								/* oldest file modification time */
	off_t   di_size;								/* oldest file size */
	long double di_bytes;							/* total size of files found */
};

char   *convexpire(int, char *);
char   *findmnt(char *, char *);
char   *fullpath(char *, char *, char *);
char   *logname(char *, char *);
char   *threadname(struct thread_info *, char *);
gid_t   verifygid(char *);
int     logretention(char *);
int     postcmd(struct thread_info *, char *);
int     threadcheck(struct thread_info *, char *);
int     workcmd(int, char **, char **);
long    findfile(struct thread_info *, short, char *, struct dir_info *);
off_t   logsize(char *);
short   rmfile(struct thread_info *, char *, char *);
short   mylogfile(char *, pcre2_code *);
short   pcrecompile(struct thread_info *);
size_t  strlcat(char *, const char *, size_t);
size_t  strlcpy(char *, const char *, size_t);
uid_t   verifyuid(char *);
void    activethreads(struct thread_info *);
void    parentsignals(void);
void    rlimit(int);
void    strreplace(char *, char *, char *);
void   *dfsthread(void *);
void   *expthread(void *);
void   *slmthread(void *);
void   *workthread(void *);

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
