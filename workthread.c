/*
 * workthread.c
 * Create FIFO if it does not exist.
 * Execute command, read input from a FIFO, write output to a logfile.
 *
 * Copyright (c) 2021 jjb
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
#include <stdlib.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"

#define	PIPEBUFSIZ	(64 * ONE_KiB)				/* better size for ipc i/o */

#define	ROTATE(lim,n,sig)	((lim && n > lim) || sig == SIGHUP)
#define	STAT(file,buf)	(stat(file, &buf) == -1 ? -1 : buf.st_size)

static void pipesize(char *, char *, long);

void   *workthread(void *arg)
{
	char    filename[PATH_MAX];
	char    pipebuf[PIPEBUFSIZ];
	char    task[TASK_COMM_LEN];
	char   *zargv[MAXARGS];
	int     holdfd;								/* fd to hold FIFO open */
	int     i;
	int     logfd;
	int     n;
	int     pflag;
	int     pipefd[2];
	int     status;
	off_t   bytes;
	struct stat stbuf;
	struct thread_info *ti = arg;
	extern int errno;

	pthread_detach(pthread_self());
	pthread_setname_np(pthread_self(), threadname(ti->ti_section, "wrk", task));

	if(ti->ti_argc)								/* if 0, we would not be here */
		fprintf(stderr, "%s: command: %s\n", ti->ti_section, ti->ti_command);

	if(ti->ti_loglimit)
		fprintf(stderr, "%s: monitor log size: %ldMiB\n", ti->ti_section,
				MiB(ti->ti_loglimit));

	for(;;) {
		/*
		 * create a FIFO, replace a file with a FIFO
		 * note: this permits symlinks to FIFOs
		 */

		pflag = FALSE;

		if(lstat(ti->ti_pipename, &stbuf) == 0) {
			/* found something */

			if(!S_ISFIFO(stbuf.st_mode)) {
				char    pidbuf[BUFSIZ];
				char    savebuf[BUFSIZ];

				snprintf(pidbuf, BUFSIZ, "%s.%05d", ti->ti_pipename, getpid());
				fullpath(ti->ti_dirname, pidbuf, savebuf);
				rename(ti->ti_pipename, savebuf);
				pflag = TRUE;
			}
		} else
			pflag = TRUE;

		if(pflag == TRUE) {
			/* need a FIFO */

			if(mkfifo(ti->ti_pipename, 0644) == -1) {
				fprintf(stderr, "%s: can't mkfifo %s: %s\n", ti->ti_section,
						base(ti->ti_pipename), strerror(errno));

				exit(EXIT_FAILURE);
			}

			chown(ti->ti_pipename, ti->ti_uid, ti->ti_gid);
			pipesize(ti->ti_section, ti->ti_pipename, PIPESIZ);
		}

		if((logfd = open(ti->ti_pipename, O_RDONLY)) == -1) {
			/* systemctl restart can cause EINTR */

			if(errno != EINTR) {
				fprintf(stderr, "%s: can't open %s: %s\n", ti->ti_section,
						base(ti->ti_pipename), strerror(errno));

				exit(EXIT_FAILURE);
			}

			exit(EXIT_SUCCESS);
		}

		if(pipe(pipefd) == -1) {
			fprintf(stderr, "%s: can't create ipc pipe\n", ti->ti_section);
			exit(EXIT_FAILURE);
		}

		logname(ti->ti_template, ti->ti_filename);
		fullpath(ti->ti_dirname, ti->ti_filename, filename);
		runcmd(ti->ti_argc, ti->ti_argv, zargv);

		/* for systemctl status sentinal */

		fprintf(stderr, "%s: ", ti->ti_section);
		for(i = 0; zargv[i]; i++)
			fprintf(stderr, "%s ", zargv[i]);
		fprintf(stderr, "> %s\n", ti->ti_filename);	/* show redirect */

		if((ti->ti_pid = fork()) == 0) {
			/*
			 * child: command reads from sentinal, writes to stdout
			 * application -> FIFO -> sentinal -> IPC pipe -> command -> logfile
			 */

			if(geteuid() == (uid_t) 0) {
				setgid(ti->ti_gid);
				setuid(ti->ti_uid);
			}

			if(access(ti->ti_dirname, R_OK | W_OK | X_OK) == -1) {
				fprintf(stderr, "%s: insufficient permissions for %s\n",
						ti->ti_section, ti->ti_dirname);

				sleep(5);						/* be nice */
				exit(EXIT_FAILURE);
			}

			if(chdir(ti->ti_dirname) == -1) {
				fprintf(stderr, "%s: can't cd to %s\n", ti->ti_section, ti->ti_dirname);
				sleep(5);						/* be nice */
				exit(EXIT_FAILURE);
			}

			close(pipefd[1]);					/* close unused write end */
			dup2(pipefd[0], STDIN_FILENO);		/* new stdin */
			close(STDOUT_FILENO);

			if(open(filename, O_WRONLY | O_CREAT, 0644) == -1) {
				/* shouldn't happen */

				fprintf(stderr, "%s: can't create %s\n", ti->ti_section, ti->ti_filename);
				sleep(5);						/* be nice */
			} else {
				/* new stdout -- close parent's and unused fds */

				for(i = 3; i < 20; i++)
					close(i);

				execv(ti->ti_path, zargv);
			}

			exit(EXIT_FAILURE);
		} else {
			/*
			 * parent: sentinal reads from FIFO, writes to stdout
			 * application -> FIFO -> sentinal -> IPC pipe -> command -> logfile
			 */

			close(pipefd[0]);					/* close unused read end */
			ti->ti_wfd = pipefd[1];				/* save fd for close */

			/* parent needs to keep the pipe open for reading */

			if(holdfd > 0)
				close(holdfd);

			holdfd = open(ti->ti_pipename, O_RDONLY | O_NONBLOCK);

			/* begin */

			ti->ti_sig = 0;						/* reset */

			for(;;) {
				if((n = read(logfd, pipebuf, PIPEBUFSIZ)) <= 0)
					break;

				if(write(ti->ti_wfd, pipebuf, n) == -1) {
					fprintf(stderr, "%s: write failed %s\n", ti->ti_section,
							ti->ti_filename);

					break;
				}

				bytes = STAT(filename, stbuf);

				if(ROTATE(ti->ti_loglimit, bytes, ti->ti_sig)) {
					/* loglimit or signaled to logrotate */

					fprintf(stderr, "%s: rotate %s\n", ti->ti_section, ti->ti_filename);
					break;
				}
			}

			/* done */

			if(n == 0) {						/* writer is gone */
				close(holdfd);
				holdfd = 0;
			}

			close(ti->ti_wfd);
			waitpid(ti->ti_pid, &status, WUNTRACED | WCONTINUED);
			ti->ti_wfd = EOF;					/* done with this file */

			/* if file is empty, write failed, e.g. */
			/* No space left on device (cannot write compressed block) */

/*
			if(STAT(filename, stbuf) > 0 && !IS_NULL(ti->ti_postcmd)) {
 */
			if(STAT(filename, stbuf) > 0) {
				/* success */
				postcmd(ti, filename);
			} else {
				/* fail */
				remove(filename);
				sleep(5);						/* be nice */
			}

			close(logfd);						/* pipe remains held open by holdfd */
		}
	}

	/* notreached */
	exit(EXIT_SUCCESS);
}

static void pipesize(char *section, char *pipename, long size)
{
	/* max pipesize is in /proc/sys/fs/pipe-max-size */
	/* kernels 2.6.35 and newer */

#ifdef	F_SETPIPE_SZ							/* undefined in < 2.6.35 kernels */

	int     fd;
	long    cursize;

	if((fd = open(pipename, O_RDONLY | O_NONBLOCK)) == -1)
		return;

	if((cursize = fcntl(fd, F_GETPIPE_SZ)) != size)
		fcntl(fd, F_SETPIPE_SZ, size);

# if 0
	char    buf[BUFSIZ];

	if((cursize = fcntl(fd, F_GETPIPE_SZ)) >= ONE_MiB)
		snprintf(buf, BUFSIZ, "%ldMiB", MiB(cursize));
	else
		snprintf(buf, BUFSIZ, "%ldKiB", KiB(cursize));

	fprintf(stderr, "%s: %s pipesize: %s\n", section, base(pipename), buf);
# endif

	close(fd);

#endif											/* F_SETPIPE_SZ */
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
