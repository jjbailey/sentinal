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

extern struct utsname utsbuf;

int postcmd(struct thread_info *ti, char *filename)
{
	char    cmdbuf[BUFSIZ];
	char    homebuf[BUFSIZ];
	char    pathbuf[BUFSIZ];
	char    pcrebuf[BUFSIZ];
	char    shellbuf[BUFSIZ];
	char    tmplbuf[BUFSIZ];
	char   *home;
	extern short dryrun;							/* dry run bool */
	int     i;
	int     status;									/* postcmd child exit */
	pid_t   pid;									/* postcmd pid */
	struct passwd *p;

	if(IS_NULL(ti->ti_postcmd)) {
		/* should not be here */
		return (-1);
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

		if(access(ti->ti_dirname, R_OK | W_OK | X_OK) == -1) {
			fprintf(stderr, "%s: insufficient permissions for %s\n",
					ti->ti_section, ti->ti_dirname);

			exit(EXIT_FAILURE);
		}

		if(chdir(ti->ti_dirname) == -1) {
			fprintf(stderr, "%s: can't cd to %s\n", ti->ti_section, ti->ti_dirname);
			exit(EXIT_FAILURE);
		}

		strlcpy(cmdbuf, ti->ti_postcmd, BUFSIZ);

		/* postcmd tokens */

		strreplace(cmdbuf, _HOST_TOK, utsbuf.nodename);
		strreplace(cmdbuf, _PATH_TOK, ti->ti_dirname);
		strreplace(cmdbuf, _FILE_TOK, filename);
		strreplace(cmdbuf, _SECT_TOK, ti->ti_section);

		fprintf(stderr, "%s: %s\n", ti->ti_section, cmdbuf);

		if(dryrun)
			exit(EXIT_SUCCESS);

		/* close parent's and unused fds */

		for(i = 3; i < MAXFILES; i++)
			close(i);

		/* execution environment */

		home = (p = getpwuid(ti->ti_uid)) ? p->pw_dir : "/tmp";
		snprintf(homebuf, BUFSIZ, "HOME=%s", home);
		snprintf(pathbuf, BUFSIZ, "PATH=%s", PATH);
		snprintf(shellbuf, BUFSIZ, "SHELL=%s", BASH);
		snprintf(tmplbuf, BUFSIZ, "TEMPLATE=%s", ti->ti_template);
		snprintf(pcrebuf, BUFSIZ, "PCRESTR=%s", ti->ti_pcrestr);

		umask(umask(0) | 022);						/* don't set less restrictive */
		nice(1);

		execl(ENV, "-S", "-",
			  homebuf, pathbuf, shellbuf,
			  tmplbuf, pcrebuf,
			  BASH, "--noprofile", "--norc", "-c", cmdbuf, (char *)NULL);

		exit(EXIT_FAILURE);

	default:
		waitpid(pid, &status, 0);

		if(ti->ti_truncate && NOT_NULL(filename)) {
			/* slm only */

			if(strcmp(strrchr(ti->ti_task, '\0') - 3, _SLM_THR) == 0)
				truncate(filename, (off_t) 0);
		}

		return (status == -1 ? status : 0);
	}
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
