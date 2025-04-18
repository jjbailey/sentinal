/*
 * strlcpy.c
 * Copy a string, return the length of the new string.
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/types.h>
#include <string.h>

size_t strlcpy(char *dst, const char *src, size_t size)
{
	strncpy(dst, src, size - 1);
	dst[size - 1] = '\0';
	return (strlen(dst));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
