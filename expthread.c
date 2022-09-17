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

#define	SCANRATE		(ONE_MINUTE * 3)			/* default, considers large filesys */
#define	DRYSCAN			5							/* scanrate for dryrun */

void   *expthread(void *arg)
{
	char    ebuf[BUFSIZ];
	extern short dryrun;
	int     interval = dryrun ? DRYSCAN : SCANRATE;
	short   expbysize;								/* consider expire size */
	short   expbytime;								/* consider expire time */
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

	for(;;) {
		sleep(interval);							/* expiry monitor rate */

		/* search for expired files */

		dinfo.di_matches = findfile(ti, TRUE, ti->ti_dirname, &dinfo);

		if(dinfo.di_matches < 1 || (ti->ti_retmin && dinfo.di_matches <= ti->ti_retmin)) {
			/* no matches, or matches below the retention count */
			continue;
		}

		/* match */

		time(&curtime);

		/*
		 * if expiresiz is set, use it, else true
		 * if expire is set, use it, else false
		 */

		expbysize = !ti->ti_expiresiz || dinfo.di_size > ti->ti_expiresiz;
		expbytime = ti->ti_expire && dinfo.di_time + ti->ti_expire < curtime;

		if(ti->ti_retmax && dinfo.di_matches > ti->ti_retmax)
			rmfile(ti, dinfo.di_file, "retmax");

		else if(ti->ti_dirlimit && dinfo.di_bytes > ti->ti_dirlimit)
			rmfile(ti, dinfo.di_file, "dirlimit");

		else if(expbysize && expbytime)
			rmfile(ti, dinfo.di_file, "expire");

		/* oldest file, not expired */
	}

	/* notreached */
	return ((void *)0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
