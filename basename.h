/*
 * basename.h
 * replace basename(3) with inline code
 * assumes no trailing slash
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#ifndef _BASE_INLINE
# define	_BASE_INLINE

# include <string.h>

static inline char *base(char *s)
{
	char   *p = strrchr(s, '/');
	return (p ? p + 1 : s);
}

#endif												/* _BASE_INLINE */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
