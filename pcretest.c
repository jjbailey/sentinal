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
 *
 * Examples of usage:
 *
 * ends with a digit
 * $ pcretest '/var/log/sys.*\d$' /var/log/sys*
 * no match: /var/log/syslog
 * match:    /var/log/syslog.1
 * no match: /var/log/syslog.2.gz
 *
 * negative lookahead for pid files
 * $ pcretest '^(?!.*\.(?:pid)$)' notapidfile pidfile.pid
 * match:    notapidfile
 * no match: pidfile.pid
 *
 * negative lookbehind for pid files
 * $ pcretest '.*(?<!\.pid)$' notapidfile pidfile.pid
 * match:    notapidfile
 * no match: pidfile.pid
 *
 * negative lookahead for compressed files
 * $ pcretest '^(?!.*\.(?:bz2|gz|lz|zip|zst)$)' $(find testdir -type f)
 * no match: testdir/ddrescue-1.25.tar.lz
 * match:    testdir/nxserver.log
 * no match: testdir/syslog.2.gz
 */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include "sentinal.h"
#include "basename.h"

short   pcretest(char *, pcre2_code *);

int main(int argc, char *argv[])
{
	struct thread_info ti;							/* so we can use pcrecompile.c */
	char   *myname;
	int     i;
	short   match;

	myname = base(argv[0]);

	if(argc > 1 && strcmp(argv[1], "-V") == 0) {
		fprintf(stdout, "%s: version %s\n", myname, VERSION_STRING);
		exit(EXIT_SUCCESS);
	}

	if(argc < 3 || IS_NULL(argv[1])) {
		fprintf(stderr, "Usage: %s <pcre> <list_of_test_strings>\n", myname);
		exit(EXIT_FAILURE);
	}

	ti.ti_section = myname;
	ti.ti_pcrestr = argv[1];
	ti.ti_terse = strcmp(myname, "pcretest");		/* decides what to print */

	if(pcrecompile(&ti) == FALSE)
		exit(EXIT_FAILURE);

	for(i = 2; i < argc; i++) {
		match = pcretest(argv[i], ti.ti_pcrecmp);

		if(!ti.ti_terse)
			fprintf(stdout, "%s %s\n", match ? "match:   " : "no match:", argv[i]);
		else if(match)
			fprintf(stdout, "%s\n", argv[i]);
	}

	exit(EXIT_SUCCESS);
}

short pcretest(char *s, pcre2_code *re)
{
	/* mostly copied from mylogfile.c */

	int     rc;
	pcre2_match_data *mdata;
	uint32_t options = 0;

	if(IS_NULL(s) || re == NULL)
		return (FALSE);

	mdata = pcre2_match_data_create_from_pattern(re, NULL);
	rc = pcre2_match(re, (PCRE2_SPTR) s, strlen(s), (PCRE2_SIZE) 0, options, mdata, NULL);
	pcre2_match_data_free(mdata);

	return (rc >= 0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
