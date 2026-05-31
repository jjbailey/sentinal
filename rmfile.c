/*
 * rmfile.c
 * Remove a file or an empty directory.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"

bool rmfile(struct thread_info *ti, const char *obj, const char *remark)
{
	extern bool dryrun;

	if(!dryrun && remove(obj) != 0) {
		int     errnum = errno;

		if(!ti->ti_terse)
			fprintf(stderr, "%s: error %s %s: %s\n",
					ti->ti_section, remark, obj, strerror(errnum));

		sleep(1);
		return (false);
	}

	if(!ti->ti_terse)
		fprintf(stderr, "%s: %s %s\n", ti->ti_section, remark, obj);

	return (true);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
