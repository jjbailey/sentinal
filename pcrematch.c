/*
 * pcrematch.c
 * Run the pcre2_match function to test a string.
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include "sentinal.h"

bool pcrematch(struct thread_info *ti, char *s)
{
	int     rc;
	pcre2_match_data *mdata;
	uint32_t options = 0;

	/* mostly copied from namematch.c */

	if(IS_NULL(s) || ti->ti_pcrecmp == NULL)
		return (false);

	mdata = pcre2_match_data_create_from_pattern(ti->ti_pcrecmp, NULL);

	rc = pcre2_match(ti->ti_pcrecmp, (PCRE2_SPTR) s, strlen(s), (PCRE2_SIZE) 0, options,
					 mdata, NULL);

	pcre2_match_data_free(mdata);
	return (rc >= 0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
