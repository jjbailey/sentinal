/*
 * strreplace.c
 * Replace all occurrences of oldstr with newstr.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#ifndef	IS_NULL
# define	IS_NULL(s) !((s) && *(s))
#endif

void strreplace(char *template, const char *oldstr, const char *newstr, size_t size)
{
	size_t  strlcat(char *, const char *, size_t);
	size_t  strlcpy(char *, const char *, size_t);

	if(IS_NULL(template) || IS_NULL(oldstr) || newstr == NULL)
		return;

	size_t  newlen = strlen(newstr);
	size_t  oldlen = strlen(oldstr);

	char   *tempbuf = malloc(size);

	if(!tempbuf)
		return;

	strlcpy(tempbuf, template, size);

	char   *result = malloc(size);

	if(!result) {
		free(tempbuf);
		return;
	}

	char   *src = tempbuf;
	char   *dst = result;
	char   *p;

	while((p = strstr(src, oldstr)) != NULL) {
		size_t  prefix_len = p - src;

		if(dst + prefix_len + newlen >= result + size - 1)
			break;

		memcpy(dst, src, prefix_len);
		dst += prefix_len;
		memcpy(dst, newstr, newlen);
		dst += newlen;
		src = p + oldlen;
	}

	// Copy remaining

	strlcpy(dst, src, (result + size) - dst);
	strlcpy(template, result, size);

	free(result);
	free(tempbuf);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
