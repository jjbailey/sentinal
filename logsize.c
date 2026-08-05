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

#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include "sentinal.h"

/* largest value representable in a signed off_t */

#define	OFF_T_MAX	((long long)((((uintmax_t) 1) << (sizeof(off_t) * CHAR_BIT - 1)) - 1))

/* SI units */
#define	ONE_KiB		1024LL							/* K, k, Ki{B,F} */
#define	ONE_MiB		(ONE_KiB << 10)					/* M, m, Mi{B,F} */
#define	ONE_GiB		(ONE_MiB << 10)					/* G, g, Gi{B,F} */
#define	ONE_TiB		(ONE_GiB << 10)					/* T, t, Ti{B,F} */
#define	ONE_PiB		(ONE_TiB << 10)					/* P, p, Pi{B,F} */
#define	ONE_EiB		(ONE_PiB << 10)					/* E, e, Ei{B,F} */

/* non-SI units */
#define	ONE_KB		1000LL							/* K{B,F} */
#define	ONE_MB		(ONE_KB * 1000)					/* M{B,F} */
#define	ONE_GB		(ONE_MB * 1000)					/* G{B,F} */
#define	ONE_TB		(ONE_GB * 1000)					/* T{B,F} */
#define	ONE_PB		(ONE_TB * 1000)					/* P{B,F} */
#define	ONE_EB		(ONE_PB * 1000)					/* E{B,F} */

off_t logsize(char *str)
{
	char   *endptr;
	long long n;
	long long mult;

	if(!str)
		return 0;

	n = strtoll(str, &endptr, 10);
	if(endptr == str)
		return 0;

	if(n < 0) {
		if(n == LLONG_MIN) {
			/* negating LLONG_MIN overflows and stays negative; a
			 * negative limit reads as "always exceeded" downstream */
			fprintf(stderr, "%s: size out of range, ignored\n", str);
			return (off_t) 0;
		}

		n = -n;
	}

	if(!*endptr)
		return (off_t) n;

	bool    isNsi = (endptr[1] &&
					 (toupper(endptr[1]) == 'B' || toupper(endptr[1]) == 'F'));

	switch (toupper(*endptr)) {

	case 'K':
		mult = isNsi ? ONE_KB : ONE_KiB;
		break;

	case 'M':
		mult = isNsi ? ONE_MB : ONE_MiB;
		break;

	case 'G':
		mult = isNsi ? ONE_GB : ONE_GiB;
		break;

	case 'T':
		mult = isNsi ? ONE_TB : ONE_TiB;
		break;

	case 'P':
		mult = isNsi ? ONE_PB : ONE_PiB;
		break;

	case 'E':
		mult = isNsi ? ONE_EB : ONE_EiB;
		break;

	case 'B':
	case 'F':
	default:
		return (off_t) n;
	}

	if(n > OFF_T_MAX / mult) {
		fprintf(stderr, "%s: size out of range, ignored\n", str);
		return (off_t) 0;
	}

	return (off_t) (n * mult);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
