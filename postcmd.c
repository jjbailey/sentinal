/*
 * postcmd.c
 * Run command after the log closes or rotates.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"

#define	ENV		"/usr/bin/env"
#define	BASH	"/bin/bash"
#define	PATH	"/usr/bin:/usr/sbin"

extern struct utsname utsbuf;

int postcmd(struct thread_info *ti, char *filename)
{
	char    cmdbuf[BUFSIZ];
	char    envhome[BUFSIZ];
	char    envpath[BUFSIZ];
	char    envshell[BUFSIZ];
	char   *home;
	int     i;
	int     status;
	pid_t   pid;
	struct passwd *p;

	if(IS_NULL(ti->ti_postcmd)) {
		/* should not be here */
		return (0);
	}

	switch (pid = fork()) {

	case -1:
		fprintf(stderr, "%s: can't fork postcmd\n", ti->ti_section);
		return (-1);

	case 0:
		if(geteuid() == (uid_t) 0) {
			setgid(ti->ti_gid);
			setuid(ti->ti_uid);
		}

		if(chdir(ti->ti_dirname) == -1) {
			fprintf(stderr, "%s: can't cd to %s\n", ti->ti_section, ti->ti_dirname);
			exit(EXIT_FAILURE);
		}

		strlcpy(cmdbuf, ti->ti_postcmd, BUFSIZ);
		strreplace(cmdbuf, _HOST_TOK, utsbuf.nodename);
		strreplace(cmdbuf, _DIR_TOK, ti->ti_dirname);
		strreplace(cmdbuf, _FILE_TOK, filename);
		strreplace(cmdbuf, _SECT_TOK, ti->ti_section);
		fprintf(stderr, "%s: %s\n", ti->ti_section, cmdbuf);

#if 0
		fprintf(stderr, "%s: postcmd pid: %d\n", ti->ti_section, getpid());
#endif

		/* close parent's and unused fds */

		for(i = 3; i < MAXFILES; i++)
			close(i);

		home = (p = getpwuid(ti->ti_uid)) ? p->pw_dir : "/tmp";
		snprintf(envhome, BUFSIZ, "HOME=%s", home);
		snprintf(envpath, BUFSIZ, "PATH=%s", PATH);
		snprintf(envshell, BUFSIZ, "SHELL=%s", BASH);

		umask(umask(0) | 022);					/* don't set less restrictive */
		nice(1);

		execl(ENV, "-S", "-",
			  envhome, envpath, envshell,
			  BASH, "--noprofile", "--norc", "-c", cmdbuf, (char *)NULL);

		exit(EXIT_FAILURE);

	default:
		return (waitpid(pid, &status, 0) == -1 ? status : 0);
	}
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
