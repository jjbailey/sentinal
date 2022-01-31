/*
 * expthread.c
 * log expiration thread
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
#include <pthread.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"

#define	SCANRATE	15

void   *expthread(void *arg)
{
	char    ebuf[BUFSIZ];
	char    oldfile[PATH_MAX];
	char    task[TASK_COMM_LEN];
	char   *reason;
	int     fc;
	int     interval = SCANRATE;
	struct thread_info *ti = arg;
	time_t  curtime;
	time_t  oldtime;

	if(!ti->ti_pcrecmp) {
		/* should not be here */
		return ((void *)0);
	}

	if(ti->ti_expire == 0 && ti->ti_retmin == 0 && ti->ti_retmax == 0) {
		/* should not be here */
		return ((void *)0);
	}

	pthread_detach(pthread_self());
	pthread_setname_np(pthread_self(), threadname(ti->ti_section, "exp", task));

	if(ti->ti_expire)
		fprintf(stderr, "%s: monitor log expiration: %s\n", ti->ti_section,
				convexpire(ti->ti_expire, ebuf));

	if(ti->ti_retmin)
		fprintf(stderr, "%s: monitor log min retention: %d\n", ti->ti_section,
				ti->ti_retmin);

	if(ti->ti_retmax)
		fprintf(stderr, "%s: monitor log max retention: %d\n", ti->ti_section,
				ti->ti_retmax);

	/* if ti->ti_expire is zero, we're only concerned with retmin and/or retmax */

	for(;;) {
		sleep(interval);						/* expiry monitor rate */

		*oldfile = oldtime = 0;

		/* full path to oldest file, its time, and the number of files found */
		fc = oldestfile(ti, TRUE, ti->ti_dirname, oldfile, &oldtime);

#if 0
		fprintf(stderr, "%s: filecount = %d\n", ti->ti_section, fc);
#endif

		if(time(&curtime) - oldtime < ONE_MINUTE) {
			/* wait for another thread to remove a file older than this one */
			continue;
		}

		if(ti->ti_retmax && *oldfile && fc > ti->ti_retmax) {
			/* too many files */
			reason = "remove";
		} else if(ti->ti_expire && *oldfile && oldtime + ti->ti_expire < curtime) {
			/* retention time exceeded */
			reason = "expire";
		} else {
			/* desired state */
			interval = SCANRATE;				/* return to normal */
			continue;
		}

		fprintf(stderr, "%s: %s %s\n", ti->ti_section, reason, base(oldfile));
		remove(oldfile);
		interval = 0;							/* 1 sec is too slow for inodes */
	}

	/* notreached */
	return ((void *)0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
