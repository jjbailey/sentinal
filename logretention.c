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
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include "sentinal.h"

#define	TIME_T_MAX	((time_t)((((uintmax_t)1) << (sizeof(time_t) * CHAR_BIT - 1)) - 1))

time_t logretention(char *str)
{
	/* default unit is ONE_DAY */

	char   *endptr;
	long long n;
	time_t  mult = ONE_DAY;

	if(IS_NULL(str))
		return (0);

	n = strtoll(str, &endptr, 10);
	if(endptr == str)
		return (0);

	if(n < 0) {
		if(n == LLONG_MIN) {
			fprintf(stderr, "%s: retention out of range, ignored\n", str);
			return (0);
		}

		n = -n;
	}

	if(*endptr) {
		switch (*endptr) {

		case 'm':
			mult = ONE_MINUTE;
			break;

		case 'H':
		case 'h':
			mult = ONE_HOUR;
			break;

		case 'D':
		case 'd':
			mult = ONE_DAY;
			break;

		case 'W':
		case 'w':
			mult = ONE_WEEK;
			break;

		case 'M':
			mult = ONE_MONTH;
			break;

		case 'Y':
		case 'y':
			mult = ONE_YEAR;
			break;
		}
	}

	if(n > (long long)(TIME_T_MAX / mult)) {
		fprintf(stderr, "%s: retention out of range, ignored\n", str);
		return (0);
	}

	return ((time_t) n * mult);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
