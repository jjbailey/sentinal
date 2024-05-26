/*
 * iniget.c
 * Functions to read INI files.
 * Wrapper for rxi ini_get() along with a couple of utility functions.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include "ini.h"

#ifndef TRUE
# define	TRUE		(short)1
#endif

#ifndef	FALSE
# define	FALSE		(short)0
#endif

#ifndef PATH_MAX
# define	PATH_MAX	255
#endif

#ifndef	NOT_NULL
# define	NOT_NULL(s)	((s) && *(s))
#endif

#ifndef	STREQ
# define	STREQ(s1,s2) (strcmp(s1, s2) == 0)
#endif

static short duplicate(int, char *, char **);


char   *EmptyStr = "";								/* empty string for strndup, etc */

char   *my_ini(ini_t *ini, char *section, char *key)
{
	char   *p;
	char   *sp;

	if((p = ini_get(ini, section, key)) == NULL)
		return (EmptyStr);

	/* remove trailing slash */

	sp = (p + strlen(p)) - 1;

	if(*sp == '/')
		*sp = '\0';

	return (p);
}

int get_sections(ini_t *inidata, int maxsect, char *sections[])
{
	/* returns a list of INI sections */

	char   *p;
	int     i = 0;
	short   validdbname(char *);

	for(p = inidata->data; p < inidata->end; p++)
		if(*p == '[') {
			if(p > inidata->data && *(p - 1))		/* line does not start with [ */
				continue;

			if(strchr(p, ']'))						/* parser bug: not a real section name */
				continue;

			if(STREQ(p + 1, "global"))				/* global is not a thread */
				continue;

			if(duplicate(i, p + 1, sections))
				continue;

			if(validdbname(p + 1) == FALSE)
				continue;

			sections[i++] = strndup(p + 1, PATH_MAX);

			if(i == maxsect)
				break;
		}

	return (i);
}

static short duplicate(int nsect, char *s, char *sections[])
{
	int     i;

	for(i = 0; i < nsect; i++)
		if(STREQ(s, sections[i]))
			return (TRUE);

	return (FALSE);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
