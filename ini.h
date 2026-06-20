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

# include <stdio.h>									/* FILE */

# define	INI_VERSION	"0.1.1"

struct ini_t {
	char   *data;
	char   *end;
};

typedef struct ini_t ini_t;

ini_t  *ini_load(const char *filename);
ini_t  *ini_load_fp(FILE * fp);						/* takes ownership; fcloses fp */
void    ini_free(ini_t *ini);
char   *ini_get(const ini_t *ini, const char *section, const char *key);

#endif

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
