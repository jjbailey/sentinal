/*
 * logname.c
 * Generate a logfile name based on the current time.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include "sentinal.h"

#define	_YEAR	"%Y"
#define	_MONTH	"%m"
#define	_DAY	"%d"
#define	_HOUR	"%H"
#define	_MIN	"%M"
#define	_SEC	"%S"
#define	_ESEC	"%s"

static void substrval(char *, char *, time_t);

char   *logname(char *template, char *filename)
{
	struct tm tbuf;
	time_t  curtime;

	/* filename becomes the completed template */
	/* example: console-%Y-%m-%d_%H-%M-%S.log.zst */

	strlcpy(filename, template, PATH_MAX);

	time(&curtime);
	localtime_r(&curtime, &tbuf);

	substrval(filename, _YEAR, tbuf.tm_year + 1900);
	substrval(filename, _MONTH, tbuf.tm_mon + 1);
	substrval(filename, _DAY, tbuf.tm_mday);
	substrval(filename, _HOUR, tbuf.tm_hour);
	substrval(filename, _MIN, tbuf.tm_min);
	substrval(filename, _SEC, tbuf.tm_sec);
	substrval(filename, _ESEC, curtime);

	return (filename);
}

static void substrval(char *template, char *token, time_t value)
{
	/* replace all occurrences of token with value */

	char    newbuf[PATH_MAX];
	char    search[PATH_MAX];
	char    valbuf[PATH_MAX];
	char   *p;
	int     len;

	if(IS_NULL(token) || *token != '%')
		return;

	if(strstr(template, token) == NULL)
		return;

	len = strlen(token);
	strlcpy(search, template, PATH_MAX);

	while(p = strstr(search, token)) {
		memset(newbuf, '\0', PATH_MAX);
		strlcpy(newbuf, search, (p - search) + 1);
		snprintf(valbuf, PATH_MAX, "%02ld", value);
		strlcat(newbuf, valbuf, PATH_MAX);
		strlcat(newbuf, p + len, PATH_MAX);
		strlcpy(search, newbuf, PATH_MAX);
	}

	strlcpy(template, search, PATH_MAX);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
