/*
 * basename.h
 * replace basename(3) with inline code
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
	/* assumes no trailing slash */

	char   *p = s + strlen(s);

	while(p > s)
		if(*--p == '/')
			return (p + 1);

	return (s);
}

#endif												/* _BASE_INLINE */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
