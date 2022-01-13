/*
 * version.c
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include "basename.h"

#define	VERSION_STRING	"1.2.0"

void version(char *prog)
{
	fprintf(stderr, "%s: version %s\n", base(prog), VERSION_STRING);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
