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

#define	PWBUFSIZ	16384							/* getpwuid_r() scratch buffer */

/*
 * do the NSS lookups (getpwuid_r, getgrouplist) here, in the parent,
 * before fork(). neither is async-signal-safe, so none of this may run
 * in a freshly-forked child of a multithreaded process -- see droppriv().
 */

bool preparedrop(struct thread_info *ti, struct dropinfo *di)
{
	char    buf[PWBUFSIZ];
	struct passwd pwbuf;
	struct passwd *p = NULL;

	di->di_uid = ti->ti_uid;
	di->di_gid = ti->ti_gid;
	di->di_ngroups = 0;
	di->di_dodrop = (geteuid() == (uid_t) 0);
	strlcpy(di->di_home, "/tmp", sizeof(di->di_home));

	if(getpwuid_r(ti->ti_uid, &pwbuf, buf, sizeof(buf), &p) != 0 || p == NULL)
		return (!di->di_dodrop);					/* home lookup failed but
													   nothing needs dropping */

	strlcpy(di->di_home, p->pw_dir, sizeof(di->di_home));

	if(di->di_dodrop) {
		int     ngroups = MAXGROUPS;

		if(getgrouplist(p->pw_name, ti->ti_gid, di->di_groups, &ngroups) == -1)
			ngroups = MAXGROUPS;					/* truncated to our cap */

		di->di_ngroups = ngroups;
	}

	return (true);
}

/* apply a prepared privilege drop -- syscalls only, safe post-fork */
int droppriv(struct dropinfo *di)
{
	if(!di->di_dodrop)
		return (0);

	if(setgroups((size_t)di->di_ngroups, di->di_groups) == -1 ||
	   setgid(di->di_gid) == -1 || setuid(di->di_uid) == -1)
		return (-1);

	return (0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
