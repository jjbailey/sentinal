/*
 * rmfile.c
 * Remove a file or an empty directory.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include "sentinal.h"

short rmfile(struct thread_info *ti, char *obj, char *remark)
{
	extern short dryrun;
	int     rc = 0;

	if(!dryrun)
		if((rc = remove(obj)) != 0)
			return (FALSE);

	if(!ti->ti_terse && rc == 0)
		fprintf(stderr, "%s: %s %s\n", ti->ti_section, remark, obj);

	return (rc == 0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
