/*
 * sentinalpipe.c
 * The purpose of this program is to prevent SIGPIPE from being sent
 * to processes writing to pipes.  Keep pipes given in the INI file,
 * if they exist, always open for reading.
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"
#include "ini.h"

struct pipeinfo {
	char   *pi_pipename;							/* FIFO name */
	int     pi_fd;									/* fd from open(2) */
};

struct pipeinfo pipelist[MAXSECT];					/* list of pipes and open fds */

static void help(char *);
static void pipeopen(int);
static void sigcatch(int);
static void systemd_signals(void);

static struct option long_options[] = {
	{ "ini-file", required_argument, NULL, 'f' },
	{ "version", no_argument, NULL, 'V' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char *argv[])
{
	char    filename[PATH_MAX];						/* full pathname */
	char    inifile[PATH_MAX];						/* ini file name */
	char   *my_ini(ini_t *, char *, char *);
	char   *myname;
	char   *p1, *p2;
	char    rbuf[PATH_MAX];
	char   *sections[MAXSECT];						/* section names */
	char    tbuf[PATH_MAX];
	ini_t  *inidata;								/* loaded ini data */
	int     c;
	int     get_sections(ini_t *, int, char **);
	int     i;
	int     index = 0;
	int     nsect;									/* number of sections found */

	myname = base(argv[0]);

	*inifile = '\0';

	while(1) {
		c = getopt_long(argc, argv, "f:Vh?", long_options, &index);

		if(c == -1)									/* end of options */
			break;

		switch (c) {

		case 'V':									/* print version */
			fprintf(stdout, "%s: version %s\n", myname, VERSION_STRING);
			exit(EXIT_SUCCESS);

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

	if((inidata = ini_load(inifile)) == NULL) {
		fprintf(stderr, "%s: can't load %s\n", myname, inifile);
		exit(EXIT_FAILURE);
	}

	if((nsect = get_sections(inidata, MAXSECT, sections)) == 0) {
		fprintf(stderr, "%s: nothing to do\n", myname);
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
		pipelist[i].pi_pipename = strndup(filename, PATH_MAX);
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

	int     savefd = pipelist[i].pi_fd;				/* save open fd */

	if(IS_NULL(pipelist[i].pi_pipename))			/* INI file error */
		return;

	if(access(pipelist[i].pi_pipename, R_OK) == -1) {
		/* not yet created or gone missing */

		if(savefd != EOF)
			close(savefd);

		pipelist[i].pi_fd = EOF;
	} else {										/* open a second fd */
		pipelist[i].pi_fd = open(pipelist[i].pi_pipename, O_RDONLY | O_NONBLOCK);

		if(savefd != EOF)							/* close saved fd */
			close(savefd);
	}
}

static void systemd_signals(void)
{
	/* catch signals sent by systemctl commands */

	int     i;
	struct sigaction sacatch;

	memset(&sacatch, 0, sizeof(sacatch));
	sacatch.sa_handler = sigcatch;
	sigemptyset(&sacatch.sa_mask);
	sacatch.sa_flags = (int)SA_RESETHAND;

	for(i = 1; i < NSIG; i++)
		sigaction(i, &sacatch, NULL);
}

static void sigcatch(int sig)
{
	/*
	 * keep pipes open for application restarts, e.g., if someone runs:
	 * systemctl restart sentinal sentinalpipe
	 * give sentinal time to restart
	 */

	signal(sig, sigcatch);

	if(sig == SIGHUP || sig == SIGTERM) {			/* systemd reload/restart */
		signal(sig, SIG_IGN);
		fprintf(stderr, "exiting in 10s...\n");
		sleep(10);									/* give sentinal 10s */
		exit(EXIT_SUCCESS);
	}
}

static void help(char *prog)
{
	char   *p = base(prog);

	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "%s -f|--ini-file ini-file\n\n", p);
	fprintf(stderr, "Print the program version, exit\n");
	fprintf(stderr, "%s -V|--version\n\n", p);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
