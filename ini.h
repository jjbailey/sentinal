/**
 * Copyright (c) 2016 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ini.c` for details.
 *
 * Wed Sep 20 02:39:25 PM PDT 2023
 * remove unused ini_sget()
 */

#ifndef INI_H
# define	INI_H

# define	INI_VERSION	"0.1.1"

struct ini_t {
	char   *data;
	char   *end;
};

typedef struct ini_t ini_t;

ini_t  *ini_load(char *filename);
void    ini_free(ini_t *ini);
char   *ini_get(ini_t *ini, char *section, char *key);
# if 0												/* jjb */
int     ini_sget(ini_t *ini, char *section, char *key, char *scanfmt, void *dst);
# endif

#endif

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
