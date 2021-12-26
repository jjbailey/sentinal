/*
 * sentinal.h
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#ifndef _SYS_TYPES_H
# include <sys/types.h>
#endif

#ifndef _PCRE_H
# include <pcre.h>
#endif

#define	TRUE		1
#define	FALSE		0
#define	MAXARGS		32
#define	MAXSECT		16							/* arbitrary, can be more */

#ifndef PATH_MAX
# define	PATH_MAX	256
#endif

#ifndef	TASK_COMM_LEN
# define	TASK_COMM_LEN	16					/* task command name length */
#endif

#define	ONE_MINUTE	60							/* m */
#define	ONE_HOUR	(ONE_MINUTE * 60)			/* H or h */
#define	ONE_DAY		(ONE_HOUR * 24)				/* D or d */
#define	ONE_WEEK	(ONE_DAY * 7)				/* W or w */
#define	ONE_MONTH	(ONE_DAY * 30)				/* M */
#define	ONE_YEAR	(ONE_DAY * 365)				/* Y or y */

#define	ONE_KiB		1024						/* K or k */
#define	ONE_MiB		(ONE_KiB << 10)				/* M or m */
#define	ONE_GiB		(ONE_MiB << 10)				/* G or g */

#define	KiB(n)		((size_t) (n) >> 10)		/* convert bytes to KiB */
#define	MiB(n)		((size_t) (n) >> 20)		/* convert bytes to MiB */

#define	PIPESIZ		(4L * ONE_MiB)				/* tunable, just a guess */

#define	IS_NULL(s)	((s) == NULL || *(s) == '\0')

#define	_HOST_TOK	"%h"						/* hostname token */
#define	_DIR_TOK	"%p"						/* dirname (path) token */
#define	_FILE_TOK	"%n"						/* filename token */

struct thread_info {
	char   *ti_section;							/* section name */
	char   *ti_command;							/* thread command */
	int     ti_argc;							/* number of args in command */
	char   *ti_path;							/* path to command */
	char   *ti_argv[MAXARGS];					/* args in command */
	char   *ti_dirname;							/* FIFO directory name */
	char   *ti_subdirs;							/* subdirectory recursion flag */
	char   *ti_pipename;						/* FIFO name */
	char   *ti_template;						/* logfile template */
	char   *ti_pcrestr;							/* pcre for logfile match */
	pcre   *ti_pcrecmp;							/* compiled pcre */
	char   *ti_filename;						/* output file name */
	pid_t   ti_pid;								/* thread pid */
	uid_t   ti_uid;								/* thread uid */
	gid_t   ti_gid;								/* thread gid */
	int     ti_wfd;								/* worker stdin or EOF */
	int     ti_sig;								/* signal number received */
	off_t   ti_loglimit;						/* logfile rotate size */
	long double ti_diskfree;					/* desired percent blocks free */
	long double ti_inofree;						/* desired percent inodes free */
	int     ti_expire;							/* logfile expiration */
	int     ti_retmin;							/* logfile retention minimum */
	int     ti_retmax;							/* logfile retention maximum */
	char   *ti_postcmd;							/* command to run after log closes */
};

char   *convexpire(int, char *);
char   *findmnt(char *, char *);
char   *fullpath(char *, char *, char *);
char   *logname(char *, char *);
char   *threadname(char *, char *, char *);
gid_t   verifygid(char *);
int     logretention(char *);
int     mylogfile(char *, pcre *);
int     oldestfile(struct thread_info *, int, char *, char *, time_t *);
int     runcmd(int, char **, char **);
int     postcmd(struct thread_info *, char *);
off_t   logsize(char *);
pcre   *pcrecheck(char *, pcre *);
size_t  strlcat(char *, const char *, size_t);
size_t  strlcpy(char *, const char *, size_t);
uid_t   verifyuid(char *);
void    parent_signals(void);
void    sigparent(int);
void    sigreject(int);
void    substrstr(char *, char *, char *);
void    version(char *);
void   *dfsthread(void *);
void   *expthread(void *);
void   *slmthread(void *);
void   *workthread(void *);

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
