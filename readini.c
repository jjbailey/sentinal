/*
 * readini.c
 * Read INI file and process the keys/values.
 *
 * Copyright (c) 2021-2024 jjb
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found
 * in the root directory of this source tree.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "sentinal.h"
#include "basename.h"
#include "ini.h"

static int parsecmd(char *, char **);
static short setiniflag(ini_t *, char *, char *);

extern char database[PATH_MAX];						/* database file name */
extern char *pidfile;								/* sentinal pid */
extern char *sections[MAXSECT];						/* section names */
extern ini_t *inidata;								/* loaded ini data */
extern struct thread_info tinfo[MAXSECT];			/* our threads */
extern struct utsname utsbuf;						/* for host info */

char   *my_ini(ini_t *, char *, char *);

int readini(char *myname, char *inifile)
{
	char   *p;
	char    inipath[PATH_MAX];						/* real path to file */
	char    rpbuf[PATH_MAX];						/* real path buffer */
	char    tbuf[PATH_MAX];							/* temp buffer */
	DIR    *dirp;
	int     i;
	int     inifd;									/* for lstat() */
	int     nsect;									/* number of sections found */
	struct stat stbuf;								/* file status */
	struct thread_info *ti;							/* thread settings */
	int     get_sections(ini_t *, int, char **);

	if(lstat(inifile, &stbuf) == 0)
		if(S_ISLNK(stbuf.st_mode)) {
			fprintf(stderr, "%s: %s is a symlink\n", myname, inifile);
			return (0);
		}

	realpath(inifile, inipath);

	if((inifd = open(inipath, O_RDONLY)) > 0)
		if(lstat(inipath, &stbuf) == 0) {
			if(S_ISLNK(stbuf.st_mode)) {
				fprintf(stderr, "%s: %s is a symlink\n", myname, inipath);
				return (0);
			}

			if(stbuf.st_mode & S_IWGRP || stbuf.st_mode & S_IWOTH) {
				fprintf(stderr, "%s: %s is writable by group or other\n", myname,
						inipath);

				return (0);
			}

			/* tighter */
			fchmod(inifd, stbuf.st_mode & ~(S_IWGRP | S_IXGRP | S_IWOTH | S_IXOTH));
			close(inifd);
		}

	/* configure the threads */

	if((inidata = ini_load(inipath)) == NULL) {
		fprintf(stderr, "%s: can't load %s\n", myname, inipath);
		return (0);
	}

	if((nsect = get_sections(inidata, MAXSECT, sections)) == 0) {
		fprintf(stderr, "%s: nothing to do\n", myname);
		return (0);
	}

	uname(&utsbuf);									/* for debug/token expansion */

	/* INI global settings */

	pidfile = strndup(my_ini(inidata, "global", "pidfile"), PATH_MAX);

	if(IS_NULL(pidfile) || *pidfile != '/') {
		fprintf(stderr, "%s: pidfile is null or path not absolute\n", myname);
		return (0);
	}

	p = my_ini(inidata, "global", "database");		/* optional */

	if(IS_NULL(p) || strcmp(p, SQLMEMDB) == 0)
		strlcpy(database, SQLMEMDB, PATH_MAX);
	else
		strlcpy(database, p, PATH_MAX);				/* verbatim */

	/* INI thread settings */

	for(i = 0; i < nsect; i++) {
		ti = &tinfo[i];								/* shorthand */
		memset(ti, '\0', sizeof(struct thread_info));

		ti->ti_section = sections[i];

		ti->ti_command = strndup(my_ini(inidata, ti->ti_section, "command"), PATH_MAX);
		ti->ti_argc = parsecmd(ti->ti_command, ti->ti_argv);

		if(NOT_NULL(ti->ti_command) && ti->ti_argc) {
			ti->ti_path = ti->ti_argv[0];

			/* get real path of argv[0] */

			if(IS_NULL(ti->ti_path) || *ti->ti_path != '/') {
				fprintf(stderr, "%s: command path is null or not absolute\n",
						ti->ti_section);
				return (0);
			}

			if(realpath(ti->ti_path, rpbuf) == NULL) {
				fprintf(stderr, "%s: missing or bad command path\n", ti->ti_section);
				return (0);
			}
		}

		ti->ti_path = strndup(ti->ti_argc ? rpbuf : "", PATH_MAX);

		/* get real path of directory */

		ti->ti_dirname = my_ini(inidata, ti->ti_section, "dirname");

		if(IS_NULL(ti->ti_dirname) || *ti->ti_dirname != '/') {
			fprintf(stderr, "%s: dirname is null or not absolute\n", ti->ti_section);
			return (0);
		}

		if(realpath(ti->ti_dirname, rpbuf) == NULL)
			*rpbuf = '\0';

		ti->ti_dirname = strndup(rpbuf, PATH_MAX);
		ti->ti_mountdir = malloc(PATH_MAX);			/* for findmnt() */

		if(IS_NULL(ti->ti_dirname) || strcmp(ti->ti_dirname, "/") == 0) {
			fprintf(stderr, "%s: missing or bad dirname\n", ti->ti_section);
			return (0);
		}

		/* is ti_dirname a directory */

		if((dirp = opendir(ti->ti_dirname)) == NULL) {
			fprintf(stderr, "%s: dirname is not a directory\n", ti->ti_section);
			return (0);
		}

		closedir(dirp);

		/* ti_subdirs defaults to true */

		p = my_ini(inidata, ti->ti_section, "subdirs");

		if(IS_NULL(p))
			ti->ti_subdirs = TRUE;
		else
			ti->ti_subdirs = setiniflag(inidata, ti->ti_section, "subdirs");

		/* notify file removal */
		ti->ti_terse = setiniflag(inidata, ti->ti_section, "terse");

		/* remove empty dirs */
		ti->ti_rmdir = setiniflag(inidata, ti->ti_section, "rmdir");

		/* follow symlinks */
		ti->ti_symlinks = setiniflag(inidata, ti->ti_section, "symlinks");

		/* truncate slm-managed files */
		ti->ti_truncate = setiniflag(inidata, ti->ti_section, "truncate");

		/*
		 * get/generate/vett real path of pipe
		 * pipe might not exist, or it might not be in dirname
		 */

		ti->ti_pipename = my_ini(inidata, ti->ti_section, "pipename");

		if(NOT_NULL(ti->ti_pipename)) {
			if(strstr(ti->ti_pipename, "..")) {
				fprintf(stderr, "%s: bad pipename\n", ti->ti_section);
				return (0);
			}

			fullpath(ti->ti_dirname, ti->ti_pipename, tbuf);

			if(realpath(dirname(tbuf), rpbuf) == NULL) {
				fprintf(stderr, "%s: missing or bad pipedir\n", ti->ti_section);
				return (0);
			}

			fullpath(rpbuf, base(ti->ti_pipename), tbuf);
			ti->ti_pipename = strndup(tbuf, PATH_MAX);
		}

		ti->ti_template = malloc(BUFSIZ);			/* more than PATH_MAX */
		memset(ti->ti_template, '\0', BUFSIZ);
		strlcpy(ti->ti_template,
				base(my_ini(inidata, ti->ti_section, "template")), PATH_MAX);

		ti->ti_pcrestr = my_ini(inidata, ti->ti_section, "pcrestr");
		pcrecompile(ti);

		ti->ti_filename = malloc(BUFSIZ);			/* more than PATH_MAX */
		memset(ti->ti_filename, '\0', BUFSIZ);

		ti->ti_pid = (pid_t) 0;						/* only workers use this */
		ti->ti_wfd = 0;								/* only workers use this */

		ti->ti_uid = verifyuid(my_ini(inidata, ti->ti_section, "uid"));
		ti->ti_gid = verifygid(my_ini(inidata, ti->ti_section, "gid"));

		ti->ti_dirlimstr = my_ini(inidata, ti->ti_section, "dirlimit");
		ti->ti_dirlimit = logsize(ti->ti_dirlimstr);

		ti->ti_rotatestr = my_ini(inidata, ti->ti_section, "rotatesiz");
		ti->ti_rotatesiz = logsize(ti->ti_rotatestr);

		ti->ti_expirestr = my_ini(inidata, ti->ti_section, "expiresiz");
		ti->ti_expiresiz = logsize(ti->ti_expirestr);

		ti->ti_diskfree = fabs(atof(my_ini(inidata, ti->ti_section, "diskfree")));
		ti->ti_inofree = fabs(atof(my_ini(inidata, ti->ti_section, "inofree")));
		ti->ti_expire = logretention(my_ini(inidata, ti->ti_section, "expire"));

		ti->ti_retminstr = my_ini(inidata, ti->ti_section, "retmin");
		ti->ti_retmin = logsize(ti->ti_retminstr);

		ti->ti_retmaxstr = my_ini(inidata, ti->ti_section, "retmax");
		ti->ti_retmax = logsize(ti->ti_retmaxstr);

		ti->ti_postcmd = malloc(BUFSIZ);
		memset(ti->ti_postcmd, '\0', BUFSIZ);
		strlcpy(ti->ti_postcmd, my_ini(inidata, ti->ti_section, "postcmd"), BUFSIZ);
	}

	return (nsect);
}

static short setiniflag(ini_t *ini, char *section, char *key)
{
	char   *inip;

	if(IS_NULL(inip = my_ini(ini, section, key)))
		return (FALSE);

	return (strcmp(inip, "1") == 0 || strcasecmp(inip, "true") == 0);
}

static int parsecmd(char *cmd, char *argv[])
{
	char    str[BUFSIZ];
	char   *ap, argv0[PATH_MAX];
	char   *p;
	int     i = 0;

	if(IS_NULL(cmd))
		return (0);

	strlcpy(str, cmd, BUFSIZ);
	strreplace(str, "\t", " ");

	if((p = strtok(str, " ")) == NULL)
		return (0);

	while(p) {
		if(i == 0) {
			ap = realpath(p, argv0);
			argv[i++] = strdup(IS_NULL(ap) ? p : ap);
		} else if(i < (MAXARGS - 1))
			argv[i++] = strdup(p);

		p = strtok(NULL, " ");
	}

	argv[i] = NULL;
	return (i);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
