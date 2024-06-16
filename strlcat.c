/*
 * strlcat.c
 * NUL terminate when dsize > 0 and something is copied.
 * dsize needs to include space for NUL.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/types.h>

size_t strlcat(char *dst, char *src, size_t dsize)
{
	char   *dp = dst;
	char   *sp = src;

	while(*dp && dsize > 0) {						/* find the end */
		dp++;
		dsize--;
	}

	while(*sp && dsize > 0) {						/* copy */
		*dp++ = *sp++;
		dsize--;
	}

	if(dp > dst && dsize == 0)						/* make room for NUL */
		dp--;

	if(dp > dst)
		*dp = '\0';

	return (dp - dst);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
