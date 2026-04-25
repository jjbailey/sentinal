/*
 * droppriv.c
 * initialize group access list
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include "sentinal.h"

int droppriv(struct thread_info *ti)
{
	struct passwd *p;

	if(geteuid() != (uid_t) 0)
		return (0);

	p = getpwuid(ti->ti_uid);

	if(p == NULL || initgroups(p->pw_name, ti->ti_gid) == -1 ||
	   setgid(ti->ti_gid) == -1 || setuid(ti->ti_uid) == -1)
		return (-1);

	return (0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
