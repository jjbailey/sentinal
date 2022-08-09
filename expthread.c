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

#define	SCANRATE		(ONE_MINUTE * 5)			/* default monitor rate */

void   *expthread(void *arg)
{
	char    ebuf[BUFSIZ];
	char   *reason;
	extern short dryrun;
	int     interval;
	short   expsizflag;
	struct dir_info dinfo;
	struct thread_info *ti = arg;
	time_t  curtime;

	/*
	 * this thread requires:
	 *  - ti_pcrecmp
	 *  - at least one of:
	 *    - ti_dirlimit
	 *    - ti_expire
	 *    - ti_retmax
	 */

	pthread_setname_np(pthread_self(), threadname(ti, _EXP_THR));

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

	interval = dryrun ? 1 : SCANRATE >> 2;			/* faster initial start */

	for(;;) {
		sleep(interval);							/* expiry monitor rate */

		/* search for expired files */

		dinfo.di_matches = findfile(ti, TRUE, ti->ti_dirname, &dinfo);

		if(dinfo.di_matches < 1) {					/* no work */
			if(interval < SCANRATE)
				interval = SCANRATE;				/* return to normal */

			continue;
		}

		if(ti->ti_retmin && dinfo.di_matches <= ti->ti_retmin) {
			/* match, but below the retention count */
			continue;
		}

		/* match */

		time(&curtime);

		/* if expiresiz is set, use it, else true */

		expsizflag = !ti->ti_expiresiz || dinfo.di_size > ti->ti_expiresiz;

		if(ti->ti_retmax && dinfo.di_matches > ti->ti_retmax)
			reason = "retmax";

		else if(ti->ti_dirlimit && dinfo.di_bytes > ti->ti_dirlimit)
			reason = "dirlimit";

		else if(expsizflag && dinfo.di_time + ti->ti_expire < curtime)
			reason = "expire";

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
