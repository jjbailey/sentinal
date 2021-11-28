/*
 * iniget.c
 * functions to read INI files
 *
 * Copyright (c) 2021 jjb
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
			if(strcmp(p + 1, "global") == 0)	/* not a thread */
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

	printf("[%s]\n", section);
	printf("command  = %s\n", my_ini(inidata, section, "command"));
	printf("dirname  = %s\n", my_ini(inidata, section, "dirname"));
	printf("subdirs  = %s\n", my_ini(inidata, section, "subdirs"));
	printf("pipename = %s\n", my_ini(inidata, section, "pipename"));
	printf("template = %s\n", my_ini(inidata, section, "template"));
	printf("pcrestr  = %s\n", my_ini(inidata, section, "pcrestr"));
	printf("uid      = %s\n", my_ini(inidata, section, "uid"));
	printf("gid      = %s\n", my_ini(inidata, section, "gid"));
	printf("loglimit = %s\n", my_ini(inidata, section, "loglimit"));
	printf("diskfree = %s\n", my_ini(inidata, section, "diskfree"));
	printf("inofree  = %s\n", my_ini(inidata, section, "inofree"));
	printf("expire   = %s\n", my_ini(inidata, section, "expire"));
	printf("retmin   = %s\n", my_ini(inidata, section, "retmin"));
	printf("retmax   = %s\n", my_ini(inidata, section, "retmax"));
	printf("postcmd  = %s\n", my_ini(inidata, section, "postcmd"));
	printf("\n");
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
