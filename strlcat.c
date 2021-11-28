/*
 * strlcat.c
 * NUL terminate when dsize > 0 and something is copied.
 *
 * Copyright (c) 2021 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/types.h>

size_t strlcat(char *dst, char *src, size_t dsize)
{
	char   *sp = src;
	char   *dp = dst;
	size_t  dleft = dsize - 1;

	if(dleft > 0)
		while(*dp && dleft) {
			dp++;
			dleft--;
		}

	if(dleft > 0)
		while(*sp && dleft) {
			*dp++ = *sp++;
			dleft--;
		}

	if(dsize && dp != dst)
		*dp = '\0';

	return (dp - dst);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
