/*
 * fullpath.c
 * Return the full path of a file.
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include "sentinal.h"

char   *fullpath(const char *dir, const char *file, char *path)
{
	if(NOT_NULL(dir) && NOT_NULL(file)) {
		if(*file == '/')
			strlcpy(path, file, PATH_MAX);
		else
			snprintf(path, PATH_MAX, "%s/%s", dir, file);
	} else {
		/* program error */
		*path = '\0';
	}

	return (path);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
