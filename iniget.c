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

char   *NullStr = "";							/* empty string for strdup, etc */

char   *my_ini(ini_t * ini, char *section, char *key)
{
	char   *p;
	char   *sp;

	if((p = ini_get(ini, section, key)) == NULL)
		return (NullStr);

	/* remove trailing slash */

	sp = (p + strlen(p)) - 1;

	if(*sp == '/')
		*sp = '\0';

	return (p);
}

int get_sections(ini_t * inidata, int maxsect, char *sections[])
{
	/* returns a list of INI sections */

	char   *p;
	int     i = 0;

	for(p = inidata->data; p < inidata->end; p++)
		if(*p == '[') {
			if(p > inidata->data && *(p - 1))	/* line does not start with [ */
				continue;

			if(strcmp(p + 1, "global") == 0)	/* global is not a thread */
				continue;

			sections[i++] = strdup(p + 1);

			if(i == maxsect)
				break;
		}

	return (i);
}

void print_section(ini_t * inidata, char *section)
{
	/* dumps an INI section */
	/* debugging only */

	fprintf(stdout, "\n[%s]\n", section);
	fprintf(stdout, "command  = %s\n", my_ini(inidata, section, "command"));
	fprintf(stdout, "dirname  = %s\n", my_ini(inidata, section, "dirname"));
	fprintf(stdout, "subdirs  = %s\n", my_ini(inidata, section, "subdirs"));
	fprintf(stdout, "pipename = %s\n", my_ini(inidata, section, "pipename"));
	fprintf(stdout, "template = %s\n", my_ini(inidata, section, "template"));
	fprintf(stdout, "pcrestr  = %s\n", my_ini(inidata, section, "pcrestr"));
	fprintf(stdout, "uid      = %s\n", my_ini(inidata, section, "uid"));
	fprintf(stdout, "gid      = %s\n", my_ini(inidata, section, "gid"));
	fprintf(stdout, "loglimit = %s\n", my_ini(inidata, section, "loglimit"));
	fprintf(stdout, "diskfree = %s\n", my_ini(inidata, section, "diskfree"));
	fprintf(stdout, "inofree  = %s\n", my_ini(inidata, section, "inofree"));
	fprintf(stdout, "expire   = %s\n", my_ini(inidata, section, "expire"));
	fprintf(stdout, "retmin   = %s\n", my_ini(inidata, section, "retmin"));
	fprintf(stdout, "retmax   = %s\n", my_ini(inidata, section, "retmax"));
	fprintf(stdout, "postcmd  = %s\n", my_ini(inidata, section, "postcmd"));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
