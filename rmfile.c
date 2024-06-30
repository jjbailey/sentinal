/*
 * rmfile.c
 * Remove a file or an empty directory.
 *
 * Copyright (c) 2021-2024 jjb
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

short rmfile(struct thread_info *ti, char *obj, char *remark)
{
	char    buf[BUFSIZ];
	extern short dryrun;

	if(!dryrun)
		if(remove(obj) != 0)
			if(strerror_r(errno, buf, BUFSIZ) == 0) {
				fprintf(stderr, "%s: %s %s: %s\n", ti->ti_section, remark, obj, buf);
				sleep(1);
				return (FALSE);
			}

	if(!ti->ti_terse)
		fprintf(stderr, "%s: %s %s\n", ti->ti_section, remark, obj);

	return (TRUE);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
