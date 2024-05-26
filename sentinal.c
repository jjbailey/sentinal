/*
 * sentinal.c
 * sentinal: Manage directory contents according to an INI file.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 *
 * examples of commands to monitor activity:
 *  # journalctl -f -n 20 -t sentinal
 *  # journalctl -f _SYSTEMD_UNIT=example.service
 *  $ ps -lT -p $(pidof sentinal)
 *  $ top -H -S -p $(echo $(pgrep sentinal) | sed 's/ /,/g')
 *  $ htop -d 5 -p $(echo $(pgrep sentinal) | sed 's/ /,/g')
 *  # lslocks -p $(pidof sentinal)
 *  # pmap -X $(pidof sentinal)
 *
 * clean up the systemd logs:
 *  journalctl --vacuum-time=2d
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <getopt.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"
#include "ini.h"

static short create_pid_file(char *);
static void help(char *);
static void threadwait(char *, pthread_t, short *, char *, short);

static int debug = FALSE;
static int split = FALSE;
static int verbose = FALSE;

/* externals declared here */
char    database[PATH_MAX];							/* database file name */
char   *pidfile;									/* sentinal pid */
char   *sections[MAXSECT];							/* section names */
ini_t  *inidata;									/* loaded ini data */
int     dryrun = FALSE;								/* dry run bool */
pthread_mutex_t dfslock;							/* thread lock */
pthread_mutex_t explock;							/* thread lock */
sqlite3 *db;										/* db handle */
struct thread_info tinfo[MAXSECT];					/* our threads */
struct utsname utsbuf;								/* for host info */

static struct option long_options[] = {
	{ "ini-file", required_argument, NULL, 'f' },
	{ "debug", no_argument, &debug, 'd' },
	{ "dry-run", no_argument, &dryrun, 'D' },
	{ "split", no_argument, &split, 's' },
	{ "verbose", no_argument, &verbose, 'v' },
	{ "version", no_argument, NULL, 'V' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 }
};

int     readini(char *, char *);
void    activethreads(struct thread_info *);
void    debug_global(ini_t *, char *);
void    debug_section(ini_t *, char *);
void    debug_split(struct thread_info *, ini_t *);
void    debug_verbose(struct thread_info *);

int main(int argc, char *argv[])
{
	char    inifile[PATH_MAX];						/* ini file name */
	char   *myname;
	char    tbuf[PATH_MAX];
	int     c;
	int     dbflags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
	int     i;
	int     index = 0;
	int     nsect;									/* number of sections found */
	struct thread_info *ti;							/* thread settings */

	myname = base(argv[0]);
	umask(umask(0) | 022);							/* don't set less restrictive */
	*inifile = '\0';

	while(1) {
		c = getopt_long(argc, argv, "DdsVvf:h?", long_options, &index);

		if(c == -1)									/* end of options */
			break;

		switch (c) {

		case 'D':									/* dry run mode */
			dryrun = TRUE;
			break;

		case 'd':									/* debug INI */
			debug = TRUE;
			break;

		case 's':									/* debug INI and split */
			split = TRUE;
			break;

		case 'V':									/* print version */
			fprintf(stdout, "%s: version %s\n", myname, VERSION_STRING);
			exit(EXIT_SUCCESS);

		case 'v':									/* verbose debug */
			verbose = TRUE;
			break;

		case 'f':									/* INI file name */
			fullpath("/opt/sentinal/etc", optarg, tbuf);
			realpath(tbuf, inifile);
			break;

		case 'h':									/* print usage */
		case '?':
			help(myname);
			exit(EXIT_SUCCESS);
		}
	}

	if(IS_NULL(inifile)) {
		help(myname);
		exit(EXIT_FAILURE);
	}

	/* convert long_options flags to bool */

	debug = debug ? TRUE : FALSE;
	dryrun = dryrun ? TRUE : FALSE;
	split = split ? TRUE : FALSE;
	verbose = verbose ? TRUE : FALSE;

	/* check the INI file for unsafe permissions */

	if((nsect = readini(myname, inifile)) == 0) {
		/* nothing to do */
		SLOWEXIT(EXIT_FAILURE);
	}

	if(debug || split || verbose) {
		/* in order of precedence */

		debug_global(inidata, inifile);

		if(split) {
			for(i = 0; i < nsect; i++) {
				ti = &tinfo[i];
				debug_split(ti, inidata);
			}
		} else if(debug) {
			for(i = 0; i < nsect; i++)
				debug_section(inidata, sections[i]);
		} else if(verbose) {
			for(i = 0; i < nsect; i++) {
				ti = &tinfo[i];
				debug_verbose(ti);
				activethreads(ti);
			}
		}

		exit(EXIT_SUCCESS);
	}

	/* for systemd */

	if(create_pid_file(pidfile) == FALSE) {
		fprintf(stderr, "%s: can't create pidfile or pidfile is in use\n", myname);
		exit(EXIT_FAILURE);
	}

	parentsignals();								/* important: signal handling */
	rlimit(MAXFILES);								/* limit the number of open files */

	/* version banner */

	fprintf(stderr, "%s: version %s %s\n", myname, VERSION_STRING,
			dryrun ? "(DRY RUN)" : "");

	/* remove and (re)create the database -- no point in keeping the old one */

	if(strcmp(database, SQLMEMDB) != 0)
		remove(database);

	if(sqlite3_open_v2(database, &db, dbflags, NULL) != SQLITE_OK) {
		fprintf(stderr, "%s: sqlite3_open_v2 failed\n", myname);
		exit(EXIT_FAILURE);
	}

	if(strcmp(database, SQLMEMDB) != 0)
		chmod(database, 0600);

	/* initialize thread locks */

	pthread_mutex_init(&dfslock, NULL);
	pthread_mutex_init(&explock, NULL);

	/*
	 * start threads
	 * give them time to start and print their journal entries
	 */

	for(i = 0; i < nsect; i++) {
		ti = &tinfo[i];								/* shorthand */

		if(threadtype(ti, _DFS_THR)) {				/* filesystem free space */
			if(!ti->ti_retmin)
				fprintf(stderr,
						"%s: notice: recommend setting retmin in dfs threads\n",
						ti->ti_section);

			usleep((useconds_t) 400000);

			fprintf(stderr, "%s: start %s thread: %s\n", ti->ti_section, _DFS_THR,
					ti->ti_dirname);

			ti->dfs_active = pthread_create(&ti->dfs_tid, NULL, &dfsthread, ti) == 0;
		}

		if(threadtype(ti, _EXP_THR)) {				/* file expiration, retention, dirlimit */
			if(ti->ti_expiresiz && !ti->ti_expire)
				fprintf(stderr,
						"%s: warning: expire size = %s, expire time = 0 (off) -- this is a noop\n",
						ti->ti_section, ti->ti_expirestr);

			if(ti->ti_retmax && ti->ti_retmax < ti->ti_retmin) {
				fprintf(stderr,
						"%s: warning: retmax is less than retmin -- setting retmax = 0\n",
						ti->ti_section);

				ti->ti_retmax = 0;					/* don't lose anything */
			}

			usleep((useconds_t) 400000);

			fprintf(stderr, "%s: start %s thread: %s\n", ti->ti_section, _EXP_THR,
					ti->ti_dirname);

			ti->exp_active = pthread_create(&ti->exp_tid, NULL, &expthread, ti) == 0;
		}

		if(threadtype(ti, _SLM_THR)) {				/* simple log monitor */
			usleep((useconds_t) 400000);

			fprintf(stderr, "%s: start %s thread: %s\n", ti->ti_section, _SLM_THR,
					ti->ti_dirname);

			ti->slm_active = pthread_create(&ti->slm_tid, NULL, &slmthread, ti) == 0;
		}

		if(threadtype(ti, _WRK_THR)) {				/* worker (log ingestion) thread */
			usleep((useconds_t) 400000);

			fprintf(stderr, "%s: start %s thread: %s\n", ti->ti_section, _WRK_THR,
					ti->ti_dirname);

			ti->wrk_active = pthread_create(&ti->wrk_tid, NULL, &workthread, ti) == 0;
		}
	}

	/* wait for threads that ended early */

	sleep(1);

	for(i = 0; i < nsect; i++) {
		ti = &tinfo[i];								/* shorthand */
		threadwait(ti->ti_section, ti->dfs_tid, &ti->dfs_active, _DFS_THR, FALSE);
		threadwait(ti->ti_section, ti->exp_tid, &ti->exp_active, _EXP_THR, FALSE);
		threadwait(ti->ti_section, ti->slm_tid, &ti->slm_active, _SLM_THR, FALSE);
		threadwait(ti->ti_section, ti->wrk_tid, &ti->wrk_active, _WRK_THR, FALSE);
	}

	/* wait for threads */

	for(i = 0; i < nsect; i++) {
		ti = &tinfo[i];								/* shorthand */
		threadwait(ti->ti_section, ti->dfs_tid, &ti->dfs_active, _DFS_THR, TRUE);
		threadwait(ti->ti_section, ti->exp_tid, &ti->exp_active, _EXP_THR, TRUE);
		threadwait(ti->ti_section, ti->slm_tid, &ti->slm_active, _SLM_THR, TRUE);
		threadwait(ti->ti_section, ti->wrk_tid, &ti->wrk_active, _WRK_THR, TRUE);
	}

	exit(EXIT_SUCCESS);
}

static short create_pid_file(char *pidfile)
{
	FILE   *fp;

	if((fp = fopen(pidfile, "r"))) {
		int     locked = lockf(fileno(fp), F_TEST, (off_t) 0);

		fclose(fp);

		if(locked == -1)							/* not my lock */
			return (FALSE);
	}

	if((fp = fopen(pidfile, "w"))) {
		fprintf(fp, "%d\n", getpid());
		fflush(fp);
		rewind(fp);

		if(lockf(fileno(fp), F_LOCK, (off_t) 0) == 0)
			return (TRUE);

		fclose(fp);
	}

	return (FALSE);
}

static void threadwait(char *section, pthread_t tid,
					   short *active, char *tname, short block)
{
	int     ret;

	if(*active == FALSE)
		return;

	ret = block ? pthread_join(tid, NULL) : pthread_tryjoin_np(tid, NULL);

	if(ret == 0) {
		fprintf(stderr, "%s: end %s thread\n", section, tname);
		*active = FALSE;
	}
}

static void help(char *prog)
{
	fprintf(stderr, "Usage: %s -f ini-file [-dsDvV]\n", base(prog));
	fprintf(stderr,
			" -f, --ini-file   INI file, full path or relative to /opt/sentinal/etc\n");
	fprintf(stderr, " -d, --debug      print the INI file, exit\n");
	fprintf(stderr, " -s, --split      print the INI file with split sections, exit\n");
	fprintf(stderr, " -D, --dry-run    don't remove anything\n");
	fprintf(stderr, " -v, --verbose    print the INI file as interpreted, exit\n");
	fprintf(stderr, " -V, --version    print version number, exit\n");
	fprintf(stderr, " -?, --help       this message\n");
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
