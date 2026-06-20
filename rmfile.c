/*
 * rmfile.c
 * Remove a file or an empty directory.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"

/*
 * Remove obj, which must reside under root (the thread's canonical dirname).
 * Walk the path one component at a time from a descriptor on root, opening
 * each intermediate directory with O_NOFOLLOW so a symlink swapped into the
 * path cannot redirect a privileged unlink outside the managed tree. The
 * final component is removed with unlinkat() (AT_REMOVEDIR for a directory),
 * neither of which follows a terminal symlink. Returns 0 on success, -1 on
 * error with errno set.
 */
static int safe_remove(const char *root, const char *obj)
{
	char    relbuf[PATH_MAX];						/* obj relative to root */
	char   *comp;									/* current path component */
	char   *slash;
	int     dirfd;
	int     nextfd;
	int     ret;
	size_t  rootlen = strlen(root);
	struct stat st;

	/* obj must be strictly under root: root + '/' + relative */

	if(strncmp(obj, root, rootlen) != 0 || obj[rootlen] != '/') {
		errno = EINVAL;
		return (-1);
	}

	if(strlcpy(relbuf, obj + rootlen + 1, sizeof(relbuf)) >= sizeof(relbuf)) {
		errno = ENAMETOOLONG;
		return (-1);
	}

	if(IS_NULL(relbuf)) {
		errno = EINVAL;
		return (-1);
	}

	if((dirfd = open(root, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC)) == -1)
		return (-1);

	/* descend through intermediate directories, refusing symlinks */

	comp = relbuf;

	while((slash = strchr(comp, '/')) != NULL) {
		*slash = '\0';

		if(*comp == '\0' || MY_DIR(comp) || MY_PARENT(comp)) {
			close(dirfd);
			errno = EINVAL;
			return (-1);
		}

		nextfd = openat(dirfd, comp, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
		close(dirfd);

		if(nextfd == -1)
			return (-1);

		dirfd = nextfd;
		comp = slash + 1;
	}

	/* final component: file or empty directory */

	if(*comp == '\0' || MY_DIR(comp) || MY_PARENT(comp)) {
		close(dirfd);
		errno = EINVAL;
		return (-1);
	}

	/* decide file vs directory without following a terminal symlink */

	if(fstatat(dirfd, comp, &st, AT_SYMLINK_NOFOLLOW) == -1) {
		close(dirfd);
		return (-1);
	}

	ret = unlinkat(dirfd, comp, S_ISDIR(st.st_mode) ? AT_REMOVEDIR : 0);
	close(dirfd);
	return (ret);
}

bool rmfile(struct thread_info *ti, const char *obj, const char *remark)
{
	extern bool dryrun;

	if(!dryrun && safe_remove(ti->ti_dirname, obj) != 0) {
		int     errnum = errno;

		if(!ti->ti_terse)
			fprintf(stderr, "%s: error %s %s: %s\n",
					ti->ti_section, remark, obj, strerror(errnum));

		sleep(1);
		return (false);
	}

	if(!ti->ti_terse)
		fprintf(stderr, "%s: %s %s\n", ti->ti_section, remark, obj);

	return (true);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
