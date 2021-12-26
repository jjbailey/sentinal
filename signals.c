/*
 * signals.c
 * Set signals for handing events.
 *
 * Copyright (c) 2021 jjb
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
#include <stdlib.h>
#include <unistd.h>
#include "sentinal.h"

extern struct thread_info tinfo[MAXSECT];

void parent_signals(void)
{
	/* parent signal setup */

	int     i;

	struct sigaction saparent;
	struct sigaction sareject;

	saparent.sa_handler = sigparent;
	sigemptyset(&saparent.sa_mask);
	saparent.sa_flags = SA_RESETHAND;

	sareject.sa_handler = sigreject;
	sigemptyset(&sareject.sa_mask);
	sareject.sa_flags = SA_RESETHAND;

	for(i = 1; i < NSIG; i++)
		sigaction(i, &sareject, NULL);

	sigaction(SIGHUP, &saparent, NULL);
	sigaction(SIGINT, &saparent, NULL);
	sigaction(SIGTERM, &saparent, NULL);
	sigaction(SIGCHLD, &saparent, NULL);
}

void sigparent(int sig)
{
	/* parent signal handler */

	int     i;
	int     status;
	pid_t   pid;

	signal(sig, sigparent);

#if 0
	fprintf(stderr, "sigparent caught signal %d\n", sig);
#endif

	if(sig == SIGINT || sig == SIGTERM)
		exit(EXIT_SUCCESS);

	if(sig == SIGCHLD) {
		while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0);

		return;
	}

	/* marked the threads as signaled */

	for(i = 0; i < MAXSECT; i++)
		if(!IS_NULL(tinfo[i].ti_section))
			tinfo[i].ti_sig = tinfo[i].ti_wfd != EOF ? sig : 0;
}

void sigreject(int sig)
{
	signal(sig, sigreject);

#if 0
	fprintf(stderr, "sigreject caught signal %d\n", sig);
#endif
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
