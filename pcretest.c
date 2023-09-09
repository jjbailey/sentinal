/*
 * pcretest.c
 * Program to test Perl-compatible regular expressions
 * against a list of strings.
 *
 * If this executable is called "pcretest", be verbose,
 * else terse (just print matches)
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#include <stdio.h>
#include <sys/types.h>
#include "sentinal.h"
#include "basename.h"

short   pcrematch(struct thread_info *, char *);

int main(int argc, char *argv[])
{
	char   *myname;
	int     i;
	short   match;
	struct thread_info ti;							/* so we can use pcrecompile.c */

	myname = base(argv[0]);

	if(argc > 1 && strcmp(argv[1], "-V") == 0) {
		fprintf(stdout, "%s: version %s\n", myname, VERSION_STRING);
		exit(EXIT_SUCCESS);
	}

	if(argc < 3 || IS_NULL(argv[1])) {
		fprintf(stderr, "Usage: %s <pcre> <string> [ <string> ... ]\n", myname);
		exit(EXIT_FAILURE);
	}

	memset(&ti, '\0', sizeof(struct thread_info));

	ti.ti_section = myname;
	ti.ti_pcrestr = argv[1];
	ti.ti_terse = strcmp(myname, "pcretest");		/* decides what to print */

	if(pcrecompile(&ti) == FALSE)
		exit(EXIT_FAILURE);

	for(i = 2; i < argc; i++) {
		match = pcrematch(&ti, argv[i]);

		if(!ti.ti_terse)
			fprintf(stdout, "%s %s\n", match ? "match:   " : "no match:", argv[i]);
		else if(match)
			fprintf(stdout, "%s\n", argv[i]);
	}

	exit(EXIT_SUCCESS);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
