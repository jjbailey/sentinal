/*
 * logsize.c
 * Return the number of bytes represented in the INI file spec.
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <ctype.h>
#include <stdlib.h>
#include "sentinal.h"

off_t logsize(char *str)
{
	char   *p;
	off_t   n;

	n = (off_t) abs(atol(str));

	for(p = str; *p; p++)
		if(isalpha(*p))
			break;

	if(IS_NULL(p))
		return (n);

	switch (toupper(*p)) {

	case 'K':
		return (n * (off_t) ONE_KiB);

	case 'M':
		return (n * (off_t) ONE_MiB);

	case 'B':
	case 'G':
		return (n * (off_t) ONE_GiB);
	}

	return (n);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
