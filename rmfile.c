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

	if(!dryrun)
		if(remove(obj) != 0)
			return (FALSE);

	if(!ti->ti_terse)
		fprintf(stderr, "%s: %s %s\n", ti->ti_section, remark, obj);

	return (TRUE);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
