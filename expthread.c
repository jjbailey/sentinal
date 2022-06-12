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

#define	SCANRATE		ONE_MINUTE					/* default monitor rate */

void   *expthread(void *arg)
{
	char    ebuf[BUFSIZ];
	char    task[TASK_COMM_LEN];
	char   *reason;
	int     interval;
	long    matches;
	struct dir_info dinfo;
	struct thread_info *ti = arg;
	time_t  curtime;

	/*
	 * this thread requires:
	 *  - ti_pcrecmp
	 *  - at least one of:
	 *    - ti_dirlimit
	 *    - ti_expire
	 *    - ti_retmin
	 *    - ti_retmax
	 */

	pthread_setname_np(pthread_self(), threadname(ti->ti_section, _EXP_THR, task));

	if(ti->ti_dirlimit)
		fprintf(stderr, "%s: monitor directory: %s for dirlimit %ldMiB\n",
				ti->ti_section, ti->ti_dirname, MiB(ti->ti_dirlimit));

	if(ti->ti_expire)
		fprintf(stderr, "%s: monitor file: %s for expiration %s\n",
				ti->ti_section, ti->ti_pcrestr, convexpire(ti->ti_expire, ebuf));

	if(ti->ti_retmin)
		fprintf(stderr, "%s: monitor file: %s for retmin %d\n",
				ti->ti_section, ti->ti_pcrestr, ti->ti_retmin);

	if(ti->ti_retmax)
		fprintf(stderr, "%s: monitor file: %s for retmax %d\n",
				ti->ti_section, ti->ti_pcrestr, ti->ti_retmax);

	/* monitor expiration times */
	/* faster initial start */

	interval = SCANRATE >> 1;

	for(;;) {
		sleep(interval);							/* expiry monitor rate */
		time(&curtime);

		/* search for expired files */

		matches = findfile(ti, TRUE, ti->ti_dirname, &dinfo);

		if(matches < 1) {							/* no work */
			if(interval < SCANRATE)
				interval = SCANRATE;				/* return to normal */

			continue;
		}

		if(ti->ti_retmin && matches <= ti->ti_retmin) {
			/* match, but below the retention count */
			continue;
		}

		/* match */

		if(ti->ti_retmax && matches > ti->ti_retmax)
			reason = "retmax exceeded";

		else if(ti->ti_dirlimit && dinfo.di_bytes > ti->ti_dirlimit)
			reason = "dirlimit exceeded";

		else if(ti->ti_loglimit && ti->ti_expire) {
			/* when both given, both conditions must be true */

			if(dinfo.di_size < ti->ti_loglimit || dinfo.di_time + ti->ti_expire > curtime)
				continue;
		}

		else if(ti->ti_loglimit && dinfo.di_size >= ti->ti_loglimit)
			reason = "loglimit exceeded";

		else if(ti->ti_expire && dinfo.di_time + ti->ti_expire < curtime)
			reason = "expired";

		else {
			interval = SCANRATE;					/* return to normal */
			continue;
		}

		rmfile(ti, dinfo.di_file, reason);
		interval = 0;								/* 1 sec is too slow for inodes */
	}

	/* notreached */
	return ((void *)0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
