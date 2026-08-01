/*
 * strlcat.c
 * Concatenate two strings, return the length of the new string.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/types.h>
#include <string.h>

size_t strlcat(char *dst, const char *src, size_t size)
{
	const size_t dlen = strlen(dst);
	const size_t srclen = strlen(src);
	size_t  copylen;

	/* return the would-be total length, per BSD strlcat semantics */

	if(size == 0 || dlen >= size)					/* no room to append */
		return (dlen + srclen);

	/* copy only what we keep: strncpy() would null-pad the rest of dst */

	copylen = size - dlen - 1;

	if(srclen < copylen)
		copylen = srclen;

	memcpy(dst + dlen, src, copylen);
	dst[dlen + copylen] = '\0';
	return (dlen + srclen);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
