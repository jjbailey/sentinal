/*
 * validdbname.c
 * Fix the string to conform to sqlite3 database/table naming rules.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <ctype.h>

#ifndef TRUE
# define	TRUE		(short)1
#endif

#ifndef	FALSE
# define	FALSE		(short)0
#endif

short validdbname(char *str)
{
	char   *p = str;

	if(!isalpha(*p))
		return (FALSE);

	while(*p) {
		if(!(isalnum(*p) || *p == '_'))
			*p = '_';

		p++;
	}

	return (TRUE);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
