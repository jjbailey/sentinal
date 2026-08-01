/*
 * convexpire.c
 * Human-readable units.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include "sentinal.h"

#define	EVEN_UNIT(e,u)	(e % u == 0)

char   *convexpire(time_t expire, char *str)
{
	/* returns the largest non-fraction unit possible */

	if(expire >= ONE_YEAR && EVEN_UNIT(expire, ONE_YEAR))
		snprintf(str, BUFSIZ, "%lldY", (long long)(expire / ONE_YEAR));

	else if(expire >= ONE_MONTH && EVEN_UNIT(expire, ONE_MONTH))
		snprintf(str, BUFSIZ, "%lldM", (long long)(expire / ONE_MONTH));

	else if(expire >= ONE_WEEK && EVEN_UNIT(expire, ONE_WEEK))
		snprintf(str, BUFSIZ, "%lldW", (long long)(expire / ONE_WEEK));

	else if(expire >= ONE_DAY && EVEN_UNIT(expire, ONE_DAY))
		snprintf(str, BUFSIZ, "%lldD", (long long)(expire / ONE_DAY));

	else if(expire >= ONE_HOUR && EVEN_UNIT(expire, ONE_HOUR))
		snprintf(str, BUFSIZ, "%lldH", (long long)(expire / ONE_HOUR));

	else
		snprintf(str, BUFSIZ, "%lldm", (long long)(expire / ONE_MINUTE));

	return (str);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
