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

short mylogfile(char *f, pcre2_code *re)
{
	int     rc;
	pcre2_match_data *mdata;
	uint32_t options = PCRE2_ANCHORED;

	if(IS_NULL(f) || *f == '.')
		return (FALSE);

	mdata = pcre2_match_data_create_from_pattern(re, NULL);
	rc = pcre2_match(re, (PCRE2_SPTR) f, strlen(f), (PCRE2_SIZE) 0, options, mdata, NULL);
	pcre2_match_data_free(mdata);

	return (rc >= 0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
