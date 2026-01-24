/*
 * logsize.c
 * Return the number of bytes represented in the INI file spec.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <ctype.h>
#include <stdlib.h>
#include "sentinal.h"

/* SI units */
#define	ONE_KiB		1024LL							/* K, k, Ki{B,F} */
#define	ONE_MiB		(ONE_KiB << 10)					/* M, m, Mi{B,F} */
#define	ONE_GiB		(ONE_MiB << 10)					/* G, g, Gi{B,F} */
#define	ONE_TiB		(ONE_GiB << 10)					/* T, t, Ti{B,F} */

/* non-SI units */
#define	ONE_KB		1000LL							/* K{B,F} */
#define	ONE_MB		(ONE_KB * 1000)					/* M{B,F} */
#define	ONE_GB		(ONE_MB * 1000)					/* G{B,F} */
#define	ONE_TB		(ONE_GB * 1000)					/* T{B,F} */

off_t logsize(char *str)
{
	char   *endptr;
	long long n;

	if(!str)
		return 0;

	n = strtoll(str, &endptr, 10);
	if(endptr == str)
		return 0;

	n = n < 0 ? -n : n;

	if(!*endptr)
		return (off_t) n;

	bool    isNsi = (endptr[1] &&
					 (toupper(endptr[1]) == 'B' || toupper(endptr[1]) == 'F'));

	switch (toupper(*endptr)) {

	case 'B':
	case 'F':
		return (off_t) n;

	case 'K':
		return (off_t) (n * (isNsi ? ONE_KB : ONE_KiB));

	case 'M':
		return (off_t) (n * (isNsi ? ONE_MB : ONE_MiB));

	case 'G':
		return (off_t) (n * (isNsi ? ONE_GB : ONE_GiB));

	case 'T':
		return (off_t) (n * (isNsi ? ONE_TB : ONE_TiB));
	}

	return (off_t) n;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
