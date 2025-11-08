/*
 * slmthread.c
 * Simple log monitor thread.
 *
 * Copyright (c) 2021-2025 jjb
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
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "sentinal.h"

#define	SCANRATE		2							/* default monitor rate */
#define	POLLTIMEOUT		(120 * 1000)				/* 2 minutes in milliseconds */

#define	ROTATE(lim,n,sig)	((lim && n > lim) || sig == SIGHUP)
#define	STAT(file,buf)		(stat(file, &buf) == -1 ? -1 : buf.st_size)

static bool inotify_watch(char *, char *);

void   *slmthread(void *arg)
{
	char    filename[PATH_MAX];						/* full pathname */
	int     status;									/* postcmd child exit */
	struct stat stbuf;								/* file status */
	struct thread_info *ti = arg;					/* thread settings */

	/*
	 * this thread requires:
	 *  - ti_command unset
	 *  - ti_template
	 *  - ti_postcmd
	 *  - ti_rotatesiz
	 *
	 * optional, likely required by use case:
	 *  - ti_uid
	 *  - ti_gid
	 *  - ti_truncate
	 */

	pthread_setname_np(pthread_self(), threadname(ti, _SLM_THR));

	/* for slm, ti->ti_template is the logname */

	fullpath(ti->ti_dirname, ti->ti_template, filename);

	if(ti->ti_rotatesiz)							/* mandatory, check anyway */
		fprintf(stderr, "%s: monitor file: %s for size %s\n",
				ti->ti_section, filename, ti->ti_rotatestr);

	ti->ti_sig = 0;									/* reset */

	for(;;) {
		if(ROTATE(ti->ti_rotatesiz, STAT(filename, stbuf), ti->ti_sig)) {
			/* ti->ti_rotatesiz or signaled to post-process */

			if((status = postcmd(ti, filename)) != 0) {
				fprintf(stderr, "%s: postcmd exit: %d\n", ti->ti_section, status);
				sleep(5);							/* be nice */
			}

			ti->ti_sig = 0;							/* reset */
		}

		if(inotify_watch(ti->ti_section, filename) == false)
			sleep(SCANRATE);						/* in lieu of inotify */
	}

	/* notreached */
	return ((void *)0);
}

static bool inotify_watch(char *section, char *filename)
{
	char    buf[BUFSIZ] __attribute__((aligned(__alignof__(struct inotify_event))));
	int     fd = -1;
	int     wd = -1;
	int     watchflags = IN_MODIFY | IN_MOVE_SELF | IN_DELETE_SELF;
	struct pollfd fds[1];

	if(access(filename, R_OK) == -1 || (fd = inotify_init1(IN_NONBLOCK)) == -1)
		return (false);

	if((wd = inotify_add_watch(fd, filename, watchflags)) == -1) {
		close(fd);
		return (false);
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	/* poll defers our noticing SIGHUP */

	if(poll(fds, 1, POLLTIMEOUT) > 0)
		if(read(fd, buf, BUFSIZ) > 0) {
			struct inotify_event *event = (struct inotify_event *)buf;

			if(event->mask & IN_MOVE_SELF)
				fprintf(stderr, "%s: %s moved\n", section, filename);
		}

	inotify_rm_watch(fd, wd);
	close(fd);
	return (true);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
