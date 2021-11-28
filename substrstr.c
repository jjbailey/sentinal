/*
 * substrstr.c
 * Replace all occurrences of token with filename.
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include "sentinal.h"

void substrstr(char *template, char *token, char *filename)
{
	char    newbuf[BUFSIZ];
	char    search[BUFSIZ];
	char   *p;

	if(IS_NULL(token) || *token != '%')
		return;

	if(strstr(template, token) == NULL)
		return;

	strlcpy(search, template, BUFSIZ);

	while(p = strstr(search, token)) {
		memset(newbuf, '\0', BUFSIZ);
		strlcpy(newbuf, search, (p - search) + 1);
		strlcat(newbuf, filename, BUFSIZ);
		strlcat(newbuf, p + 2, BUFSIZ);
		strlcpy(search, newbuf, BUFSIZ);
	}

	strlcpy(template, search, BUFSIZ);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
