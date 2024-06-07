/*
 * outputs.c
 * Dump INI files in various formats.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/utsname.h>
#include "sentinal.h"
#include "basename.h"
#include "ini.h"

/* so as not to print empty values */
#define	DPRINTNUM(f,s,v)	if(v) fprintf(f, s, v)
#define	DPRINTSTR(f,s,v)	if(NOT_NULL(v)) fprintf(f, s, v)

/* for debug_split() */
static char *thread_types[] = {
	_DFS_THR,
	_EXP_THR,
	_SLM_THR,
	_WRK_THR
};

static int ntypes = sizeof(thread_types) / sizeof(thread_types[0]);

/* externals */
extern struct thread_info tinfo[MAXSECT];			/* our threads */
extern struct utsname utsbuf;						/* for host info */

char   *my_ini(ini_t *, char *, char *);

void debug_global(ini_t *inidata, char *inifile)
{
	/* print the global section */

	fprintf(stdout, "# %s\n\n", inifile);
	fprintf(stdout, "[global]\n");
	fprintf(stdout, "pidfile   = %s\n", my_ini(inidata, "global", "pidfile"));
	fprintf(stdout, "database  = %s\n", my_ini(inidata, "global", "database"));
}

void debug_section(ini_t *inidata, char *section)
{
	/* print all keys for a section name as a single section */

	DPRINTSTR(stdout, "\n[%s]\n", section);
	DPRINTSTR(stdout, "dirname   = %s\n", my_ini(inidata, section, "dirname"));
	DPRINTSTR(stdout, "command   = %s\n", my_ini(inidata, section, "command"));
	DPRINTSTR(stdout, "dirlimit  = %s\n", my_ini(inidata, section, "dirlimit"));
	DPRINTSTR(stdout, "diskfree  = %s\n", my_ini(inidata, section, "diskfree"));
	DPRINTSTR(stdout, "expiresiz = %s\n", my_ini(inidata, section, "expiresiz"));
	DPRINTSTR(stdout, "expire    = %s\n", my_ini(inidata, section, "expire"));
	DPRINTSTR(stdout, "uid       = %s\n", my_ini(inidata, section, "uid"));
	DPRINTSTR(stdout, "gid       = %s\n", my_ini(inidata, section, "gid"));
	DPRINTSTR(stdout, "inofree   = %s\n", my_ini(inidata, section, "inofree"));
	DPRINTSTR(stdout, "pcrestr   = %s\n", my_ini(inidata, section, "pcrestr"));
	DPRINTSTR(stdout, "pipename  = %s\n", my_ini(inidata, section, "pipename"));
	DPRINTSTR(stdout, "postcmd   = %s\n", my_ini(inidata, section, "postcmd"));
	DPRINTSTR(stdout, "retmax    = %s\n", my_ini(inidata, section, "retmax"));
	DPRINTSTR(stdout, "retmin    = %s\n", my_ini(inidata, section, "retmin"));
	DPRINTSTR(stdout, "rmdir     = %s\n", my_ini(inidata, section, "rmdir"));
	DPRINTSTR(stdout, "rotatesiz = %s\n", my_ini(inidata, section, "rotatesiz"));
	DPRINTSTR(stdout, "subdirs   = %s\n", my_ini(inidata, section, "subdirs"));
	DPRINTSTR(stdout, "symlinks  = %s\n", my_ini(inidata, section, "symlinks"));
	DPRINTSTR(stdout, "template  = %s\n", my_ini(inidata, section, "template"));
	DPRINTSTR(stdout, "terse     = %s\n", my_ini(inidata, section, "terse"));
	DPRINTSTR(stdout, "truncate  = %s\n", my_ini(inidata, section, "truncate"));
}

void debug_verbose(struct thread_info *ti)
{
	/* print the section with keys evaluated */

	char    dbuf[BUFSIZ];
	char    ebuf[BUFSIZ];
	char    fbuf[BUFSIZ];
	char   *zargv[MAXARGS];
	int     i;
	int     n;

	*ebuf = *fbuf = '\0';

	DPRINTSTR(stdout, "\n[%s]\n", ti->ti_section);
	DPRINTSTR(stdout, "dirname   = %s\n", ti->ti_dirname);
	DPRINTSTR(stdout, "command   = %s\n", ti->ti_command);

	logname(ti->ti_template, fbuf);
	fullpath(ti->ti_dirname, fbuf, ti->ti_filename);

	if(ti->ti_argc) {
		n = workcmd(ti->ti_argc, ti->ti_argv, zargv);

		fprintf(stdout, "#           ");

		for(i = 0; i < n; i++)
			fprintf(stdout, "%s ", zargv[i]);

		fprintf(stdout, "> %s\n", ti->ti_filename);
	}

	DPRINTSTR(stdout, "dirlimit  = %s\n", ti->ti_dirlimstr);
	DPRINTNUM(stdout, "diskfree  = %.2Lf\n", ti->ti_diskfree);
	DPRINTSTR(stdout, "expiresiz = %s\n", ti->ti_expirestr);
	DPRINTSTR(stdout, "expire    = %s\n", convexpire(ti->ti_expire, ebuf));
	DPRINTNUM(stdout, "uid       = %d\n", ti->ti_uid);
	DPRINTNUM(stdout, "gid       = %d\n", ti->ti_gid);
	DPRINTNUM(stdout, "inofree   = %.2Lf\n", ti->ti_inofree);
	DPRINTSTR(stdout, "pcrestr   = %s\n", ti->ti_pcrestr);
	DPRINTSTR(stdout, "pipename  = %s\n", ti->ti_pipename);
	DPRINTNUM(stdout, "retmax    = %d\n", ti->ti_retmax);
	DPRINTNUM(stdout, "retmin    = %d\n", ti->ti_retmin);
	DPRINTNUM(stdout, "rmdir     = %d\n", ti->ti_rmdir);
	DPRINTSTR(stdout, "rotatesiz = %s\n", ti->ti_rotatestr);

	/* always print subdirs */
	snprintf(dbuf, BUFSIZ, "%d", ti->ti_subdirs);
	DPRINTSTR(stdout, "subdirs   = %s\n", dbuf);

	DPRINTNUM(stdout, "symlinks  = %d\n", ti->ti_symlinks);
	DPRINTSTR(stdout, "template  = %s\n", ti->ti_template);
	DPRINTSTR(stdout, "#           %s\n", base(ti->ti_filename));
	DPRINTNUM(stdout, "terse     = %d\n", ti->ti_terse);

	/* postcmd tokens */
	DPRINTSTR(stdout, "postcmd   = %s\n", ti->ti_postcmd);
	strreplace(ti->ti_postcmd, _HOST_TOK, utsbuf.nodename);
	strreplace(ti->ti_postcmd, _PATH_TOK, ti->ti_dirname);
	strreplace(ti->ti_postcmd, _FILE_TOK, ti->ti_filename);
	strreplace(ti->ti_postcmd, _SECT_TOK, ti->ti_section);
	DPRINTSTR(stdout, "#           %s\n", ti->ti_postcmd);

	DPRINTNUM(stdout, "truncate  = %d\n", ti->ti_truncate);
}

void debug_split(struct thread_info *ti, ini_t *inidata)
{
	/* print the section split into separate thread sections */

	char    delbuf[BUFSIZ];							/* delete string */
	char    pstbuf[BUFSIZ];							/* for copy of postcmd */
	char    secbuf[BUFSIZ];							/* for copy of ti->ti_section */
	int     tt;
	size_t  strdel(char *, char *, char *, size_t);

	char   *command = my_ini(inidata, ti->ti_section, "command");
	char   *dirlimit = my_ini(inidata, ti->ti_section, "dirlimit");
	char   *dirname = my_ini(inidata, ti->ti_section, "dirname");
	char   *diskfree = my_ini(inidata, ti->ti_section, "diskfree");
	char   *expire = my_ini(inidata, ti->ti_section, "expire");
	char   *expiresiz = my_ini(inidata, ti->ti_section, "expiresiz");
	char   *gid = my_ini(inidata, ti->ti_section, "gid");
	char   *inofree = my_ini(inidata, ti->ti_section, "inofree");
	char   *pcrestr = my_ini(inidata, ti->ti_section, "pcrestr");
	char   *pipename = my_ini(inidata, ti->ti_section, "pipename");
	char   *postcmd = my_ini(inidata, ti->ti_section, "postcmd");
	char   *retmax = my_ini(inidata, ti->ti_section, "retmax");
	char   *retmin = my_ini(inidata, ti->ti_section, "retmin");
	char   *rmdir = my_ini(inidata, ti->ti_section, "rmdir");
	char   *rotatesiz = my_ini(inidata, ti->ti_section, "rotatesiz");
	char   *subdirs = my_ini(inidata, ti->ti_section, "subdirs");
	char   *symlinks = my_ini(inidata, ti->ti_section, "symlinks");
	char   *template = my_ini(inidata, ti->ti_section, "template");
	char   *terse = my_ini(inidata, ti->ti_section, "terse");
	char   *truncate = my_ini(inidata, ti->ti_section, "truncate");
	char   *uid = my_ini(inidata, ti->ti_section, "uid");

	for(tt = 0; tt < ntypes; tt++) {
		if(!threadtype(ti, thread_types[tt]))
			continue;

		/*
		 * section names must be unique
		 * remove the thread_type if it's already in the name,
		 * then add the thread_type to the name
		 */

		snprintf(delbuf, BUFSIZ, "_%s", thread_types[tt]);
		strdel(secbuf, ti->ti_section, delbuf, BUFSIZ);
		fprintf(stdout, "\n[%.11s_%.3s]\n", secbuf, thread_types[tt]);

		/*
		 * section name change breaks usage of _SECT_TOK
		 * strreplace it here with the original section name
		 * applies only to ti->ti_postcmd
		 */

		if(NOT_NULL(postcmd)) {
			strlcpy(pstbuf, postcmd, BUFSIZ);
			strreplace(pstbuf, _SECT_TOK, ti->ti_section);
		}

		if(strcmp(thread_types[tt], _DFS_THR) == 0) {	/* filesystem free space */
			DPRINTSTR(stdout, "dirname   = %s\n", dirname);
			DPRINTSTR(stdout, "diskfree  = %s\n", diskfree);
			DPRINTSTR(stdout, "inofree   = %s\n", inofree);
			DPRINTSTR(stdout, "pcrestr   = %s\n", pcrestr);
			DPRINTSTR(stdout, "retmin    = %s\n", retmin);
			DPRINTSTR(stdout, "rmdir     = %s\n", rmdir);
			DPRINTSTR(stdout, "subdirs   = %s\n", subdirs);
			DPRINTSTR(stdout, "symlinks  = %s\n", symlinks);
			DPRINTSTR(stdout, "terse     = %s\n", terse);
			continue;
		}

		if(strcmp(thread_types[tt], _EXP_THR) == 0) {	/* file expiration, retention, dirlimit */
			DPRINTSTR(stdout, "dirname   = %s\n", dirname);
			DPRINTSTR(stdout, "dirlimit  = %s\n", dirlimit);
			DPRINTSTR(stdout, "expiresiz = %s\n", expiresiz);
			DPRINTSTR(stdout, "expire    = %s\n", expire);
			DPRINTSTR(stdout, "pcrestr   = %s\n", pcrestr);
			DPRINTSTR(stdout, "retmax    = %s\n", retmax);
			DPRINTSTR(stdout, "retmin    = %s\n", retmin);
			DPRINTSTR(stdout, "rmdir     = %s\n", rmdir);
			DPRINTSTR(stdout, "subdirs   = %s\n", subdirs);
			DPRINTSTR(stdout, "symlinks  = %s\n", symlinks);
			DPRINTSTR(stdout, "terse     = %s\n", terse);
			continue;
		}

		if(strcmp(thread_types[tt], _SLM_THR) == 0) {	/* simple log monitor */
			DPRINTSTR(stdout, "dirname   = %s\n", dirname);
			DPRINTSTR(stdout, "uid       = %s\n", uid);
			DPRINTSTR(stdout, "gid       = %s\n", gid);
			DPRINTSTR(stdout, "postcmd   = %s\n", pstbuf);
			DPRINTSTR(stdout, "rotatesiz = %s\n", rotatesiz);
			DPRINTSTR(stdout, "template  = %s\n", template);
			DPRINTSTR(stdout, "truncate  = %s\n", truncate);
			continue;
		}

		if(strcmp(thread_types[tt], _WRK_THR) == 0) {	/* worker (log ingestion) thread */
			DPRINTSTR(stdout, "dirname   = %s\n", dirname);
			DPRINTSTR(stdout, "command   = %s\n", command);
			DPRINTSTR(stdout, "uid       = %s\n", uid);
			DPRINTSTR(stdout, "gid       = %s\n", gid);
			DPRINTSTR(stdout, "pipename  = %s\n", pipename);
			DPRINTSTR(stdout, "postcmd   = %s\n", pstbuf);
			DPRINTSTR(stdout, "rotatesiz = %s\n", rotatesiz);
			DPRINTSTR(stdout, "template  = %s\n", template);
			continue;
		}
	}
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
