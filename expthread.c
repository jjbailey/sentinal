/*
 * expthread.c
 * Log expiration thread.
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

#define	SCANRATE		ONE_MINUTE				/* default monitor rate */

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

	/*
	 * this thread requires:
	 *  - ti_pcrecmp
	 *  - ti_expire or ti_retmin or ti_retmax
	 */

	pthread_detach(pthread_self());
	pthread_setname_np(pthread_self(), threadname(ti->ti_section, _EXP_THR, task));

	if(ti->ti_expire)
		fprintf(stderr, "%s: monitor file: %s for expiration %s\n",
				ti->ti_section, ti->ti_pcrestr, convexpire(ti->ti_expire, ebuf));

	if(ti->ti_retmin)
		fprintf(stderr, "%s: monitor file: %s for retmin %d\n",
				ti->ti_section, ti->ti_pcrestr, ti->ti_retmin);

	if(ti->ti_retmax)
		fprintf(stderr, "%s: monitor file: %s for retmax %d\n",
				ti->ti_section, ti->ti_pcrestr, ti->ti_retmax);

	for(;;) {
		sleep(interval);						/* expiry monitor rate */

		/* full path to oldest file, its time, and the number of files found */
		fc = oldestfile(ti, TRUE, ti->ti_dirname, oldfile, &oldtime);

		if(!fc) {								/* no work */
			if(interval < SCANRATE)
				interval = SCANRATE;			/* return to normal */

			continue;
		}

		if(ti->ti_retmin && fc <= ti->ti_retmin) {
			/* keep */
			continue;
		}

		if(time(&curtime) - oldtime < ONE_MINUTE) {
			/* wait for another thread to remove a file older than this one */
			interval = ONE_MINUTE >> 1;			/* intermediate sleep state */
			continue;
		}

		if(ti->ti_retmax && fc > ti->ti_retmax) {
			/* too many files */
			reason = "remove";
		} else if(ti->ti_expire && oldtime + ti->ti_expire < curtime) {
			/* retention time exceeded */
			reason = "expire";
		} else {
			/* desired state */
			interval = SCANRATE;				/* return to normal */
			continue;
		}

		if(*ti->ti_terse == 'f')
			fprintf(stderr, "%s: %s %s\n", ti->ti_section, reason, base(oldfile));

		remove(oldfile);
		interval = 0;							/* 1 sec is too slow for inodes */
	}

	/* notreached */
	return ((void *)0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
