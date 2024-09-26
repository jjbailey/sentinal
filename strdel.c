/*
 * strdel.c
 * Delete a substring from a string, return the length of the new string.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>

size_t strdel(char *dst, const char *src, const char *substr, size_t size)
{
	char   *pos;
	size_t  sslen = strlen(substr);
	size_t  strlcpy(char *, const char *, size_t);

	strlcpy(dst, src, size);

	while((pos = strstr(dst, substr)) != NULL)		/* clang warns */
		memmove(pos, pos + sslen, strlen(pos + sslen) + 1);

	return (strlen(dst));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
