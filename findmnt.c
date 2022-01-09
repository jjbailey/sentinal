/*
 * findmnt.c
 * find the mount point for a directory
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <libgen.h>
#include <limits.h>								/* for realpath() */
#include <mntent.h>
#include <string.h>
#include "sentinal.h"

char   *findmnt(char *dir, char *mountdir)
{
	FILE   *fp;
	char   *mp;
	struct mntent *fs;

	if(IS_NULL(dir) || realpath(dir, mountdir) == NULL)
		return (NULL);

	if((fp = setmntent("/etc/mtab", "r")) == NULL)
		return (NULL);

	mp = mountdir;

	while(strlen(mp) > 1) {
		while(fs = getmntent(fp))
			if(strcmp(fs->mnt_dir, mp) == 0) {
				endmntent(fp);
				return (mp);
			}

		mp = dirname(mp);
		rewind(fp);
	}

	endmntent(fp);
	return (mp);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
