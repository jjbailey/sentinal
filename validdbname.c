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

#include <stdbool.h>
#include <ctype.h>

#ifndef true
# define	true		(bool)1
#endif

#ifndef	false
# define	false		(bool)0
#endif

bool validdbname(char *str)
{
	char   *p = str;

	if(!isalpha(*p))
		return (false);

	while(*p) {
		if(!(isalnum(*p) || *p == '_'))
			*p = '_';

		p++;
	}

	return (true);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
