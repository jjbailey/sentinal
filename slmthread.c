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
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "sentinal.h"

#define	SCANRATE	2							/* a proc can write a lot in 2 secs */

#define	ROTATE(lim,n,sig)	((lim && n > lim) || sig == SIGHUP)
#define	STAT(file,buf)	(stat(file, &buf) == -1 ? -1 : buf.st_size)

void   *slmthread(void *arg)
{
	char    filename[PATH_MAX];
	char    task[TASK_COMM_LEN];
	int     status;
	struct stat stbuf;
	struct thread_info *ti = arg;

	if(threadcheck(ti, "slm") == FALSE)			/* should not be here */
		return ((void *)0);

	pthread_detach(pthread_self());
	pthread_setname_np(pthread_self(), threadname(ti->ti_section, "slm", task));

	/* for slm, ti->ti_template is the logname */

	fullpath(ti->ti_dirname, ti->ti_template, filename);
	fprintf(stderr, "%s: monitor log: %s\n", ti->ti_section, filename);

	if(ti->ti_loglimit)
		fprintf(stderr, "%s: monitor log size: %ldMiB\n", ti->ti_section,
				MiB(ti->ti_loglimit));

	ti->ti_sig = 0;								/* reset */

	for(;;) {
		sleep(SCANRATE);						/* stat scan rate */

		if(ROTATE(ti->ti_loglimit, STAT(filename, stbuf), ti->ti_sig)) {
			/* loglimit or signaled to post-process */

			if((status = postcmd(ti, filename)) != 0) {
				fprintf(stderr, "%s: postcmd exit: %d\n", ti->ti_section, status);
				sleep(5);						/* be nice */
			}

			ti->ti_sig = 0;						/* reset */
		}
	}

	/* notreached */
	return ((void *)0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
