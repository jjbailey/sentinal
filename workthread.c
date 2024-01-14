/*
 * workthread.c
 * Execute command, read input from a FIFO, write output to a logfile.
 *
 * Copyright (c) 2021-2023 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"

#define PIPEBUFSIZ  (4 << 20)						/* 4MiB, better size for IPC i/o */

#define	ROTATE(lim,n,sig)	((lim && n > lim) || sig == SIGHUP)
#define	STAT(file,buf)		(stat(file, &buf) == -1 ? -1 : buf.st_size)
#define	SLOWEXIT(code)		{ sleep(5); exit(code); }

static int fifoopen(struct thread_info *);
static void fifosize(struct thread_info *, int);

void   *workthread(void *arg)
{
	char    filename[PATH_MAX];						/* full pathname */
	char    homebuf[BUFSIZ];						/* hone env var */
	char    pathbuf[BUFSIZ];						/* path env var */
	char    pcrebuf[BUFSIZ];						/* env convenience */
	char    pipebuf[PIPEBUFSIZ];					/* pipe read buffer */
	char    shellbuf[BUFSIZ];						/* shell env var */
	char    tmplbuf[BUFSIZ];						/* env convenience */
	char   *home;									/* from passwd file entry */
	char   *zargv[MAXARGS];							/* arglist for fifo reader */
	int     holdfd = 0;								/* fd to hold FIFO open */
	int     i;
	int     logfd;									/* primary FIFO fd */
	int     pipefd[2];								/* pipe readers and writers */
	int     status;									/* child status codes */
	ssize_t n;										/* FIFO read */
	struct passwd *p;
	struct stat stbuf;								/* file status */
	struct thread_info *ti = arg;					/* thread settings */

	/*
	 * this thread requires:
	 *  - ti_command
	 *  - ti_pipename
	 *  - ti_template
	 *
	 * optional, but recommended:
	 *  - ti_rotatesiz
	 *
	 * optional:
	 *  - ti_postcmd
	 *
	 * optional, likely required by use case:
	 *  - ti_uid
	 *  - ti_gid
	 */

	pthread_setname_np(pthread_self(), threadname(ti, _WRK_THR));

	fprintf(stderr, "%s: command: %s\n", ti->ti_section, ti->ti_command);

	if(ti->ti_rotatesiz)
		fprintf(stderr, "%s: monitor file: %s for size %s\n",
				ti->ti_section, ti->ti_pcrestr, ti->ti_rotatestr);

	for(;;) {
		/* set up pipes */

		if((logfd = fifoopen(ti)) == -1) {
			/* open/create FIFO failed */
			return ((void *)0);
		}

		if(pipe(pipefd) == -1) {
			fprintf(stderr, "%s: can't create IPC pipe\n", ti->ti_section);
			return ((void *)0);
		}

		logname(ti->ti_template, ti->ti_filename);
		fullpath(ti->ti_dirname, ti->ti_filename, filename);
		workcmd(ti->ti_argc, ti->ti_argv, zargv);

		/* for systemctl status sentinal */

		fprintf(stderr, "%s: ", ti->ti_section);
		for(i = 0; zargv[i]; i++)
			fprintf(stderr, "%s ", zargv[i]);
		fprintf(stderr, "> %s\n", ti->ti_filename);	/* show redirect */

		/* get going */

		switch (ti->ti_pid = fork()) {

		case -1:
			fprintf(stderr, "%s: can't fork command\n", ti->ti_section);
			return ((void *)0);

		case 0:
			/*
			 * child: command reads from sentinal, writes to stdout
			 * application -> FIFO -> sentinal -> IPC pipe -> command -> logfile
			 */

			if(geteuid() == (uid_t) 0) {
				(void)setgid(ti->ti_gid);
				(void)setuid(ti->ti_uid);
			}

			if(access(ti->ti_dirname, R_OK | W_OK | X_OK) == -1) {
				fprintf(stderr, "%s: insufficient permissions for %s\n",
						ti->ti_section, ti->ti_dirname);

				SLOWEXIT(EXIT_FAILURE);
			}

			if(chdir(ti->ti_dirname) == -1) {
				fprintf(stderr, "%s: can't cd to %s\n", ti->ti_section, ti->ti_dirname);
				SLOWEXIT(EXIT_FAILURE);
			}

			close(pipefd[1]);						/* close unused write end */
			dup2(pipefd[0], STDIN_FILENO);			/* new stdin */
			close(STDOUT_FILENO);

			if(open(filename, O_WRONLY | O_CREAT, 0644) == -1) {
				/* shouldn't happen */

				fprintf(stderr, "%s: can't create %s\n", ti->ti_section, ti->ti_filename);
				SLOWEXIT(EXIT_FAILURE);
			} else {
				/* new stdout -- close parent's and unused fds */

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

				umask(umask(0) | 022);				/* don't set less restrictive */
				nice(1);
				execv(ti->ti_path, zargv);
				exit(EXIT_FAILURE);
			}

		default:
			/*
			 * parent: sentinal reads from FIFO, writes to stdout
			 * application -> FIFO -> sentinal -> IPC pipe -> command -> logfile
			 */

			close(pipefd[0]);						/* close unused read end */
			ti->ti_wfd = pipefd[1];					/* save fd for close */

			/* parent needs to keep the pipe open for reading */

			if(holdfd > 0)
				close(holdfd);

			holdfd = open(ti->ti_pipename, O_RDONLY | O_NONBLOCK);

			/* begin */

			ti->ti_sig = 0;							/* reset */

			for(;;) {
				if((n = read(logfd, pipebuf, PIPEBUFSIZ)) <= 0)
					break;

				if(write(ti->ti_wfd, pipebuf, (size_t)n) == -1) {
					fprintf(stderr, "%s: write failed %s\n", ti->ti_section,
							ti->ti_filename);

					break;
				}

				if(ROTATE(ti->ti_rotatesiz, STAT(filename, stbuf), ti->ti_sig)) {
					/* ti_rotatesiz or signaled to logrotate */

					fprintf(stderr, "%s: rotate %s\n", ti->ti_section, ti->ti_filename);
					ti->ti_sig = 0;					/* reset */
					break;
				}
			}

			/* done */

			if(n == 0) {							/* writer is gone */
				close(holdfd);
				holdfd = 0;
			}

			close(ti->ti_wfd);
			waitpid(ti->ti_pid, &status, 0);
			ti->ti_wfd = EOF;						/* done with this file */

			/* if file is empty, write failed, e.g. */
			/* No space left on device (cannot write compressed block) */

			if(STAT(filename, stbuf) > 0) {			/* success */
				if(NOT_NULL(ti->ti_postcmd))
					if((status = postcmd(ti, filename)) != 0) {
						fprintf(stderr, "%s: postcmd exit: %d\n", ti->ti_section, status);
						sleep(5);					/* be nice */
					}
			} else {								/* fail */
				remove(filename);					/* exists, CWE-367 N/A */
				sleep(5);							/* be nice */
			}

			close(logfd);							/* pipe remains held open by holdfd */
		}
	}

	/* notreached */
	return ((void *)0);
}

static void fifosize(struct thread_info *ti, int size)
{
	/* max pipesize is in /proc/sys/fs/pipe-max-size */
	/* kernels 2.6.35 and newer */

#ifdef	F_SETPIPE_SZ								/* undefined in < 2.6.35 kernels */

	int     fd;

	if((fd = open(ti->ti_pipename, O_RDONLY | O_NONBLOCK)) == -1)
		return;

	if(fcntl(fd, F_GETPIPE_SZ) != size)
		fcntl(fd, F_SETPIPE_SZ, size);

	close(fd);

#endif												/* F_SETPIPE_SZ */
}

static int fifoopen(struct thread_info *ti)
{
	int     fd;
	int     status;
	pid_t   pid;
	short   pflag = FALSE;
	struct stat stbuf;								/* file status */

	/* create a FIFO. note: this permits symlinks to FIFOs */

	if(lstat(ti->ti_pipename, &stbuf) == 0) {		/* found something */
		if(!S_ISFIFO(stbuf.st_mode)) {
			fprintf(stderr, "%s: not a FIFO: %s\n", ti->ti_section,
					base(ti->ti_pipename));

			return (-1);
		}
	} else
		pflag = TRUE;

	if(pflag) {										/* need a FIFO */
		/* fork to set ids */

		if((pid = fork()) == 0) {
			if(geteuid() == (uid_t) 0) {
				(void)setgid(ti->ti_gid);
				(void)setuid(ti->ti_uid);
			}

			if(mkfifo(ti->ti_pipename, 0600) == -1) {
				fprintf(stderr, "%s: can't mkfifo %s: %s\n", ti->ti_section,
						base(ti->ti_pipename), strerror(errno));

				exit(EXIT_FAILURE);
			}

			fifosize(ti, FIFOSIZ);
			exit(EXIT_SUCCESS);
		}

		if(pid > 0)
			waitpid(pid, &status, 0);
	}

	if((fd = open(ti->ti_pipename, O_RDONLY)) == -1) {
		/* systemctl restart can cause EINTR */

		if(errno != EINTR)
			fprintf(stderr, "%s: can't open %s: %s\n", ti->ti_section,
					base(ti->ti_pipename), strerror(errno));
	} else {
		/* reenforce in case these were changed externally */

		chown(ti->ti_pipename, ti->ti_uid, ti->ti_gid);
		chmod(ti->ti_pipename, 0600);
	}

	return (fd);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
