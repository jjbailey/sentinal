/*
 * signals.c
 * Set signals for handling events.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"

static void sigparent(int);
static void sigreject(int);

static volatile sig_atomic_t parent_sig;
static volatile sig_atomic_t parent_sig_gen;

void parentsignals(void)
{
	/* parent signal setup */

	int     i;

	struct sigaction saparent;						/* catch these */
	struct sigaction sareject;						/* reject these */
	struct sigaction sasigchld;						/* let threads reap their own children */
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

	memset(&sasigchld, 0, sizeof(sasigchld));
	sasigchld.sa_handler = SIG_DFL;
	sigemptyset(&sasigchld.sa_mask);

	memset(&sasigsegv, 0, sizeof(sasigsegv));
	sigaction(SIGSEGV, NULL, &sasigsegv);
	memset(&sasigstp, 0, sizeof(sasigstp));
	sigaction(SIGTSTP, NULL, &sasigstp);

	for(i = 1; i < NSIG; i++)
		sigaction(i, &sareject, NULL);

	sigaction(SIGHUP, &saparent, NULL);
	sigaction(SIGINT, &saparent, NULL);
	sigaction(SIGTERM, &saparent, NULL);
	sigaction(SIGCHLD, &sasigchld, NULL);
	sigaction(SIGSEGV, &sasigsegv, NULL);
	sigaction(SIGTSTP, &sasigstp, NULL);
}

int parentsigcheck(sig_atomic_t *seen)
{
	if(*seen == parent_sig_gen)
		return (0);

	*seen = parent_sig_gen;
	return (parent_sig);
}

static void sigparent(int sig)
{
	/* parent signal handler */

	if(sig == SIGINT || sig == SIGTERM)
		_exit(EXIT_SUCCESS);

	parent_sig = sig;
	parent_sig_gen++;
}

static void sigreject(int sig)
{
	(void)sig;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
