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

void strreplace(char *template, char *oldstr, char *newstr)
{
	char    newbuf[BUFSIZ];
	char   *p;
	size_t  len;
	size_t  strlcat(char *, const char *, size_t);
	size_t  strlcpy(char *, const char *, size_t);

	if(IS_NULL(oldstr) || IS_NULL(newstr))			/* null */
		return;

	if(strstr(newstr, oldstr))						/* recursion */
		return;

	len = strlen(oldstr);
	memset(newbuf, '\0', BUFSIZ);

	while((p = strstr(template, oldstr))) {
		strlcpy(newbuf, template, (p - template) + 1);	/* add NUL */
		strlcat(newbuf, newstr, BUFSIZ);
		strlcat(newbuf, p + len, BUFSIZ);
		strlcpy(template, newbuf, BUFSIZ);
	}
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
