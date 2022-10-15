/*
 * iniget.c
 * Functions to read INI files.
 * Wrapper for rxi ini_get() along with a couple of utility functions.
 *
 * Copyright (c) 2021, 2022 jjb
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

#ifndef	NOT_NULL
# define	NOT_NULL(s)	((s) && *(s))
#endif

#ifndef	STREQ
# define	STREQ(s1,s2) (strcmp(s1, s2) == 0)
#endif

static short duplicate(int, char *, char **);
short   validdbname(char *);

char   *EmptyStr = "";								/* empty string for strdup, etc */

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

			sections[i++] = strdup(p + 1);

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

void print_global(ini_t *inidata, char *inifile)
{
	fprintf(stdout, "# %s\n\n", inifile);
	fprintf(stdout, "[global]\n");
	fprintf(stdout, "pidfile   = %s\n", my_ini(inidata, "global", "pidfile"));
	fprintf(stdout, "database  = %s\n", my_ini(inidata, "global", "database"));
}

void print_section(ini_t *inidata, char *section)
{
	/* dumps an INI section */
	/* debugging only */

	fprintf(stdout, "\n[%s]\n", section);
	fprintf(stdout, "command   = %s\n", my_ini(inidata, section, "command"));
	fprintf(stdout, "dirname   = %s\n", my_ini(inidata, section, "dirname"));
	fprintf(stdout, "dirlimit  = %s\n", my_ini(inidata, section, "dirlimit"));
	fprintf(stdout, "subdirs   = %s\n", my_ini(inidata, section, "subdirs"));
	fprintf(stdout, "pipename  = %s\n", my_ini(inidata, section, "pipename"));
	fprintf(stdout, "template  = %s\n", my_ini(inidata, section, "template"));
	fprintf(stdout, "pcrestr   = %s\n", my_ini(inidata, section, "pcrestr"));
	fprintf(stdout, "uid       = %s\n", my_ini(inidata, section, "uid"));
	fprintf(stdout, "gid       = %s\n", my_ini(inidata, section, "gid"));
	fprintf(stdout, "rotatesiz = %s\n", my_ini(inidata, section, "rotatesiz"));
	fprintf(stdout, "expiresiz = %s\n", my_ini(inidata, section, "expiresiz"));
	fprintf(stdout, "diskfree  = %s\n", my_ini(inidata, section, "diskfree"));
	fprintf(stdout, "inofree   = %s\n", my_ini(inidata, section, "inofree"));
	fprintf(stdout, "expire    = %s\n", my_ini(inidata, section, "expire"));
	fprintf(stdout, "retmin    = %s\n", my_ini(inidata, section, "retmin"));
	fprintf(stdout, "retmax    = %s\n", my_ini(inidata, section, "retmax"));
	fprintf(stdout, "terse     = %s\n", my_ini(inidata, section, "terse"));
	fprintf(stdout, "rmdir     = %s\n", my_ini(inidata, section, "rmdir"));
	fprintf(stdout, "symlinks  = %s\n", my_ini(inidata, section, "symlinks"));
	fprintf(stdout, "postcmd   = %s\n", my_ini(inidata, section, "postcmd"));
	fprintf(stdout, "truncate  = %s\n", my_ini(inidata, section, "truncate"));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
