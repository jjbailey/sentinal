/*
 * sentinal.c
 * sentinal: Manage directory contents according to an INI file
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 *
 * examples of commands to monitor activity:
 *
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
#include <libgen.h>
#include <limits.h>								/* for realpath() */
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"
#include "ini.h"

static int create_pid_file(char *);
static int parsecmd(char *, char **);
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

int main(int argc, char *argv[])
{
	char    inifile[PATH_MAX];
	char    rbuf[PATH_MAX];
	char    tbuf[PATH_MAX];
	char   *pidfile;
	char   *sections[MAXSECT];
	ini_t  *inidata;
	int     debug = FALSE;
	int     i;
	int     nsect;
	int     opt;
	pthread_t *dfsmons;
	pthread_t *expmons;
	pthread_t *slmmons;
	pthread_t *workers;
	short   verbose = FALSE;
	short   dfs_started = FALSE;
	short   exp_started = FALSE;
	short   slm_started = FALSE;
	short   wrk_started = FALSE;

	umask(022);
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
			version(argv[0]);
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
	if(debug == TRUE || verbose == TRUE) {
		printf("sysname:  %s\n", utsbuf.sysname);
		printf("nodename: %s\n", utsbuf.nodename);
		printf("release:  %s\n", utsbuf.release);
		printf("version:  %s\n", utsbuf.version);
		printf("\n");
	}
#endif

	if(debug == TRUE) {
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
		tinfo[i].ti_section = sections[i];
		tinfo[i].ti_command = strdup(my_ini(inidata, sections[i], "command"));
		tinfo[i].ti_argc = parsecmd(tinfo[i].ti_command, tinfo[i].ti_argv);

		if(!IS_NULL(tinfo[i].ti_command) && tinfo[i].ti_argc) {
			tinfo[i].ti_path = tinfo[i].ti_argv[0];

			/* get real path of argv[0] */

			if(IS_NULL(tinfo[i].ti_path) || *tinfo[i].ti_path != '/') {
				fprintf(stderr, "%s: command path is null or not absolute\n",
						sections[i]);
				exit(EXIT_FAILURE);
			}

			if(realpath(tinfo[i].ti_path, rbuf) == NULL) {
				fprintf(stderr, "%s: missing or bad command path\n", sections[i]);
				exit(EXIT_FAILURE);
			}
		}

		tinfo[i].ti_path = strdup(tinfo[i].ti_argc ? rbuf : "");

		/* get real path of directory */

		tinfo[i].ti_dirname = my_ini(inidata, sections[i], "dirname");

		if(IS_NULL(tinfo[i].ti_dirname) || *tinfo[i].ti_dirname != '/') {
			fprintf(stderr, "%s: dirname is null or not absolute\n", sections[i]);
			exit(EXIT_FAILURE);
		}

		if(realpath(tinfo[i].ti_dirname, rbuf) == NULL)
			*rbuf = '\0';

		tinfo[i].ti_dirname = strdup(rbuf);

		if(IS_NULL(tinfo[i].ti_dirname) || strcmp(tinfo[i].ti_dirname, "/") == 0) {
			fprintf(stderr, "%s: missing or bad dirname\n", sections[i]);
			exit(EXIT_FAILURE);
		}

		/* directory recursion */

		tinfo[i].ti_subdirs = my_ini(inidata, sections[i], "subdirs");

		if(strcasecmp(tinfo[i].ti_subdirs, "1") == 0 ||
		   strcasecmp(tinfo[i].ti_subdirs, "true") == 0)
			tinfo[i].ti_subdirs = strdup("1");
		else
			tinfo[i].ti_subdirs = strdup("0");

		/*
		 * get/generate/vett real path of pipe
		 * pipe might not exist, or it might not be in dirname
		 */

		tinfo[i].ti_pipename = my_ini(inidata, sections[i], "pipename");

		if(!IS_NULL(tinfo[i].ti_pipename)) {
			if(strstr(tinfo[i].ti_pipename, "..")) {
				fprintf(stderr, "%s: bad pipename\n", sections[i]);
				exit(EXIT_FAILURE);
			}

			fullpath(tinfo[i].ti_dirname, tinfo[i].ti_pipename, tbuf);

			if(realpath(dirname(tbuf), rbuf) == NULL) {
				fprintf(stderr, "%s: missing or bad pipedir\n", sections[i]);
				exit(EXIT_FAILURE);
			}

			fullpath(rbuf, base(tinfo[i].ti_pipename), tbuf);
			tinfo[i].ti_pipename = strdup(tbuf);
		}

		if(tinfo[i].ti_argc && IS_NULL(tinfo[i].ti_pipename)) {
			fprintf(stderr, "%s: command needs a pipe defined\n", sections[i]);
			exit(EXIT_FAILURE);
		}

		tinfo[i].ti_template = strdup(base(my_ini(inidata, sections[i], "template")));

		if(IS_NULL(tinfo[i].ti_template))
			if(!IS_NULL(tinfo[i].ti_command)) {
				fprintf(stderr, "%s: command requires a template\n", sections[i]);
				exit(EXIT_FAILURE);
			}

		tinfo[i].ti_pcrestr = my_ini(inidata, sections[i], "pcrestr");
		tinfo[i].ti_pcrecmp = pcrecheck(tinfo[i].ti_pcrestr, tinfo[i].ti_pcrecmp);

		if(IS_NULL(tinfo[i].ti_pcrestr) || tinfo[i].ti_pcrecmp == NULL) {
			fprintf(stderr, "%s: missing or bad pcre\n", sections[i]);
			exit(EXIT_FAILURE);
		}

		tinfo[i].ti_filename = malloc(PATH_MAX);
		memset(tinfo[i].ti_filename, '\0', PATH_MAX);

		tinfo[i].ti_pid = (pid_t) 0;
		tinfo[i].ti_uid = verifyuid(my_ini(inidata, sections[i], "uid"));
		tinfo[i].ti_gid = verifygid(my_ini(inidata, sections[i], "gid"));
		tinfo[i].ti_wfd = EOF;
		tinfo[i].ti_loglimit = logsize(my_ini(inidata, sections[i], "loglimit"));
		tinfo[i].ti_diskfree = fabs(atof(my_ini(inidata, sections[i], "diskfree")));
		tinfo[i].ti_inofree = fabs(atof(my_ini(inidata, sections[i], "inofree")));
		tinfo[i].ti_expire = logretention(my_ini(inidata, sections[i], "expire"));
		tinfo[i].ti_retmin = logsize(my_ini(inidata, sections[i], "retmin"));
		tinfo[i].ti_retmax = logsize(my_ini(inidata, sections[i], "retmax"));

		tinfo[i].ti_postcmd = malloc(BUFSIZ);
		memset(tinfo[i].ti_postcmd, '\0', BUFSIZ);
		strlcpy(tinfo[i].ti_postcmd, my_ini(inidata, sections[i], "postcmd"), BUFSIZ);

		if(verbose == TRUE)
			dump_thread_info(&tinfo[i]);
	}

	if(verbose == TRUE)
		exit(EXIT_SUCCESS);

	/* for systemd */

	if(create_pid_file(pidfile) == FALSE) {
		fprintf(stderr, "%s: can't create pidfile or pidfile is in use\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* important: signal handling */

	parent_signals();

	/* setup threads and run */

	workers = (pthread_t *) malloc(nsect * sizeof(*workers));
	dfsmons = (pthread_t *) malloc(nsect * sizeof(*dfsmons));
	expmons = (pthread_t *) malloc(nsect * sizeof(*expmons));
	slmmons = (pthread_t *) malloc(nsect * sizeof(*slmmons));

	/* usleep for systemd journal */

	for(i = 0; i < nsect; i++) {
		/* worker thread */

		if(tinfo[i].ti_argc) {
			usleep((useconds_t) 2000);
			pthread_create(&workers[i], NULL, &workthread, (void *)&tinfo[i]);
			wrk_started = TRUE;
		}

		/* monitor logfile expiration, retention */

		if(tinfo[i].ti_expire || tinfo[i].ti_retmin || tinfo[i].ti_retmax) {
			usleep((useconds_t) 2000);
			pthread_create(&expmons[i], NULL, &expthread, (void *)&tinfo[i]);
			exp_started = TRUE;
		}

		/* monitor filesystem free space */

		if(tinfo[i].ti_diskfree || tinfo[i].ti_inofree) {
			usleep((useconds_t) 2000);
			pthread_create(&dfsmons[i], NULL, &dfsthread, (void *)&tinfo[i]);
			dfs_started = TRUE;
		}

		/* simple log monitor spec must meet several conditions */

		if(tinfo[i].ti_argc == 0 && tinfo[i].ti_loglimit &&
		   !(IS_NULL(tinfo[i].ti_template) && IS_NULL(tinfo[i].ti_pcrestr))) {
			usleep((useconds_t) 2000);
			pthread_create(&slmmons[i], NULL, &slmthread, (void *)&tinfo[i]);
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
	char   *zargv[MAXARGS];
	char    ebuf[BUFSIZ];
	int     i;
	int     n;

	printf("name:     %s\n", ti->ti_section);
	printf("command:  %s\n", ti->ti_command);
	printf("argc:     %d\n", ti->ti_argc);
	printf("path:     %s\n", ti->ti_path);

	printf("argv:     ");
	for(i = 1; i < ti->ti_argc; i++)
		printf("%s ", ti->ti_argv[i]);
	printf("\n");

	printf("dirname:  %s\n", ti->ti_dirname);
	printf("subdirs:  %s\n", ti->ti_subdirs);
	printf("pipename: %s\n", ti->ti_pipename);
	printf("template: %s\n", ti->ti_template);
	printf("pcrestr:  %s\n", ti->ti_pcrestr);
	printf("uid:      %d\n", ti->ti_uid);
	printf("gid:      %d\n", ti->ti_gid);
	printf("loglimit: %ldMiB\n", MiB(ti->ti_loglimit));
	printf("diskfree: %.2Lf\n", ti->ti_diskfree);
	printf("inofree:  %.2Lf\n", ti->ti_inofree);
	printf("expire:   %s\n", convexpire(ti->ti_expire, ebuf));
	printf("retmin:   %d\n", ti->ti_retmin);
	printf("retmax:   %d\n", ti->ti_retmax);

	logname(ti->ti_template, ti->ti_filename);

	if(ti->ti_argc) {
		n = runcmd(ti->ti_argc, ti->ti_argv, zargv);

		printf("execcmd:  ");
		for(i = 0; i < n; i++)
			printf("%s ", zargv[i]);
		printf("> %s\n", ti->ti_filename);		/* show redirect */
	} else
		printf("execcmd:  \n");

	substrstr(ti->ti_postcmd, _HOST_TOK, utsbuf.nodename);
	substrstr(ti->ti_postcmd, _FILE_TOK, ti->ti_filename);
	printf("postcmd:  %s\n\n", ti->ti_postcmd);
}

static int create_pid_file(char *pidfile)
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
