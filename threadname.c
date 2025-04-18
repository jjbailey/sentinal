/*
 * threadname.c
 * Name the thread for use with shell utils.
 *
 * Copyright (c) 2021-2025 jjb
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

	char    delbuf[BUFSIZ];							/* delete string */
	char    secbuf[BUFSIZ];							/* for copy of ti->ti_section */
	size_t  strdel(char *, const char *, char *, size_t);

	/* remove the thread_type if it's already in the name */
	snprintf(delbuf, BUFSIZ, "_%s", tname);
	strdel(secbuf, ti->ti_section, delbuf, BUFSIZ);

	if((ti->ti_task = (char *)malloc(TASK_COMM_LEN)) == NULL)
		return (NULL);

	/* add tname to the section name and call it the task name */
	snprintf(ti->ti_task, TASK_COMM_LEN, "%.11s_%.3s", secbuf, tname);
	return (ti->ti_task);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
