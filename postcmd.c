/*
 * postcmd.c
 * Run command after the log closes or rotates.
 *
 * Copyright (c) 2021-2026 jjb
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

static bool appendstr(char *dst, size_t dstsize, size_t *dstlen, const char *src)
{
	while(src && *src) {
		if(*dstlen >= dstsize - 1)
			return (false);

		dst[(*dstlen)++] = *src++;
	}

	dst[*dstlen] = '\0';
	return (true);
}

/* append src in single-quotes, escaping embedded single-quotes as '\'' */
static bool shellquote(char *dst, size_t dstsize, size_t *dstlen, const char *src)
{
	if(!src)
		return (true);

	if(!appendstr(dst, dstsize, dstlen, "'"))
		return (false);

	while(*src) {
		if(*src == '\'') {
			if(!appendstr(dst, dstsize, dstlen, "'\\''"))
				return (false);
		} else {
			if(*dstlen >= dstsize - 1)
				return (false);

			dst[(*dstlen)++] = *src;
			dst[*dstlen] = '\0';
		}

		src++;
	}

	return (appendstr(dst, dstsize, dstlen, "'"));
}

static void expandpostcmd(struct thread_info *ti, char *filename, char *cmd,
						  size_t cmdsize)
{
	const char *src = ti->ti_postcmd;
	size_t  dstlen = 0;

	*cmd = '\0';

	while(src && *src) {
		if(strncmp(src, _HOST_TOK, strlen(_HOST_TOK)) == 0) {
			if(!shellquote(cmd, cmdsize, &dstlen, utsbuf.nodename))
				break;
			src += strlen(_HOST_TOK);
		} else if(strncmp(src, _PATH_TOK, strlen(_PATH_TOK)) == 0) {
			if(!shellquote(cmd, cmdsize, &dstlen, ti->ti_dirname))
				break;
			src += strlen(_PATH_TOK);
		} else if(strncmp(src, _FILE_TOK, strlen(_FILE_TOK)) == 0) {
			if(!shellquote(cmd, cmdsize, &dstlen, filename))
				break;
			src += strlen(_FILE_TOK);
		} else if(strncmp(src, _SECT_TOK, strlen(_SECT_TOK)) == 0) {
			if(!shellquote(cmd, cmdsize, &dstlen, ti->ti_section))
				break;
			src += strlen(_SECT_TOK);
		} else {
			if(dstlen >= cmdsize - 1)
				break;

			cmd[dstlen++] = *src++;
			cmd[dstlen] = '\0';
		}
	}
}

int postcmd(struct thread_info *ti, char *filename)
{
	char    cmdbuf[BUFSIZ];							/* command var */
	char   *home;									/* from passwd file entry */
	extern bool dryrun;								/* dry run flag */
	int     i;
	int     status;									/* postcmd child exit */
	pid_t   pid;									/* postcmd pid */
	struct passwd *p;
	size_t  tasklen;

	if(IS_NULL(ti->ti_postcmd)) {
		/* should not be here */
		return (-1);
	}

	switch (pid = fork()) {

	case -1:
		fprintf(stderr, "%s: can't fork postcmd\n", ti->ti_section);
		return (-1);

	case 0:
		if(droppriv(ti) == -1) {
			fprintf(stderr, "%s: can't drop privileges\n", ti->ti_section);
			exit(EXIT_FAILURE);
		}

		if(chdir(ti->ti_dirname) == -1) {
			fprintf(stderr, "%s: can't cd to %s\n", ti->ti_section, ti->ti_dirname);
			exit(EXIT_FAILURE);
		}

		/* postcmd tokens -- values are single-quoted to prevent shell injection */

		expandpostcmd(ti, filename, cmdbuf, BUFSIZ);

		fprintf(stderr, "%s: %s\n", ti->ti_section, cmdbuf);
		fflush(stderr);

		if(dryrun)
			exit(EXIT_SUCCESS);

		/* close parent's and unused fds */

		for(i = 3; i < MAXFILES; i++)
			close(i);

		/* execution environment */

		home = (p = getpwuid(ti->ti_uid)) ? p->pw_dir : "/tmp";
		setenv("HOME", home, 1);
		setenv("PATH", PATH, 1);
		setenv("SHELL", BASH, 1);
		setenv("TEMPLATE", ti->ti_template, 1);
		setenv("PCRESTR", ti->ti_pcrestr, 1);

		umask(umask(0) | 022);						/* don't set less restrictive */
		nice(1);
		execl(BASH, "bash", "--noprofile", "--norc", "-c", cmdbuf, NULL);
		exit(EXIT_FAILURE);

	default:
		if(waitpid(pid, &status, 0) == -1)
			return (-1);

		if(ti->ti_truncate && NOT_NULL(filename) && NOT_NULL(ti->ti_task)) {
			/* slm only */

			tasklen = strlen(ti->ti_task);

			if(tasklen >= strlen(_SLM_THR) &&
			   strcmp(ti->ti_task + tasklen - strlen(_SLM_THR), _SLM_THR) == 0)
				truncate(filename, (off_t) 0);
		}

		return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
	}
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
