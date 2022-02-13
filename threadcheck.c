/*
 * threadcheck.c
 * Considering the config data given, test if threads should run.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include "sentinal.h"

int threadcheck(struct thread_info *ti, char *tname)
{
	short   fail = TRUE;

	if(strcmp(tname, "dfs") == 0)				/* filesystem free space */
		fail = !ti->ti_pcrecmp || (ti->ti_diskfree == 0 && ti->ti_inofree == 0);

	else if(strcmp(tname, "exp") == 0)			/* logfile expiration, retention */
		fail = !ti->ti_pcrecmp ||
			(ti->ti_expire == 0 && ti->ti_retmin == 0 && ti->ti_retmax == 0);

	else if(strcmp(tname, "slm") == 0)			/* simple log monitor */
		fail = !IS_NULL(ti->ti_command) ||
			IS_NULL(ti->ti_template) || IS_NULL(ti->ti_postcmd) ||
			ti->ti_loglimit == (off_t) 0;

	else if(strcmp(tname, "wrk") == 0)			/* worker (log ingestion) thread */
		fail = ti->ti_argc == 0 || IS_NULL(ti->ti_pipename) || IS_NULL(ti->ti_template);

	/* return true if thread should run, false if not */

	return (fail == TRUE ? FALSE : TRUE);
}

void activethreads(struct thread_info *ti)
{
	char   *f = "false";
	char   *t = "true";

	fprintf(stderr, "threads:  free: %s  expire: %s  slm: %s  worker: %s\n",
			threadcheck(ti, "dfs") ? t : f, threadcheck(ti, "exp") ? t : f,
			threadcheck(ti, "slm") ? t : f, threadcheck(ti, "wrk") ? t : f);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
