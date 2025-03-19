/*
 * basename.h
 * replace basename(3) with inline code
 * assumes no trailing slash
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#ifndef BASE_INLINE
# define	BASE_INLINE

# include <string.h>

static inline char *base(char *s)
{
	char   *p = strrchr(s, '/');
	return (p ? p + 1 : s);
}

#endif												/* BASE_INLINE */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
