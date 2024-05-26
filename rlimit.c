/*
 * rlimit.c
 * Set max open files.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <sys/resource.h>

void rlimit(int maxfiles)
{
	struct rlimit lim;

	lim.rlim_cur = (rlim_t) maxfiles;
	lim.rlim_max = (rlim_t) maxfiles;

	setrlimit(RLIMIT_NOFILE, &lim);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
