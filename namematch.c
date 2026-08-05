/*
 * namematch.c
 * Return true if this is a file we're watching.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include "sentinal.h"

bool namematch(struct thread_info *ti, char *f)
{
	static __thread pcre2_match_data *mdata = NULL;
	static __thread pcre2_code *mdata_code = NULL;
	int     rc;
	uint32_t options = 0;

	if(IS_NULL(f) || *f == '.' || ti->ti_pcrecmp == NULL)
		return (false);

	if(mdata == NULL || mdata_code != ti->ti_pcrecmp) {
		if(mdata != NULL)
			pcre2_match_data_free(mdata);

		mdata_code = ti->ti_pcrecmp;
		mdata = pcre2_match_data_create_from_pattern(mdata_code, NULL);

		if(mdata == NULL) {
			mdata_code = NULL;
			return (false);
		}
	}

	rc = pcre2_match(ti->ti_pcrecmp, (PCRE2_SPTR) f, strlen(f), (PCRE2_SIZE) 0, options,
					 mdata, NULL);

	return (rc >= 0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
