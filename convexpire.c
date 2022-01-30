/*
 * convexpire.c
 * Human-readable units.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include "sentinal.h"

char   *convexpire(int expire, char *str)
{
	if(expire >= ONE_YEAR)
		snprintf(str, BUFSIZ, "%dM", (int)(expire / ONE_MONTH));
	else if(expire >= ONE_MONTH)
		snprintf(str, BUFSIZ, "%dD", (int)(expire / ONE_DAY));
	else if(expire >= ONE_WEEK)
		snprintf(str, BUFSIZ, "%dD", (int)(expire / ONE_DAY));
	else if(expire >= ONE_DAY)
		snprintf(str, BUFSIZ, "%dH", (int)(expire / ONE_HOUR));
	else
		/* >= ONE_HOUR */
		snprintf(str, BUFSIZ, "%dm", (int)(expire / ONE_MINUTE));

	return (str);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
