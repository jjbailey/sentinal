/*
 * logsize.c
 * Return the number of bytes represented in the INI file spec.
 *
 * Copyright (c) 2021-2023 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <ctype.h>
#include "sentinal.h"

/* SI units */
#define	ONE_KiB		(off_t)1024						/* K, k, Ki{B,F} */
#define	ONE_MiB		(ONE_KiB << 10)					/* M, m, Mi{B,F} */
#define	ONE_GiB		(ONE_MiB << 10)					/* G, g, Gi{B,F} */
#define	ONE_TiB		(ONE_GiB << 10)					/* T, t, Ti{B,F} */

/* non-SI units */
#define	ONE_KB		(off_t)1000						/* K{B,F} */
#define	ONE_MB		(ONE_KB * 1000)					/* M{B,F} */
#define	ONE_GB		(ONE_MB * 1000)					/* G{B,F} */
#define	ONE_TB		(ONE_GB * 1000)					/* T{B,F} */

/* is non-SI unit */
#define	UNIT(p)		(*(p + 1))
#define	IS_NSI(p)	(UNIT(p) && (toupper(UNIT(p)) == 'B' || toupper(UNIT(p)) == 'F'))

off_t logsize(char *str)
{
	char   *p;
	off_t   n;

	n = (off_t) labs(atol(str));

	for(p = str; *p; p++)
		if(isalpha(*p))
			break;

	if(IS_NULL(p))
		return (n);

	switch (toupper(*p)) {

	case 'B':
	case 'F':
		return (n);

	case 'K':
		return (n * (off_t) IS_NSI(p) ? ONE_KB : ONE_KiB);

	case 'M':
		return (n * (off_t) IS_NSI(p) ? ONE_MB : ONE_MiB);

	case 'G':
		return (n * (off_t) IS_NSI(p) ? ONE_GB : ONE_GiB);

	case 'T':
		return (n * (off_t) IS_NSI(p) ? ONE_TB : ONE_TiB);
	}

	return (n);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
