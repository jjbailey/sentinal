/*
 * sentinalpipe.c
 * sentinalpipe: The purpose of this program is to prevent SIGPIPE from being
 * sent to processes writing to pipes.  Keep pipes given in the INI file, if
 * they exist, always open for reading.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>								/* for realpath() */
#include <signal.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"
#include "ini.h"

struct pipeinfo {
	char   *pi_pipename;						/* FIFO name */
	int     pi_fd;								/* fd from open(2) */
};

struct pipeinfo pipelist[MAXSECT];				/* list of pipes and open fds */

static void help(char *);
static void pipeopen(int);
static void sigcatch(int);
static void systemd_signals(void);

/* iniget.c functions */
char   *my_ini(ini_t *, char *, char *);
int     get_sections(ini_t *, int, char **);

void    version(char *, FILE *);

int main(int argc, char *argv[])
{
	char    filename[PATH_MAX];
	char    inifile[PATH_MAX];
	char    rbuf[PATH_MAX];
	char    tbuf[PATH_MAX];
	char   *p1, *p2;
	char   *sections[MAXSECT];
	ini_t  *inidata;
	int     i;
	int     nsect;
	int     opt;

	*inifile = '\0';

	while((opt = getopt(argc, argv, "f:V?")) != -1) {
		switch (opt) {

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

	if((inidata = ini_load(inifile)) == NULL) {
		fprintf(stderr, "%s: can't load %s\n", argv[0], inifile);
		exit(EXIT_FAILURE);
	}

	if((nsect = get_sections(inidata, MAXSECT, sections)) == 0) {
		fprintf(stderr, "%s: nothing to do\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < nsect; i++) {
		p1 = my_ini(inidata, sections[i], "dirname");
		p2 = my_ini(inidata, sections[i], "pipename");

		if(IS_NULL(p1) || IS_NULL(p2)) {
			/*
			 * dirname missing: error
			 * pipename missing: error or section has no worker
			 * either way, skip
			 */

			continue;
		}

		fullpath(p1, p2, tbuf);

		if(realpath(dirname(tbuf), rbuf) == NULL) {
			/* ignore broken path */
			fprintf(stderr, "%s: missing or bad pipedir\n", rbuf);
			continue;
		}

		fullpath(rbuf, base(p2), filename);
		pipelist[i].pi_pipename = strdup(filename);
		pipelist[i].pi_fd = EOF;

		fprintf(stderr, "monitor %s\n", pipelist[i].pi_pipename);
	}

	systemd_signals();

	for(;;) {
		for(i = 0; i < nsect; i++)
			pipeopen(i);

		sleep(ONE_MINUTE);
	}

	/* notreached */
	exit(EXIT_SUCCESS);
}

static void pipeopen(int i)
{
	/* keep pipes open by opening a new fd before closing the old fd */

	int     savefd = pipelist[i].pi_fd;			/* save open fd */

	if(IS_NULL(pipelist[i].pi_pipename))		/* INI file error */
		return;

	if(access(pipelist[i].pi_pipename, R_OK) == -1) {
		/* not yet created or gone missing */

		if(savefd != EOF)
			close(savefd);

		pipelist[i].pi_fd = EOF;
	} else {									/* open a second fd */
		pipelist[i].pi_fd = open(pipelist[i].pi_pipename, O_RDONLY | O_NONBLOCK);

		if(savefd != EOF)						/* close saved fd */
			close(savefd);
	}
}

static void systemd_signals(void)
{
	/* catch signals sent by systemctl commands */

	int     i;
	struct sigaction sacatch;

	sacatch.sa_handler = sigcatch;
	sigemptyset(&sacatch.sa_mask);
	sacatch.sa_flags = SA_RESETHAND;

	for(i = 1; i < NSIG; i++)
		sigaction(i, &sacatch, NULL);
}

static void sigcatch(int sig)
{
	/*
	 * keep pipes open for application restarts, e.g., if someone runs:
	 * systemctl restart sentinal sentinalpipe
	 * give the application time to restart
	 */

	signal(sig, sigcatch);

#if 0
	fprintf(stderr, "sigcatch caught signal %d\n", sig);
#endif

	if(sig == SIGHUP || sig == SIGTERM) {		/* systemd reload/restart */
		signal(sig, SIG_IGN);
		fprintf(stderr, "exiting in 10s...\n");
		sleep(10);								/* give sentinal 10s to restart */
		exit(EXIT_SUCCESS);
	}
}

static void help(char *prog)
{
	char   *p = base(prog);

	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "%s -f <inifile>\n\n", p);
	fprintf(stderr, "Print the program version, exit\n");
	fprintf(stderr, "%s -V\n\n", p);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
