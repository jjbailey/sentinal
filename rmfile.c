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
#include <unistd.h>
#include "sentinal.h"

void rmfile(struct thread_info *ti, char *obj, char *remark)
{
	extern short dryrun;

	if(access(obj, F_OK) == 0) {
		/* not yet removed by another thread */

		if(!ti->ti_terse)
			fprintf(stderr, "%s: %s %s\n", ti->ti_section, remark, obj);

		if(!dryrun)
			remove(obj);
	}
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
