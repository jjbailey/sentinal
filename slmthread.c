/*
 * slmthread.c
 * Simple log monitor thread.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "sentinal.h"

static int inotify_watch(char *, char *);

#define	SCANRATE		2						/* default monitor rate */
#define	POLLTIMEOUT		(120 * 1000)			/* 2 minutes in milliseconds */

#define	ROTATE(lim,n,sig)	((lim && n > lim) || sig == SIGHUP)
#define	STAT(file,buf)		(stat(file, &buf) == -1 ? -1 : buf.st_size)

void   *slmthread(void *arg)
{
	char    filename[PATH_MAX];
	char    task[TASK_COMM_LEN];
	int     status;
	struct stat stbuf;
	struct thread_info *ti = arg;

	/*
	 * this thread requires:
	 *  - ti_command unset
	 *  - ti_template
	 *  - ti_postcmd
	 *  - ti_loglimit
	 */

	pthread_detach(pthread_self());
	pthread_setname_np(pthread_self(), threadname(ti->ti_section, _SLM_THR, task));

	/* for slm, ti->ti_template is the logname */

	fullpath(ti->ti_dirname, ti->ti_template, filename);
	fprintf(stderr, "%s: monitor file: %s\n", ti->ti_section, filename);

	if(ti->ti_loglimit)
		fprintf(stderr, "%s: monitor file size: %ldMiB\n", ti->ti_section,
				MiB(ti->ti_loglimit));

	ti->ti_sig = 0;								/* reset */

	for(;;) {
		if(ROTATE(ti->ti_loglimit, STAT(filename, stbuf), ti->ti_sig)) {
			/* loglimit or signaled to post-process */

			if((status = postcmd(ti, filename)) != 0) {
				fprintf(stderr, "%s: postcmd exit: %d\n", ti->ti_section, status);
				sleep(5);						/* be nice */
			}

			ti->ti_sig = 0;						/* reset */
		}

		if(inotify_watch(ti->ti_section, filename) == FALSE)
			sleep(SCANRATE);					/* in lieu of inotify */
	}

	/* notreached */
	return ((void *)0);
}

static int inotify_watch(char *section, char *filename)
{
	char    buf[BUFSIZ];
	int     fd;
	int     wd;
	struct inotify_event *event;
	struct pollfd fds[1];

	if(access(filename, R_OK) == -1 || (fd = inotify_init1(IN_NONBLOCK)) == -1)
		return (FALSE);

	wd = inotify_add_watch(fd, filename, IN_MODIFY | IN_MOVE_SELF);

	if(wd == -1) {
		close(fd);
		return (FALSE);
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	/* poll defers our noticing SIGHUP */

	if(poll(fds, 1, POLLTIMEOUT) > 0)
		if(read(fd, buf, BUFSIZ) > 0) {
			event = (struct inotify_event *)buf;

			if(event->mask & IN_MOVE_SELF)
				fprintf(stderr, "%s: %s moved\n", section, filename);
		}

	inotify_rm_watch(fd, wd);
	close(fd);
	return (TRUE);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
