/*
 * verifyids.c
 * Verify user and group ids.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <unistd.h>
#include "sentinal.h"

#define	NOBODY	((uid_t) 65534)
#define	NOGROUP	((gid_t) 65534)

/* parse a fully numeric id; return -1 on malformed/out-of-range input */
static long parsenumid(const char *id)
{
	char   *endptr;
	long    val;

	errno = 0;
	val = strtol(id, &endptr, 10);

	if(errno != 0 || *endptr != '\0' || val < 0 || val > 0x7fffffffL)
		return (-1);								/* not a clean, in-range number */

	return (val);
}

uid_t verifyuid(const char *id)
{
	struct passwd *p;
	uid_t   myuid;
	uid_t   uid = NOBODY;

	if((myuid = geteuid()) != 0)
		return (myuid);

	if(IS_NULL(id))
		return (NOBODY);

	if(isdigit((unsigned char)*id)) {				/* numeric uid, cannot be 0 */
		long    val = parsenumid(id);

		return (val <= 0 ? NOBODY : (uid_t) val);
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

	if(isdigit((unsigned char)*id)) {				/* numeric gid, cannot be 0 */
		long    val = parsenumid(id);

		return (val <= 0 ? NOGROUP : (gid_t) val);
	}

	/* "root" ok past this point */

	setgrent();

	if((p = getgrnam(id)))
		gid = p->gr_gid;

	endgrent();
	return (gid);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
