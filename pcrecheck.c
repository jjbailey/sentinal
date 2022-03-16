/*
 * pcrecheck.c
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

pcre2_code *pcrecheck(char *pcrestr, pcre2_code *re)
{
	PCRE2_SIZE erroffset;
	PCRE2_UCHAR buf[256];
	int     errnumber;
	uint32_t options = PCRE2_ANCHORED;

	if(IS_NULL(pcrestr))
		return ((pcre2_code *) NULL);

	re = pcre2_compile((PCRE2_SPTR) pcrestr, PCRE2_ZERO_TERMINATED,
					   options, &errnumber, &erroffset, NULL);

	if(re == NULL) {
		pcre2_get_error_message(errnumber, buf, sizeof(buf));

		fprintf(stderr, "PCRE2 compilation failed at offset %d: %hhn\n",
				(int)erroffset, buf);
	}

	return (re);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
