/*
 * logretention.c
 * Return the number of seconds represented in the INI file spec.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <ctype.h>
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

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
