/*
 * pcrecheck.c
 * Check and compile a regex for later use.
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include "sentinal.h"

pcre   *pcrecheck(char *pcrestr, pcre * pcrecmp)
{
	const char *errptr;
	int     erroffset;
	int     options = PCRE_ANCHORED;			/* might be undesirable */

	if(IS_NULL(pcrestr))
		return ((pcre *) NULL);

	return (pcrecmp = pcre_compile(pcrestr, options, &errptr, &erroffset, NULL));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
