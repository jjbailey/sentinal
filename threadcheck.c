/*
 * threadcheck.c
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

	if(strcmp(tname, "dfs") == 0)
		fail = !ti->ti_pcrecmp || (ti->ti_diskfree == 0 && ti->ti_inofree == 0);

	else if(strcmp(tname, "exp") == 0)
		fail = !ti->ti_pcrecmp ||
			(ti->ti_expire == 0 && ti->ti_retmin == 0 && ti->ti_retmax == 0);

	else if(strcmp(tname, "slm") == 0)
		fail = IS_NULL(ti->ti_template) || IS_NULL(ti->ti_postcmd) ||
			ti->ti_loglimit == (off_t) 0;

	else if(strcmp(tname, "wrk") == 0)
		fail = ti->ti_argc == 0 || IS_NULL(ti->ti_pipename) || IS_NULL(ti->ti_template);

	/* return true if thread should run, false if not */

	return (fail == TRUE ? FALSE : TRUE);
}

void activethreads(struct thread_info *ti)
{
	int     dfs = threadcheck(ti, "dfs");
	int     exp = threadcheck(ti, "exp");
	int     slm = threadcheck(ti, "slm");
	int     wrk = threadcheck(ti, "wrk");

	fprintf(stderr,
			"threads:  freespace: %s  expiration: %s  logmonitor: %s  worker: %s\n",
			dfs ? "true" : "false", exp ? "true" : "false", slm ? "true" : "false",
			wrk ? "true" : "false");
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
