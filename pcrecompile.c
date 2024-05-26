/*
 * pcrecompile.c
 * Check and compile a regex for later use.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include "sentinal.h"

short pcrecompile(struct thread_info *ti)
{
	PCRE2_SIZE erroffset;
	PCRE2_SIZE length = PCRE2_ZERO_TERMINATED;
	int     errnumber;
	uint32_t options = 0;

	if(IS_NULL(ti->ti_pcrestr)) {
		/*
		 * null is ok -- ensures namematch() always returns false
		 */

		ti->ti_pcrecmp = NULL;
	} else {
		ti->ti_pcrecmp = pcre2_compile((PCRE2_SPTR) ti->ti_pcrestr, length,
									   options, &errnumber, &erroffset, NULL);

		if(ti->ti_pcrecmp == NULL)
			fprintf(stderr, "%s: pcre2 compilation failed: %s\n",
					ti->ti_section, ti->ti_pcrestr);
	}

	return (ti->ti_pcrecmp != NULL);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
