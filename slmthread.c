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
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"

#define	SCANRATE		2							/* default monitor rate */
#define	POLLTIMEOUT		(120 * 1000)				/* 2 minutes in milliseconds */

#define	ROTATE(lim, n, sig)	((lim && n > lim) || (sig) == SIGHUP)
#define	STAT(file, bufptr)	(stat(file, (bufptr)) == -1 ? -1 : (bufptr)->st_size)

static bool inotify_watch(char *, char *);
static void safe_reset_sig(volatile sig_atomic_t * sig_var);

void   *slmthread(void *arg)
{
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

	char    filename[PATH_MAX];
	int     status;
	struct stat stbuf;
	struct thread_info *ti = arg;

	int     setname_err = pthread_setname_np(pthread_self(), threadname(ti, _SLM_THR));

	if(setname_err)
		fprintf(stderr, "%s: pthread_setname_np() failed: %s\n", ti->ti_section,
				strerror(setname_err));

	/* for slm, ti->ti_template is the logname */

	if(fullpath(ti->ti_dirname, ti->ti_template, filename) < 0) {
		fprintf(stderr, "%s: error building log file path\n", ti->ti_section);
		return ((void *)0);
	}

	if(ti->ti_rotatesiz)
		fprintf(stderr, "%s: monitor file: %s for size %s\n",
				ti->ti_section, filename, ti->ti_rotatestr);

	safe_reset_sig(&ti->ti_sig);					/* reset */

	for(;;) {
		if(ROTATE(ti->ti_rotatesiz, STAT(filename, &stbuf), ti->ti_sig)) {
			/* ti->ti_rotatesiz or signaled to post-process */

			if((status = postcmd(ti, filename)) != 0) {
				fprintf(stderr, "%s: postcmd exit: %d\n", ti->ti_section, status);
				sleep(5);							/* be nice */
			}

			safe_reset_sig(&ti->ti_sig);			/* reset */
		}

		if(inotify_watch(ti->ti_section, filename) == false)
			sleep(SCANRATE);						/* in lieu of reliable inotify */
	}

	/* notreached */
	return ((void *)0);
}

static bool inotify_watch(char *section, char *filename)
{
	bool    ret = false;
	char    buf[BUFSIZ] __attribute__((aligned(__alignof__(struct inotify_event))));
	int     fd = -1;
	int     wd = -1;
	ssize_t len;
	struct pollfd fds[1];

	if(access(filename, R_OK) == -1)
		return (false);

	if((fd = inotify_init1(IN_NONBLOCK)) == -1) {
		fprintf(stderr, "%s: inotify_init1 failed: %s\n", section, strerror(errno));
		return (false);
	}

	wd = inotify_add_watch(fd, filename, IN_MODIFY | IN_MOVE_SELF | IN_DELETE_SELF);
	if(wd == -1) {
		fprintf(stderr, "%s: inotify_add_watch failed for %s: %s\n",
				section, filename, strerror(errno));

		goto cleanup;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	int     poll_ret = poll(fds, 1, POLLTIMEOUT);
	if(poll_ret > 0) {
		while((len = read(fd, buf, sizeof(buf))) > 0) {
			char   *ptr;

			for(ptr = buf; ptr < buf + len;) {
				struct inotify_event *event = (struct inotify_event *)ptr;

				if(event->mask & IN_MOVE_SELF)
					fprintf(stderr, "%s: %s moved\n", section, filename);
				if(event->mask & IN_DELETE_SELF)
					fprintf(stderr, "%s: %s deleted\n", section, filename);

				ptr += sizeof(struct inotify_event) + event->len;
			}
		}

		if(len < 0 && errno != EAGAIN)
			fprintf(stderr, "%s: inotify read error: %s\n", section, strerror(errno));
	} else if(poll_ret < 0) {
		fprintf(stderr, "%s: poll error: %s\n", section, strerror(errno));
		goto cleanup;
	}

	ret = true;

  cleanup:
	if(wd != -1 && inotify_rm_watch(fd, wd) == -1)
		fprintf(stderr, "%s: inotify_rm_watch failed: %s\n", section, strerror(errno));

	if(fd != -1 && close(fd) == -1)
		fprintf(stderr, "%s: close failed: %s\n", section, strerror(errno));

	return (ret);
}

static void safe_reset_sig(volatile sig_atomic_t *sig_var)
{
	*sig_var = 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
