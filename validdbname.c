/*
 * validdbname.c
 * Fix the string to conform to SQLite3 database/table naming rules.
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdbool.h>
#include <ctype.h>

#ifndef	IS_NULL
# define	IS_NULL(s) !((s) && *(s))
#endif

bool validdbname(char *str)
{
	if(IS_NULL(str) || !isalpha((unsigned char)*str))
		return (false);

	for(char *p = str; *p; p++)
		if(!(isalnum((unsigned char)*p) || *p == '_'))
			*p = '_';

	return (true);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
