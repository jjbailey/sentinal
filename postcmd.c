/*
 * postcmd.c
 * Run command after the log closes or rotates.
 *
 * Copyright (c) 2021-2024 jjb
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

extern struct utsname utsbuf;						/* for host info */

int postcmd(struct thread_info *ti, char *filename)
{
	char    cmdbuf[BUFSIZ];							/* command var */
	char    homebuf[BUFSIZ];						/* hone env var */
	char    pathbuf[BUFSIZ];						/* path env var */
	char    pcrebuf[BUFSIZ];						/* env convenience */
	char    shellbuf[BUFSIZ];						/* shell env var */
	char    tmplbuf[BUFSIZ];						/* env convenience */
	char   *home;									/* from passwd file entry */
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
			(void)setgid(ti->ti_gid);
			(void)setuid(ti->ti_uid);
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
		if(snprintf(homebuf, BUFSIZ, "HOME=%s", home) < BUFSIZ)
			putenv(homebuf);

		if(snprintf(pathbuf, BUFSIZ, "PATH=%s", PATH) < BUFSIZ)
			putenv(pathbuf);

		if(snprintf(shellbuf, BUFSIZ, "SHELL=%s", BASH) < BUFSIZ)
			putenv(shellbuf);

		if(snprintf(tmplbuf, BUFSIZ, "TEMPLATE=%s", ti->ti_template) < BUFSIZ)
			putenv(tmplbuf);

		if(snprintf(pcrebuf, BUFSIZ, "PCRESTR=%s", ti->ti_pcrestr) < BUFSIZ)
			putenv(pcrebuf);

		umask(umask(0) | 022);						/* don't set less restrictive */
		nice(1);
		execl(ENV, "-S", "-", BASH, "--noprofile", "--norc", "-c", cmdbuf, NULL);
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
