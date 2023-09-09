/*
 * threadname.c
 * Name the thread for monitoring with shell utils.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include "sentinal.h"

char   *threadname(struct thread_info *ti, char *tname)
{
	/* strict format: <section name>_<thread name> */

	ti->ti_task = malloc(TASK_COMM_LEN);
	snprintf(ti->ti_task, TASK_COMM_LEN, "%.11s_%.3s", ti->ti_section, tname);
	return (ti->ti_task);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
