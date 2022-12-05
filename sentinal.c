/*
 * sentinal.c
 * sentinal: Manage directory contents according to an INI file
 *
 * Copyright (c) 2021, 2022 jjb
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
#include <dirent.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"
#include "ini.h"

/* externals */
int     dryrun = FALSE;								/* dry run bool */
sqlite3 *db;										/* db handle */
struct thread_info tinfo[MAXSECT];					/* our threads */
struct utsname utsbuf;								/* for host info */

static int parsecmd(char *, char **);
static short create_pid_file(char *);
static short setiniflag(ini_t *, char *, char *);
static void dump_thread_info(struct thread_info *);
static void help(char *);
static void threadwait(char *, pthread_t, short *, char *, short);

static int debug = FALSE;
static int verbose = FALSE;

static struct option long_options[] = {
	{ "dry-run", no_argument, &dryrun, 'D' },
	{ "debug", no_argument, &debug, 'd' },
	{ "version", no_argument, NULL, 'V' },
	{ "verbose", no_argument, &verbose, 'v' },
	{ "ini-file", required_argument, NULL, 'f' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 }
};

/* iniget.c functions */
char   *my_ini(ini_t *, char *, char *);
int     get_sections(ini_t *, int, char **);
void    print_global(ini_t *, char *);
void    print_section(ini_t *, char *);

int main(int argc, char *argv[])
{
	DIR    *dirp;
	char   *myname;
	char    database[PATH_MAX];
	char    inifile[PATH_MAX], rpinifile[PATH_MAX];
	char    rbuf[PATH_MAX];
	char    tbuf[PATH_MAX];
	char   *p;
	char   *pidfile;
	char   *sections[MAXSECT];
	ini_t  *inidata;
	int     c;
	int     dbflags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
	int     i;
	int     index = 0;
	int     nsect;
	struct stat stbuf;								/* file status */
	struct thread_info *ti;							/* thread settings */

	myname = base(argv[0]);

	umask(umask(0) | 022);							/* don't set less restrictive */
	*inifile = '\0';

	while(1) {
		c = getopt_long(argc, argv, "DdVvf:h?", long_options, &index);

		if(c == -1)									/* end of options */
			break;

		switch (c) {

		case 'D':									/* dry run mode */
			dryrun = TRUE;
			break;

		case 'd':									/* debug INI parse */
			debug = TRUE;
			break;

		case 'V':									/* print version */
			fprintf(stdout, "%s: version %s\n", myname, VERSION_STRING);
			exit(EXIT_SUCCESS);

		case 'v':									/* verbose debug */
			verbose = TRUE;
			break;

		case 'f':									/* INI file name */
			strlcpy(inifile, optarg, PATH_MAX);
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
	verbose = verbose ? TRUE : FALSE;

	/* configure the threads */

	if((inidata = ini_load(inifile)) == NULL) {
		fprintf(stderr, "%s: can't load %s\n", myname, inifile);
		exit(EXIT_FAILURE);
	}

	if((nsect = get_sections(inidata, MAXSECT, sections)) == 0) {
		fprintf(stderr, "%s: nothing to do\n", myname);
		exit(EXIT_FAILURE);
	}

	/* protect the INI file */

	if(stat(inifile, &stbuf) == 0)
		if(stbuf.st_mode & S_IWGRP || stbuf.st_mode & S_IWOTH) {
			chmod(inifile, stbuf.st_mode & ~(S_IWGRP | S_IXGRP | S_IWOTH | S_IXOTH));
			fprintf(stderr, "%s: %s was writable by group or other\n", myname, inifile);
		}

	uname(&utsbuf);									/* for debug/token expansion */

	if(debug) {
		realpath(inifile, rpinifile);
		print_global(inidata, rpinifile);

		for(i = 0; i < nsect; i++)
			print_section(inidata, sections[i]);

		exit(EXIT_SUCCESS);
	}

	/* INI global settings */

	pidfile = strndup(my_ini(inidata, "global", "pidfile"), PATH_MAX);

	if(IS_NULL(pidfile) || *pidfile != '/') {
		fprintf(stderr, "%s: pidfile is null or path not absolute\n", myname);
		exit(EXIT_FAILURE);
	}

	p = my_ini(inidata, "global", "database");		/* optional */

	if(IS_NULL(p) || strcmp(p, SQLMEMDB) == 0)
		strlcpy(database, SQLMEMDB, PATH_MAX);
	else
		strlcpy(database, p, PATH_MAX);				/* verbatim */

	/* INI thread settings */

	if(verbose) {
		fprintf(stdout, "# %s\n\n", realpath(inifile, rbuf));
		fprintf(stdout, "[global]\n");
		fprintf(stdout, "pidfile   = %s\n", pidfile);
		fprintf(stdout, "database  = %s\n", database);
	}

	for(i = 0; i < nsect; i++) {
		ti = &tinfo[i];								/* shorthand */
		memset(ti, '\0', sizeof(struct thread_info));

		ti->ti_section = sections[i];

		ti->ti_command = strndup(my_ini(inidata, ti->ti_section, "command"), PATH_MAX);
		ti->ti_argc = parsecmd(ti->ti_command, ti->ti_argv);

		if(NOT_NULL(ti->ti_command) && ti->ti_argc) {
			ti->ti_path = ti->ti_argv[0];

			/* get real path of argv[0] */

			if(IS_NULL(ti->ti_path) || *ti->ti_path != '/') {
				fprintf(stderr, "%s: command path is null or not absolute\n",
						ti->ti_section);
				exit(EXIT_FAILURE);
			}

			if(realpath(ti->ti_path, rbuf) == NULL) {
				fprintf(stderr, "%s: missing or bad command path\n", ti->ti_section);
				exit(EXIT_FAILURE);
			}
		}

		ti->ti_path = strndup(ti->ti_argc ? rbuf : "", PATH_MAX);

		/* get real path of directory */

		ti->ti_dirname = my_ini(inidata, ti->ti_section, "dirname");

		if(IS_NULL(ti->ti_dirname) || *ti->ti_dirname != '/') {
			fprintf(stderr, "%s: dirname is null or not absolute\n", ti->ti_section);
			exit(EXIT_FAILURE);
		}

		if(realpath(ti->ti_dirname, rbuf) == NULL)
			*rbuf = '\0';

		ti->ti_dirname = strndup(rbuf, PATH_MAX);

		if(IS_NULL(ti->ti_dirname) || strcmp(ti->ti_dirname, "/") == 0) {
			fprintf(stderr, "%s: missing or bad dirname\n", ti->ti_section);
			exit(EXIT_FAILURE);
		}

		/* is ti_dirname a directory */

		if((dirp = opendir(ti->ti_dirname)) == NULL) {
			fprintf(stderr, "%s: dirname is not a directory\n", ti->ti_section);
			exit(EXIT_FAILURE);
		}

		closedir(dirp);

		/* directory recursion */
		ti->ti_subdirs = setiniflag(inidata, ti->ti_section, "subdirs");

		/* notify file removal */
		ti->ti_terse = setiniflag(inidata, ti->ti_section, "terse");

		/* remove empty dirs */
		ti->ti_rmdir = setiniflag(inidata, ti->ti_section, "rmdir");

		/* follow symlinks */
		ti->ti_symlinks = setiniflag(inidata, ti->ti_section, "symlinks");

		/* truncate slm-managed files */
		ti->ti_truncate = setiniflag(inidata, ti->ti_section, "truncate");

		/*
		 * get/generate/vett real path of pipe
		 * pipe might not exist, or it might not be in dirname
		 */

		ti->ti_pipename = my_ini(inidata, ti->ti_section, "pipename");

		if(NOT_NULL(ti->ti_pipename)) {
			if(strstr(ti->ti_pipename, "..")) {
				fprintf(stderr, "%s: bad pipename\n", ti->ti_section);
				exit(EXIT_FAILURE);
			}

			fullpath(ti->ti_dirname, ti->ti_pipename, tbuf);

			if(realpath(dirname(tbuf), rbuf) == NULL) {
				fprintf(stderr, "%s: missing or bad pipedir\n", ti->ti_section);
				exit(EXIT_FAILURE);
			}

			fullpath(rbuf, base(ti->ti_pipename), tbuf);
			ti->ti_pipename = strndup(tbuf, PATH_MAX);
		}

		ti->ti_template = malloc(BUFSIZ);			/* more than PATH_MAX */
		memset(ti->ti_template, '\0', BUFSIZ);
		strlcpy(ti->ti_template,
				base(my_ini(inidata, ti->ti_section, "template")), PATH_MAX);

		ti->ti_pcrestr = my_ini(inidata, ti->ti_section, "pcrestr");
		pcrecompile(ti);

		ti->ti_filename = malloc(BUFSIZ);			/* more than PATH_MAX */
		memset(ti->ti_filename, '\0', BUFSIZ);

		ti->ti_pid = (pid_t) 0;						/* only workers use this */
		ti->ti_wfd = 0;								/* only workers use this */

		ti->ti_uid = verifyuid(my_ini(inidata, ti->ti_section, "uid"));
		ti->ti_gid = verifygid(my_ini(inidata, ti->ti_section, "gid"));
		ti->ti_dirlimit = logsize(my_ini(inidata, ti->ti_section, "dirlimit"));
		ti->ti_rotatesiz = logsize(my_ini(inidata, ti->ti_section, "rotatesiz"));
		ti->ti_expiresiz = logsize(my_ini(inidata, ti->ti_section, "expiresiz"));
		ti->ti_diskfree = fabs(atof(my_ini(inidata, ti->ti_section, "diskfree")));
		ti->ti_inofree = fabs(atof(my_ini(inidata, ti->ti_section, "inofree")));
		ti->ti_expire = logretention(my_ini(inidata, ti->ti_section, "expire"));
		ti->ti_retmin = logsize(my_ini(inidata, ti->ti_section, "retmin"));
		ti->ti_retmax = logsize(my_ini(inidata, ti->ti_section, "retmax"));

		ti->ti_postcmd = malloc(BUFSIZ);
		memset(ti->ti_postcmd, '\0', BUFSIZ);
		strlcpy(ti->ti_postcmd, my_ini(inidata, ti->ti_section, "postcmd"), BUFSIZ);

		if(verbose) {
			dump_thread_info(ti);
			activethreads(ti);
		}
	}

	if(verbose)
		exit(EXIT_SUCCESS);

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

	for(i = 0; i < nsect; i++) {
		ti = &tinfo[i];								/* shorthand */

		/*
		 * start threads
		 * give them time to start and print their journal entries
		 */

		if(threadcheck(ti, _DFS_THR)) {				/* filesystem free space */
			if(!ti->ti_retmin)
				fprintf(stderr,
						"%s: notice: recommend setting retmin in dfs threads\n",
						ti->ti_section);

			usleep((useconds_t) 500000);

			fprintf(stderr, "%s: start %s thread: %s\n", ti->ti_section, _DFS_THR,
					ti->ti_dirname);

			ti->dfs_active = pthread_create(&ti->dfs_tid, NULL, &dfsthread, ti) == 0;
		}

		if(threadcheck(ti, _EXP_THR)) {				/* file expiration, retention, dirlimit */
			if(ti->ti_expiresiz && !ti->ti_expire)
				fprintf(stderr,
						"%s: warning: expire size = %ldMiB, expire time = 0 (off)\n",
						ti->ti_section, MiB(ti->ti_expiresiz));

			if(ti->ti_retmax && ti->ti_retmax < ti->ti_retmin) {
				fprintf(stderr,
						"%s: warning: retmax is less than retmin -- setting retmax = 0\n",
						ti->ti_section);

				ti->ti_retmax = 0;					/* don't lose anything */
			}

			usleep((useconds_t) 500000);

			fprintf(stderr, "%s: start %s thread: %s\n", ti->ti_section, _EXP_THR,
					ti->ti_dirname);

			ti->exp_active = pthread_create(&ti->exp_tid, NULL, &expthread, ti) == 0;
		}

		if(threadcheck(ti, _SLM_THR)) {				/* simple log monitor */
			usleep((useconds_t) 500000);

			fprintf(stderr, "%s: start %s thread: %s\n", ti->ti_section, _SLM_THR,
					ti->ti_dirname);

			ti->slm_active = pthread_create(&ti->slm_tid, NULL, &slmthread, ti) == 0;
		}

		if(threadcheck(ti, _WRK_THR)) {				/* worker (log ingestion) thread */
			usleep((useconds_t) 500000);

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

static int parsecmd(char *cmd, char *argv[])
{
	char    str[BUFSIZ];
	char   *ap, argv0[PATH_MAX];
	char   *p;
	int     i = 0;

	if(IS_NULL(cmd))
		return (0);

	strlcpy(str, cmd, BUFSIZ);
	strreplace(str, "\t", " ");

	if((p = strtok(str, " ")) == NULL)
		return (0);

	while(p) {
		if(i == 0) {
			ap = realpath(p, argv0);
			argv[i++] = strdup(IS_NULL(ap) ? p : ap);
		} else if(i < (MAXARGS - 1))
			argv[i++] = strdup(p);

		p = strtok(NULL, " ");
	}

	argv[i] = (char *)NULL;
	return (i);
}

static void dump_thread_info(struct thread_info *ti)
{
	char    ebuf[BUFSIZ];
	char    fbuf[BUFSIZ];
	char   *zargv[MAXARGS];
	int     i;
	int     n;

	*ebuf = *fbuf = '\0';

	fprintf(stdout, "\n[%s]\n", ti->ti_section);
	fprintf(stdout, "command   = %s\n", ti->ti_command);

	logname(ti->ti_template, fbuf);
	fullpath(ti->ti_dirname, fbuf, ti->ti_filename);

	if(ti->ti_argc) {
		n = workcmd(ti->ti_argc, ti->ti_argv, zargv);

		fprintf(stdout, "#           ");

		for(i = 0; i < n; i++)
			fprintf(stdout, "%s ", zargv[i]);

		if(NOT_NULL(ti->ti_filename))
			fprintf(stdout, "> %s\n", ti->ti_filename);
	}

	fprintf(stdout, "dirname   = %s\n", ti->ti_dirname);
	fprintf(stdout, "dirlimit  = %ldMiB\n", MiB(ti->ti_dirlimit));
	fprintf(stdout, "subdirs   = %d\n", ti->ti_subdirs);
	fprintf(stdout, "pipename  = %s\n", ti->ti_pipename);
	fprintf(stdout, "template  = %s\n", ti->ti_template);

	if(NOT_NULL(ti->ti_filename))
		fprintf(stdout, "#           %s\n", base(ti->ti_filename));

	fprintf(stdout, "pcrestr   = %s\n", ti->ti_pcrestr);
	fprintf(stdout, "uid       = %d\n", ti->ti_uid);
	fprintf(stdout, "gid       = %d\n", ti->ti_gid);

	fprintf(stdout, "rotatesiz = %ldMiB\n", MiB(ti->ti_rotatesiz));
	fprintf(stdout, "expiresiz = %ldMiB\n", MiB(ti->ti_expiresiz));
	fprintf(stdout, "diskfree  = %.2Lf\n", ti->ti_diskfree);
	fprintf(stdout, "inofree   = %.2Lf\n", ti->ti_inofree);
	fprintf(stdout, "expire    = %s\n", convexpire(ti->ti_expire, ebuf));
	fprintf(stdout, "retmin    = %d\n", ti->ti_retmin);
	fprintf(stdout, "retmax    = %d\n", ti->ti_retmax);
	fprintf(stdout, "terse     = %d\n", ti->ti_terse);
	fprintf(stdout, "rmdir     = %d\n", ti->ti_rmdir);
	fprintf(stdout, "symlinks  = %d\n", ti->ti_symlinks);

	/* postcmd tokens */

	fprintf(stdout, "postcmd   = %s\n", ti->ti_postcmd);

	strreplace(ti->ti_postcmd, _HOST_TOK, utsbuf.nodename);
	strreplace(ti->ti_postcmd, _PATH_TOK, ti->ti_dirname);
	strreplace(ti->ti_postcmd, _FILE_TOK, ti->ti_filename);
	strreplace(ti->ti_postcmd, _SECT_TOK, ti->ti_section);

	if(NOT_NULL(ti->ti_postcmd))
		fprintf(stdout, "#           %s\n", ti->ti_postcmd);

	fprintf(stdout, "truncate  = %d\n", ti->ti_truncate);
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

static short setiniflag(ini_t *ini, char *section, char *key)
{
	char   *inip;

	if(IS_NULL(inip = my_ini(ini, section, key)))
		return (FALSE);

	return (strcmp(inip, "1") == 0 || strcasecmp(inip, "true") == 0);
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
	char   *p = base(prog);

	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "%s -f|--ini-file ini-file\n\n", p);
	fprintf(stderr, "Print the INI file as parsed, exit:\n");
	fprintf(stderr, "%s -f|--ini-file ini-file -d|--debug\n\n", p);
	fprintf(stderr, "Print the INI file as interpreted, exit:\n");
	fprintf(stderr, "%s -f|--ini-file ini-file -v|--verbose\n\n", p);
	fprintf(stderr, "Dry run mode:\n");
	fprintf(stderr, "%s -f|--ini-file ini-file -D|--dry-run\n\n", p);
	fprintf(stderr, "Print the program version, exit:\n");
	fprintf(stderr, "%s -V|--version\n\n", p);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
