/*
 * workcmd.c
 * Create a command in zargv[] for execv().
 *
 * Copyright (c) 2021-2025 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <string.h>
#include "sentinal.h"
#include "basename.h"

int workcmd(int argc, char *argv[], char *zargv[])
{
	char   *baseName;
	int     i;
	int     n = 0;

	static const char *compprogs[] = {
		"bzip2", "gzip", "lzip", "pbzip2",
		"pigz", "pzstd", "xz", "zstd"
	};

	static const int ncprogs = sizeof(compprogs) / sizeof(compprogs[0]);

	if(argc < 1 || !argv || !zargv)
		return (0);

	baseName = base(argv[0]);

	if(IS_NULL(baseName))
		return (0);

	zargv[n++] = baseName;

	/* sorry, we're going to add/enforce -f for compression programs */

	for(i = 0; i < ncprogs; i++)
		if(strcmp(zargv[0], compprogs[i]) == 0) {
			zargv[n++] = "-f";
			break;
		}

	/* copy the remaining command line options from the INI file */

	for(i = 1; i < argc && n < (MAXARGS - 1); ++i)
		zargv[n++] = argv[i];

	zargv[n] = NULL;
	return (n);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
