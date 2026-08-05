/*
 * iniget.c
 * Functions to read INI files.
 * Wrapper for rxi ini_get() along with a couple of utility functions.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "ini.h"

#ifndef PATH_MAX
# define	PATH_MAX	255
#endif

#ifndef	NOT_NULL
# define	NOT_NULL(s)	((s) && *(s))
#endif

#ifndef	STREQ
# define	STREQ(s1,s2) (strcmp(s1, s2) == 0)
#endif

static bool duplicate(int, char *, char **);

char   *EmptyStr = "";								/* empty string for strndup, etc */

char   *my_ini(ini_t *ini, char *section, char *key)
{
	char   *p;
	char   *sp;
	size_t  len;

	if((p = ini_get(ini, section, key)) == NULL)
		return (EmptyStr);

	len = strlen(p);

	if(len > 1) {
		sp = (p + len) - 1;

		if(*sp == '/')
			*sp = '\0';
	}

	return (p);
}

/* advance to the next '\0'-delimited token, same rule ini_get() uses */
static char *sections_next(char *end, char *p)
{
	p += strlen(p);

	while(p < end && *p == '\0')
		p++;

	return p;
}

int get_sections(ini_t *inidata, int maxsect, char *sections[])
{
	/* returns a list of INI sections */

	char   *end = inidata->end;
	char   *p = inidata->data;
	int     i = 0;
	bool    validdbname(char *);

	if(p < end && *p == '\0')
		p = sections_next(end, p);

	while(p < end) {
		if(*p == '[') {
			if(!STREQ(p + 1, "global") &&			/* global is not a thread */
			   validdbname(p + 1) &&				/* for sqlite3 */
			   !duplicate(i, p + 1, sections)) {	/* section already exists */

				if(i == maxsect) {
					fprintf(stderr, "too many sections: max %d\n", maxsect);
					return (-1);
				}

				sections[i++] = strndup(p + 1, PATH_MAX);
			}

			p = sections_next(end, p);
		} else {
			/* key: skip both the key and its paired value token */
			p = sections_next(end, p);
			p = sections_next(end, p);
		}
	}

	return (i);
}

static bool duplicate(int nsect, char *s, char *sections[])
{
	int     i;

	for(i = 0; i < nsect; i++)
		if(STREQ(s, sections[i]))
			return (true);

	return (false);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
