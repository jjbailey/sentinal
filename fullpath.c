/*
 * fullpath.c
 * return the full path of a file
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include "sentinal.h"

char   *fullpath(char *dir, char *file, char *path)
{
	if(*dir && *file) {
		if(*file == '/')
			strlcpy(path, file, PATH_MAX);
		else
			snprintf(path, PATH_MAX, "%s/%s", dir, file);
	}

	return (path);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
