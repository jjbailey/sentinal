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
 *  $ journalctl -f -n 20 -t sentinal
 *  $ journalctl -f _SYSTEMD_UNIT=example.service
 *  $ ps -lT -p $(pidof sentinal)
 *  $ top -H -S -p $(pidof sentinal)
 *  $ htop -d 5 -p $(pidof sentinal)
 *  # lslocks -p $(pidof sentinal)
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
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"
#include "ini.h"

static int parsecmd(char *, char **);
static short create_pid_file(char *);
static short emptyconfig(struct thread_info *);
static void dump_thread_info(struct thread_info *);
static void help(char *);

/* external for signal handling */
struct thread_info tinfo[MAXSECT];

/* external for token expansion */
struct utsname utsbuf;

/* external for stderr buffer */
char    stderrbuf[BUFSIZ << 1];

/* iniget.c functions */
char   *my_ini(ini_t *, char *, char *);
int     get_sections(ini_t *, int, char **);
void    print_section(ini_t *, char *);

void    version(char *, FILE *);

int main(int argc, char *argv[])
{
	DIR    *dirp;
	char    inifile[PATH_MAX];
	char    rbuf[PATH_MAX];
	char    tbuf[PATH_MAX];
	char   *pidfile;
	char   *sections[MAXSECT];
	ini_t  *inidata;
	int     i;
	int     nsect;
	int     opt;
	pthread_t *dfsmons;
	pthread_t *expmons;
	pthread_t *slmmons;
	pthread_t *workers;
	short   debug = FALSE;
	short   dfs_started = FALSE;
	short   exp_started = FALSE;
	short   slm_started = FALSE;
	short   verbose = FALSE;
	short   wrk_started = FALSE;
	struct thread_info *ti;

	umask(umask(0) | 022);						/* don't set less restrictive */
	*inifile = '\0';
	setvbuf(stderr, stderrbuf, _IOLBF, sizeof(stderrbuf));

	while((opt = getopt(argc, argv, "dvf:V?")) != -1) {
		switch (opt) {

		case 'd':								/* debug INI parse */
			debug = TRUE;
			break;

		case 'v':								/* verbose debug */
			verbose = TRUE;
			break;

		case 'f':								/* INI file name */
			strlcpy(inifile, optarg, PATH_MAX);
			break;

		case 'V':								/* print version */
			version(argv[0], stdout);
			exit(EXIT_SUCCESS);

		case '?':								/* print usage */
			help(argv[0]);
			exit(EXIT_SUCCESS);

		default:								/* print error and usage */
			help(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if(IS_NULL(inifile)) {
		help(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* configure the threads */

	if((inidata = ini_load(inifile)) == NULL) {
		fprintf(stderr, "%s: can't load %s\n", argv[0], inifile);
		exit(EXIT_FAILURE);
	}

	if((nsect = get_sections(inidata, MAXSECT, sections)) == 0) {
		fprintf(stderr, "%s: nothing to do\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	uname(&utsbuf);								/* for debug/token expansion */

#if 0
	if(debug || verbose) {
		fprintf(stdout, "sysname:  %s\n", utsbuf.sysname);
		fprintf(stdout, "nodename: %s\n", utsbuf.nodename);
		fprintf(stdout, "release:  %s\n", utsbuf.release);
		fprintf(stdout, "version:  %s\n", utsbuf.version);
		fprintf(stdout, "\n");
	}
#endif

	if(debug) {
		for(i = 0; i < nsect; i++)
			print_section(inidata, sections[i]);

		exit(EXIT_SUCCESS);
	}

	/* INI global settings */

	/* only global setting supported */
	pidfile = strdup(my_ini(inidata, "global", "pidfile"));

	if(IS_NULL(pidfile) || *pidfile != '/') {
		fprintf(stderr, "%s: pidfile is null or path not absolute\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* INI thread settings */

	for(i = 0; i < nsect; i++) {
		ti = &tinfo[i];							/* shorthand */

		ti->ti_section = sections[i];
		ti->ti_command = strdup(my_ini(inidata, sections[i], "command"));
		ti->ti_argc = parsecmd(ti->ti_command, ti->ti_argv);

		if(!IS_NULL(ti->ti_command) && ti->ti_argc) {
			ti->ti_path = ti->ti_argv[0];

			/* get real path of argv[0] */

			if(IS_NULL(ti->ti_path) || *ti->ti_path != '/') {
				fprintf(stderr, "%s: command path is null or not absolute\n",
						sections[i]);
				exit(EXIT_FAILURE);
			}

			if(realpath(ti->ti_path, rbuf) == NULL) {
				fprintf(stderr, "%s: missing or bad command path\n", sections[i]);
				exit(EXIT_FAILURE);
			}
		}

		ti->ti_path = strdup(ti->ti_argc ? rbuf : "");

		/* get real path of directory */

		ti->ti_dirname = my_ini(inidata, sections[i], "dirname");

		if(IS_NULL(ti->ti_dirname) || *ti->ti_dirname != '/') {
			fprintf(stderr, "%s: dirname is null or not absolute\n", sections[i]);
			exit(EXIT_FAILURE);
		}

		if(realpath(ti->ti_dirname, rbuf) == NULL)
			*rbuf = '\0';

		ti->ti_dirname = strdup(rbuf);

		if(IS_NULL(ti->ti_dirname) || strcmp(ti->ti_dirname, "/") == 0) {
			fprintf(stderr, "%s: missing or bad dirname\n", sections[i]);
			exit(EXIT_FAILURE);
		}

		/* is ti_dirname a directory */

		if((dirp = opendir(ti->ti_dirname)) == NULL) {
			fprintf(stderr, "%s: dirname is not a directory\n", sections[i]);
			exit(EXIT_FAILURE);
		}

		closedir(dirp);

		/* directory recursion */

		ti->ti_subdirs = my_ini(inidata, sections[i], "subdirs");

		if(strcmp(ti->ti_subdirs, "1") == 0 || strcasecmp(ti->ti_subdirs, "true") == 0)
			ti->ti_subdirs = strdup("true");
		else
			ti->ti_subdirs = strdup("false");

		/*
		 * get/generate/vett real path of pipe
		 * pipe might not exist, or it might not be in dirname
		 */

		ti->ti_pipename = my_ini(inidata, sections[i], "pipename");

		if(!IS_NULL(ti->ti_pipename)) {
			if(strstr(ti->ti_pipename, "..")) {
				fprintf(stderr, "%s: bad pipename\n", sections[i]);
				exit(EXIT_FAILURE);
			}

			fullpath(ti->ti_dirname, ti->ti_pipename, tbuf);

			if(realpath(dirname(tbuf), rbuf) == NULL) {
				fprintf(stderr, "%s: missing or bad pipedir\n", sections[i]);
				exit(EXIT_FAILURE);
			}

			fullpath(rbuf, base(ti->ti_pipename), tbuf);
			ti->ti_pipename = strdup(tbuf);
		}

		ti->ti_template = strdup(base(my_ini(inidata, sections[i], "template")));
		ti->ti_pcrestr = my_ini(inidata, sections[i], "pcrestr");
		ti->ti_pcrecmp = pcrecheck(ti->ti_pcrestr, ti->ti_pcrecmp);
		ti->ti_filename = malloc(PATH_MAX);
		memset(ti->ti_filename, '\0', PATH_MAX);
		ti->ti_pid = (pid_t) 0;					/* only workers use this */
		ti->ti_uid = verifyuid(my_ini(inidata, sections[i], "uid"));
		ti->ti_gid = verifygid(my_ini(inidata, sections[i], "gid"));
		ti->ti_wfd = 0;							/* only workers use this */
		ti->ti_loglimit = logsize(my_ini(inidata, sections[i], "loglimit"));
		ti->ti_diskfree = fabs(atof(my_ini(inidata, sections[i], "diskfree")));
		ti->ti_inofree = fabs(atof(my_ini(inidata, sections[i], "inofree")));
		ti->ti_expire = logretention(my_ini(inidata, sections[i], "expire"));
		ti->ti_retmin = logsize(my_ini(inidata, sections[i], "retmin"));
		ti->ti_retmax = logsize(my_ini(inidata, sections[i], "retmax"));
		ti->ti_postcmd = malloc(BUFSIZ);
		memset(ti->ti_postcmd, '\0', BUFSIZ);
		strlcpy(ti->ti_postcmd, my_ini(inidata, sections[i], "postcmd"), BUFSIZ);

		if(verbose)
			dump_thread_info(ti);

		/* INI option combo checks */

		if(emptyconfig(ti)) {
			fprintf(stderr, "%s: nothing to do\n", sections[i]);
			exit(EXIT_FAILURE);
		}

		if(ti->ti_argc) {
			if(IS_NULL(ti->ti_pipename)) {
				fprintf(stderr, "%s: command requires a pipe\n", sections[i]);
				exit(EXIT_FAILURE);
			}

			if(IS_NULL(ti->ti_template)) {
				fprintf(stderr, "%s: command requires a template\n", sections[i]);
				exit(EXIT_FAILURE);
			}
		}

		if(ti->ti_diskfree || ti->ti_inofree ||
		   ti->ti_expire || ti->ti_retmin || ti->ti_retmax) {
			if(!ti->ti_pcrecmp) {
				fprintf(stderr, "%s: missing or bad pcre\n", sections[i]);
				exit(EXIT_FAILURE);
			}

			if(ti->ti_retmax && ti->ti_retmin > ti->ti_retmax) {
				if(verbose)
					fprintf(stdout, "%s: retmin is greater than retmax\n", sections[i]);

				ti->ti_retmax = ti->ti_retmin;
			}
		}

		if(ti->ti_diskfree == 0 && ti->ti_inofree == 0 && ti->ti_expire == 0) {
			if(IS_NULL(ti->ti_template)) {
				fprintf(stderr, "%s: slm requires a template\n", sections[i]);
				exit(EXIT_FAILURE);
			}

			if(IS_NULL(ti->ti_postcmd)) {
				fprintf(stderr, "%s: slm requires a postcmd\n", sections[i]);
				exit(EXIT_FAILURE);
			}
		}

		if(verbose)
			activethreads(ti);
	}

	if(verbose)
		exit(EXIT_SUCCESS);

	/* for systemd */

	if(create_pid_file(pidfile) == FALSE) {
		fprintf(stderr, "%s: can't create pidfile or pidfile is in use\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	parent_signals();							/* important: signal handling */
	rlimit(MAXFILES);							/* limit the number of open files */

	/* setup threads and run */

	workers = (pthread_t *) malloc(nsect * sizeof(*workers));
	dfsmons = (pthread_t *) malloc(nsect * sizeof(*dfsmons));
	expmons = (pthread_t *) malloc(nsect * sizeof(*expmons));
	slmmons = (pthread_t *) malloc(nsect * sizeof(*slmmons));

	/* version banner */
	version(argv[0], stderr);

	for(i = 0; i < nsect; i++) {
		/* usleep for systemd journal */

		ti = &tinfo[i];							/* shorthand */

		if(threadcheck(ti, _WRK_THR)) {			/* worker (log ingestion) thread */
			usleep((useconds_t) 2000);
			pthread_create(&workers[i], NULL, &workthread, (void *)ti);
			wrk_started = TRUE;
		}

		if(threadcheck(ti, _EXP_THR)) {			/* logfile expiration, retention */
			usleep((useconds_t) 2000);
			pthread_create(&expmons[i], NULL, &expthread, (void *)ti);
			exp_started = TRUE;
		}

		if(threadcheck(ti, _DFS_THR)) {			/* filesystem free space */
			usleep((useconds_t) 2000);
			pthread_create(&dfsmons[i], NULL, &dfsthread, (void *)ti);
			dfs_started = TRUE;
		}

		if(threadcheck(ti, _SLM_THR)) {			/* simple log monitor */
			usleep((useconds_t) 2000);
			pthread_create(&slmmons[i], NULL, &slmthread, (void *)ti);
			slm_started = TRUE;
		}
	}

	for(i = 0; i < nsect; i++) {
		if(wrk_started)
			pthread_join(workers[i], NULL);

		if(exp_started)
			pthread_join(expmons[i], NULL);

		if(dfs_started)
			pthread_join(dfsmons[i], NULL);

		if(slm_started)
			pthread_join(slmmons[i], NULL);
	}

	exit(EXIT_SUCCESS);
}

static int parsecmd(char *cmd, char *argv[])
{
	char   *ap, argv0[PATH_MAX];
	char   *p;
	char    str[BUFSIZ];
	int     i = 0;

	if(IS_NULL(cmd))
		return (0);

	strlcpy(str, cmd, BUFSIZ);

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
	char   *zargv[MAXARGS];
	int     i;
	int     n;

	fprintf(stdout, "\n");
	fprintf(stdout, "section:  %s\n", ti->ti_section);
	fprintf(stdout, "command:  %s\n", ti->ti_command);
	fprintf(stdout, "argc:     %d\n", ti->ti_argc);
	fprintf(stdout, "path:     %s\n", ti->ti_path);

	fprintf(stdout, "argv:     ");
	for(i = 1; i < ti->ti_argc; i++)
		fprintf(stdout, "%s ", ti->ti_argv[i]);
	fprintf(stdout, "\n");

	fprintf(stdout, "dirname:  %s\n", ti->ti_dirname);
	fprintf(stdout, "subdirs:  %s\n", ti->ti_subdirs);
	fprintf(stdout, "pipename: %s\n", ti->ti_pipename);
	fprintf(stdout, "template: %s\n", ti->ti_template);
	fprintf(stdout, "pcrestr:  %s\n", ti->ti_pcrestr);
	fprintf(stdout, "uid:      %d\n", ti->ti_uid);
	fprintf(stdout, "gid:      %d\n", ti->ti_gid);
	fprintf(stdout, "loglimit: %ldMiB\n", MiB(ti->ti_loglimit));
	fprintf(stdout, "diskfree: %.2Lf\n", ti->ti_diskfree);
	fprintf(stdout, "inofree:  %.2Lf\n", ti->ti_inofree);
	fprintf(stdout, "expire:   %s\n", convexpire(ti->ti_expire, ebuf));
	fprintf(stdout, "retmin:   %d\n", ti->ti_retmin);
	fprintf(stdout, "retmax:   %d\n", ti->ti_retmax);

	logname(ti->ti_template, ti->ti_filename);

	if(ti->ti_argc) {
		n = runcmd(ti->ti_argc, ti->ti_argv, zargv);

		fprintf(stdout, "execcmd:  ");
		for(i = 0; i < n; i++)
			fprintf(stdout, "%s ", zargv[i]);
		fprintf(stdout, "> %s\n", ti->ti_filename);	/* show redirect */
	} else
		fprintf(stdout, "execcmd:  \n");

	/* postcmd tokens (1.3.0+) */

	strreplace(ti->ti_postcmd, _HOST_TOK, utsbuf.nodename);
	strreplace(ti->ti_postcmd, _PATH_TOK, ti->ti_dirname);
	strreplace(ti->ti_postcmd, _FILE_TOK, ti->ti_filename);
	strreplace(ti->ti_postcmd, _SECT_TOK, ti->ti_section);

	/* deprecated postcmd tokens (pre-1.3.0) */

	strreplace(ti->ti_postcmd, _OLD_HOST_TOK, utsbuf.nodename);
	strreplace(ti->ti_postcmd, _OLD_DIR_TOK, ti->ti_dirname);
	strreplace(ti->ti_postcmd, _OLD_FILE_TOK, ti->ti_filename);
	strreplace(ti->ti_postcmd, _OLD_SECT_TOK, ti->ti_section);

	fprintf(stdout, "postcmd:  %s\n", ti->ti_postcmd);
}

static short create_pid_file(char *pidfile)
{
	FILE   *fp;

	if(fp = fopen(pidfile, "r")) {
		int     locked = lockf(fileno(fp), F_TEST, (off_t) 0);

		fclose(fp);

		if(locked == -1)						/* not my lock */
			return (FALSE);
	}

	if(fp = fopen(pidfile, "w")) {
		fprintf(fp, "%d\n", getpid());
		fflush(fp);
		rewind(fp);

		if(lockf(fileno(fp), F_LOCK, (off_t) 0) == 0)
			return (TRUE);

		fclose(fp);
	}

	return (FALSE);
}

static short emptyconfig(struct thread_info *ti)
{
	/*
	 * is there anything to do
	 * note: config might be rejected during parsing
	 */

	if(ti->ti_argc ||
	   ti->ti_loglimit ||
	   ti->ti_diskfree || ti->ti_inofree ||
	   ti->ti_expire || ti->ti_retmin || ti->ti_retmax)
		return (FALSE);

	return (TRUE);
}

static void help(char *prog)
{
	char   *p = base(prog);

	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "%s -f <inifile>\n\n", p);
	fprintf(stderr, "Print the INI file as parsed, exit\n");
	fprintf(stderr, "%s -f <inifile> -d\n\n", p);
	fprintf(stderr, "Print the INI file as interpreted, exit\n");
	fprintf(stderr, "%s -f <inifile> -v\n\n", p);
	fprintf(stderr, "Print the program version, exit\n");
	fprintf(stderr, "%s -V\n\n", p);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
