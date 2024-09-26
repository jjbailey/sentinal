/*
 * logname.c
 * Generate a logfile name based on the current time.
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
#include <time.h>
#include "sentinal.h"

#define	_FULL	"%F"
#define	_YMD	"%Y-%m-%d"

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
	time_t  curtime;								/* now */

	/*
	 * filename becomes the completed template
	 * example: console-%Y-%m-%d_%H-%M-%S.log.zst
	 *          console-%F_%H-%M-%S.log.zst
	 */

	strlcpy(filename, template, PATH_MAX);

	time(&curtime);
	localtime_r(&curtime, &tbuf);

	strreplace(filename, _FULL, _YMD, PATH_MAX);
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

	char    valbuf[PATH_MAX];

	if(IS_NULL(token) || *token != '%')
		return;

	if(strstr(template, token) == NULL)
		return;

	snprintf(valbuf, PATH_MAX, "%02ld", value);
	strreplace(template, token, valbuf, PATH_MAX);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
