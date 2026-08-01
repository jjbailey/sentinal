/*
 * openini.c
 * Open the INI file and validate its type, ownership, and permissions.
 * Returns a descriptor the caller parses directly, never a path to reopen.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include "sentinal.h"

/* the INI dictates what commands run and as which uid, so anyone who can */
/* write it is fully trusted: owner must be root or self, mode at most 0644 */

#define	INIMODE		(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int openini(char *myname, char *inifile, char *inipath)
{
	int     inifd;									/* file descriptor */
	int     openflags = O_RDONLY;
	struct stat stbuf;								/* file status */

	if(lstat(inifile, &stbuf) == 0)
		if(S_ISLNK(stbuf.st_mode)) {
			fprintf(stderr, "%s: %s is a symlink\n", myname, inifile);
			return (-1);
		}

	if(realpath(inifile, inipath) == NULL) {
		fprintf(stderr, "%s: can't resolve %s\n", myname, inifile);
		return (-1);
	}

#ifdef O_NOFOLLOW
	openflags |= O_NOFOLLOW;
#endif

	if((inifd = open(inipath, openflags)) == -1) {
		fprintf(stderr, "%s: can't open %s\n", myname, inipath);
		return (-1);
	}

	/* validate the descriptor, not the name: closes the TOCTOU window */

	if(fstat(inifd, &stbuf) == -1) {
		fprintf(stderr, "%s: can't stat %s\n", myname, inipath);
		close(inifd);
		return (-1);
	}

	if(!S_ISREG(stbuf.st_mode)) {
		fprintf(stderr, "%s: %s is not a regular file\n", myname, inipath);
		close(inifd);
		return (-1);
	}

	if(stbuf.st_uid != 0 && stbuf.st_uid != geteuid()) {
		fprintf(stderr, "%s: %s not owned by root or self\n", myname, inipath);
		close(inifd);
		return (-1);
	}

	if((stbuf.st_mode & 07777) & ~INIMODE) {
		fprintf(stderr, "%s: %s permissions exceed 0644\n", myname, inipath);
		close(inifd);
		return (-1);
	}

	return (inifd);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
