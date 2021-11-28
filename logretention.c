/*
 * logretention.c
 * Return the number of seconds represented in the INI file spec.
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "sentinal.h"

int logretention(char *str)
{
	/* default unit is ONE_DAY */

	char   *p;
	int     n;

	if(IS_NULL(str))
		return (0);

	n = abs(atoi(str));

	for(p = str; *p; p++)
		if(isalpha(*p))
			break;

	if(IS_NULL(p))
		return (n * ONE_DAY);

	switch (*p) {

	case 'm':
		return (n * ONE_MINUTE);

	case 'H':
	case 'h':
		return (n * ONE_HOUR);

	case 'D':
	case 'd':
		return (n * ONE_DAY);

	case 'W':
	case 'w':
		return (n * ONE_WEEK);

	case 'M':
		return (n * ONE_MONTH);

	case 'Y':
	case 'y':
		return (n * ONE_YEAR);
	}

	return (n * ONE_DAY);
}

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
