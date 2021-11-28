/*
 * mylogfile.c
 * Return TRUE if this is a file we're watching for.
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include "sentinal.h"

#define	OVECSIZE	32

int mylogfile(char *fname, pcre * pcrecmp)
{
	int     ovec[OVECSIZE];

	if(IS_NULL(fname))
		return (FALSE);

	if(*fname == '-' || *fname == '.')			/* begins with - or . */
		return (FALSE);

	if(pcre_exec(pcrecmp, NULL, fname, strlen(fname), 0, 0, ovec, OVECSIZE) >= 0) {
#if 0
		fprintf(stderr, "%s\n", fname);
#endif
		return (TRUE);
	}

	return (FALSE);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
