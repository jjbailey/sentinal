/*
 * strreplace.c
 * Replace all occurrences of oldstr with newstr.
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

#ifndef	IS_NULL
# define	IS_NULL(s) !((s) && *(s))
#endif

void strreplace(char *template, const char *oldstr, const char *newstr, size_t size)
{
	char   *p;
	char    replbuf[BUFSIZ];
	char    tempbuf[BUFSIZ];
	size_t  oldlen = strlen(oldstr);
	size_t  newlen = strlen(newstr);
	size_t  strlcat(char *, const char *, size_t);
	size_t  strlcpy(char *, const char *, size_t);

	if(IS_NULL(oldstr) || IS_NULL(newstr))
		return;

	strlcpy(tempbuf, template, size);
	p = tempbuf;

	while((p = strstr(p, oldstr)) != NULL) {		/* clang warns */
		strlcpy(replbuf, tempbuf, (p - tempbuf) + 1);
		strlcat(replbuf, newstr, size);
		strlcat(replbuf, p + oldlen, size);
		strlcpy(tempbuf, replbuf, size);
		p += newlen;
	}

	strlcpy(template, tempbuf, size);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
