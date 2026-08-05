/*
 * strlcpy.c
 * Copy a string, return the length of the new string.
 *
 * Copyright (c) 2021-2026 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/types.h>
#include <string.h>

size_t strlcpy(char *dst, const char *src, size_t size)
{
	size_t  srclen = strlen(src);
	size_t  copylen;

	if(size == 0)									/* no room to write */
		return (srclen);

	/* copy only what we keep: strncpy() would null-pad all of dst */

	copylen = srclen < size - 1 ? srclen : size - 1;
	memcpy(dst, src, copylen);
	dst[copylen] = '\0';
	return (srclen);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
