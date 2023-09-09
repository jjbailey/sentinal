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

#define	EVEN_UNIT(e,u)	(e % u == 0)

char   *convexpire(int expire, char *str)
{
	/* returns the largest non-fraction unit possible */

	if(expire >= ONE_YEAR && EVEN_UNIT(expire, ONE_YEAR))
		snprintf(str, BUFSIZ, "%dY", (int)(expire / ONE_YEAR));

	else if(expire >= ONE_MONTH && EVEN_UNIT(expire, ONE_MONTH))
		snprintf(str, BUFSIZ, "%dM", (int)(expire / ONE_MONTH));

	else if(expire >= ONE_WEEK && EVEN_UNIT(expire, ONE_WEEK))
		snprintf(str, BUFSIZ, "%dW", (int)(expire / ONE_WEEK));

	else if(expire >= ONE_DAY && EVEN_UNIT(expire, ONE_DAY))
		snprintf(str, BUFSIZ, "%dD", (int)(expire / ONE_DAY));

	else if(expire >= ONE_HOUR && EVEN_UNIT(expire, ONE_HOUR))
		snprintf(str, BUFSIZ, "%dH", (int)(expire / ONE_HOUR));

	else
		snprintf(str, BUFSIZ, "%dm", (int)(expire / ONE_MINUTE));

	return (str);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
