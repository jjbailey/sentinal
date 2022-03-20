/*
 * pcrecompile.c
 * Check and compile a regex for later use.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include "sentinal.h"

void pcrecompile(struct thread_info *ti)
{
	PCRE2_SIZE erroffset;
	int     errnumber;
	uint32_t options = PCRE2_ANCHORED;

	if(IS_NULL(ti->ti_pcrestr))
		return;

	ti->ti_pcrecmp = pcre2_compile((PCRE2_SPTR) ti->ti_pcrestr, PCRE2_ZERO_TERMINATED,
								   options, &errnumber, &erroffset, NULL);

	if(ti->ti_pcrecmp == NULL)
		fprintf(stderr, "%s: pcre2 compilation failed: %s\n", ti->ti_section,
				ti->ti_pcrestr);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
