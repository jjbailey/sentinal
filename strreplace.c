/*
 * strreplace.c
 * Replace all occurrences of oldstr with newstr.
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#ifndef IS_NULL
# define IS_NULL(s) !((s) && *(s))
#endif

size_t  strlcat(char *, const char *, size_t);
size_t  strlcpy(char *, const char *, size_t);

void strreplace(char *template, char *oldstr, char *newstr)
{
	char    newbuf[BUFSIZ];
	char   *p;
	int     len;

	if(IS_NULL(oldstr) || IS_NULL(newstr))
		return;

	if(strcmp(oldstr, newstr) == 0 || strstr(newstr, oldstr))
		return;

	len = strlen(oldstr);
	memset(newbuf, '\0', BUFSIZ);

	while(p = strstr(template, oldstr)) {
		strlcpy(newbuf, template, (p - template) + 1);
		strlcat(newbuf, newstr, BUFSIZ);
		strlcat(newbuf, p + len, BUFSIZ);
		strlcpy(template, newbuf, BUFSIZ);
	}
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
