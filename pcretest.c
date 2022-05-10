/*
 * pcretest.c
 * Program to test Perl-compatible regular expressions against a list of strings.
 *
 * Copyright (c) 2021, 2022 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 *
 * examples of usage:
 *
 * ends with a digit
 * $ pcretest '/var/log/sys.*\d$' /var/log/sys*
 * no match: /var/log/syslog
 * match:    /var/log/syslog.1
 * no match: /var/log/syslog.2.gz
 *
 * negative lookahead for pid files
 * $ pcretest '(?!.*\.(?:pid)$)' notapidfile pidfile.pid
 * match:    notapidfile
 * no match: pidfile.pid
 *
 * negative lookbehind for pid files
 * $ pcretest '.*(?<!\.pid)$' notapidfile pidfile.pid
 * match:    notapidfile
 * no match: pidfile.pid
 *
 * negative lookahead for compressed files
 * $ find testdir -type f | xargs pcretest '(?!.*\.(?:bz2|gz|lz|zip|zstd)$)'
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
void    version(char *, FILE *);

int main(int argc, char *argv[])
{
	struct thread_info ti;							/* so we can use pcrecompile.c */
	int     i;

	if(argc < 3) {
		fprintf(stderr, "Usage: pcretest <pcre> <list_of_test_strings>\n");
		exit(1);
	}

	ti.ti_section = base(argv[0]);
	ti.ti_pcrestr = argv[1];
	pcrecompile(&ti);

	if(ti.ti_pcrecmp == NULL)
		exit(1);

	for(i = 2; i < argc; i++) {
		if(pcretest(argv[i], ti.ti_pcrecmp))
			fprintf(stdout, "match:    %s\n", argv[i]);
		else
			fprintf(stdout, "no match: %s\n", argv[i]);
	}

	exit(0);
}

short pcretest(char *s, pcre2_code *re)
{
	/* mostly copied from mylogfile.c */

	int     rc;
	pcre2_match_data *mdata;
	uint32_t options = PCRE2_ANCHORED;

	mdata = pcre2_match_data_create_from_pattern(re, NULL);
	rc = pcre2_match(re, (PCRE2_SPTR) s, strlen(s), (PCRE2_SIZE) 0, options, mdata, NULL);
	pcre2_match_data_free(mdata);

	return (rc >= 0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
