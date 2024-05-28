/*
 * strdel.c
 * Delete a substring from a string.
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

size_t strdel(char *dst, char *src, char *del)
{
	char   *pos;
	size_t  dlen = strlen(del);
	size_t  strlcpy(char *, char *, size_t);

	strlcpy(dst, src, BUFSIZ);

	while((pos = strstr(dst, del)) != NULL)
		memmove(pos, pos + dlen, strlen(pos + dlen) + 1);

	return (strlen(dst));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
