/*
 * verifyids.c
 * Verify user and group ids.
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/types.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include "sentinal.h"

#define	NOBODY	((uid_t) 65534)
#define	NOGROUP	((gid_t) 65534)

uid_t verifyuid(const char *id)
{
	struct passwd *p;
	uid_t   myuid;
	uid_t   uid = NOBODY;

	if((myuid = geteuid()) != 0)
		return (myuid);

	if(IS_NULL(id))
		return (NOBODY);

	if(isdigit(*id)) {								/* uid cannot be 0 */
		uid = (uid_t) atoi(id);
		return (uid == 0 ? NOBODY : uid);
	}

	/* "root" ok past this point */

	setpwent();

	if((p = getpwnam(id)))
		uid = p->pw_uid;

	endpwent();
	return (uid);
}

gid_t verifygid(const char *id)
{
	struct group *p;
	gid_t   mygid;
	gid_t   gid = NOGROUP;

	if((mygid = getegid()) != 0)
		return (mygid);

	if(IS_NULL(id))
		return (NOGROUP);

	if(isdigit(*id)) {								/* gid cannot be 0 */
		gid = (gid_t) atoi(id);
		return (gid == 0 ? NOBODY : gid);
	}

	/* "root" ok past this point */

	setgrent();

	if((p = getgrnam(id)))
		gid = p->gr_gid;

	endgrent();
	return (gid);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
