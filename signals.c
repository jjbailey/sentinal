/*
 * signals.c
 * Set signals for handling events.
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"

static void sigparent(int);
static void sigreject(int);

extern struct thread_info tinfo[MAXSECT];			/* our threads */

void parentsignals(void)
{
	/* parent signal setup */

	int     i;

	struct sigaction saparent;						/* catch these */
	struct sigaction sareject;						/* reject these */
	struct sigaction sasigsegv;						/* don't catch or reject */
	struct sigaction sasigstp;						/* for testing */

	memset(&saparent, 0, sizeof(saparent));
	saparent.sa_handler = sigparent;
	sigemptyset(&saparent.sa_mask);
	saparent.sa_flags = SA_RESTART;

	memset(&sareject, 0, sizeof(sareject));
	sareject.sa_handler = sigreject;
	sigemptyset(&sareject.sa_mask);
	sareject.sa_flags = SA_RESTART;

	memset(&sasigsegv, 0, sizeof(sasigsegv));
	sigaction(SIGSEGV, NULL, &sasigsegv);
	memset(&sasigstp, 0, sizeof(sasigstp));
	sigaction(SIGTSTP, NULL, &sasigstp);

	for(i = 1; i < NSIG; i++)
		sigaction(i, &sareject, NULL);

	sigaction(SIGHUP, &saparent, NULL);
	sigaction(SIGINT, &saparent, NULL);
	sigaction(SIGTERM, &saparent, NULL);
	sigaction(SIGCHLD, &saparent, NULL);
	sigaction(SIGSEGV, &sasigsegv, NULL);
	sigaction(SIGTSTP, &sasigstp, NULL);
}

static void sigparent(int sig)
{
	/* parent signal handler */

	int     i;
	int     status;

	signal(sig, sigparent);							/* reset */

	if(sig == SIGINT || sig == SIGTERM)
		_exit(EXIT_SUCCESS);

	if(sig == SIGCHLD) {
		while(waitpid(-1, &status, WNOHANG) > 0)
			;

		return;
	}

	/* mark the threads as signaled */
	/* only workers use ti_wfd */

	for(i = 0; i < MAXSECT; i++)
		if(NOT_NULL(tinfo[i].ti_section))
			tinfo[i].ti_sig = tinfo[i].ti_wfd != EOF ? sig : 0;
}

static void sigreject(int sig)
{
	signal(sig, sigreject);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
