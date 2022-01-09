/*
 * verifyids.c
 * Verify user and group ids.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/types.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include "sentinal.h"

#define	NOBODY	((uid_t) 65534)
#define	NOGROUP	((gid_t) 65534)

uid_t verifyuid(char *id)
{
	struct passwd *p;
	uid_t   uid = NOBODY;

	if(IS_NULL(id))
		return (NOBODY);

	if(isdigit(*id)) {
		if((uid = (uid_t) atoi(id)) == 0)
			return (NOBODY);

		return (uid);
	}

	/* "root" ok past this point */

	setpwent();

	if(p = getpwnam(id))
		uid = p->pw_uid;

	endpwent();
	return (uid);
}

gid_t verifygid(char *id)
{
	struct group *p;
	gid_t   gid = NOGROUP;

	if(IS_NULL(id))
		return (NOGROUP);

	if(isdigit(*id)) {
		if((gid = (gid_t) atoi(id)) == 0)
			return (NOGROUP);

		return (gid);
	}

	/* "root" ok past this point */

	setgrent();

	if(p = getgrnam(id))
		gid = p->gr_gid;

	endgrent();
	return (gid);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
