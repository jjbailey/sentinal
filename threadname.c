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

char   *threadname(char *section, char *tname, char *nbuf)
{
	/* strict format: <section name>_<thread name> */

	snprintf(nbuf, TASK_COMM_LEN, "%.11s_%.3s", section, tname);
	return (nbuf);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
