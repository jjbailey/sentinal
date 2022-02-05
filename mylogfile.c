/*
 * mylogfile.c
 * Return TRUE if this is a file we're watching.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include "sentinal.h"

#define	OVECSIZE	32

int mylogfile(char *f, pcre *p)
{
	int     ovec[OVECSIZE];

	if(IS_NULL(f) || *f == '.')					/* null or begins with . */
		return (FALSE);

	return (pcre_exec(p, NULL, f, strlen(f), 0, 0, ovec, OVECSIZE) >= 0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
