/*
 * Initially written by Alexey Tourbin <at@altlinux.org>.
 *
 * The author has dedicated the code to the public domain.  Anyone is free
 * to copy, modify, publish, use, compile, sell, or distribute the original
 * code, either in source code form or as a compiled binary, for any purpose,
 * commercial or non-commercial, and by any means.
 *
 * https://github.com/christian-proust/sqlite3-pcre2
 * gcc -shared -o pcre2.so -fPIC -W -Werror pcre2.c -lpcre2-8
 *
 * Sat Aug 26 07:12:34 PM PDT 2023
 * edits to fix clang warnings
 * clang-15 -shared -o pcre2.so -fPIC -W -Werror pcre2.c -lpcre2-8
 *
 * Sat Apr 18 07:24:21 PM PDT 2026
 * codex
 */

#define PCRE2_CODE_UNIT_WIDTH 8
#include <assert.h>
#include <pcre2.h>
#include <sqlite3ext.h>

SQLITE_EXTENSION_INIT1 typedef struct {
	pcre2_code *pattern_code;
	pcre2_match_data *match_data;
} regexp_cache;

static void free_regexp_cache(void *arg)
{
	regexp_cache *cache = arg;

	if(cache == NULL)
		return;

	if(cache->match_data)
		pcre2_match_data_free(cache->match_data);

	if(cache->pattern_code)
		pcre2_code_free(cache->pattern_code);

	sqlite3_free(cache);
}

static regexp_cache *compile_regexp(sqlite3_context *ctx, sqlite3_value *value)
{
	const unsigned char *pattern_str;
	int     error_code;
	int     pattern_len;
	PCRE2_SIZE error_position;
	regexp_cache *cache;

	cache = sqlite3_get_auxdata(ctx, 0);
	if(cache)
		return (cache);

	pattern_str = sqlite3_value_text(value);
	if(!pattern_str) {
		sqlite3_result_error(ctx, "no pattern", -1);
		return (NULL);
	}

	pattern_len = sqlite3_value_bytes(value);
	cache = sqlite3_malloc(sizeof(*cache));
	if(cache == NULL) {
		sqlite3_result_error_nomem(ctx);
		return (NULL);
	}

	cache->pattern_code = NULL;
	cache->match_data = NULL;
	cache->pattern_code = pcre2_compile(pattern_str, pattern_len, 0,
										&error_code, &error_position, NULL);
	if(cache->pattern_code == NULL) {
		PCRE2_UCHAR error_buffer[256];
		char   *message;

		pcre2_get_error_message(error_code, error_buffer, sizeof(error_buffer));
		message = sqlite3_mprintf("Cannot compile pattern \"%s\" at offset %d: %s",
								  pattern_str, (int)error_position, error_buffer);
		sqlite3_result_error(ctx, message, -1);
		sqlite3_free(message);
		sqlite3_free(cache);
		return (NULL);
	}

	cache->match_data = pcre2_match_data_create_from_pattern(cache->pattern_code, NULL);
	if(cache->match_data == NULL) {
		pcre2_code_free(cache->pattern_code);
		sqlite3_result_error_nomem(ctx);
		sqlite3_free(cache);
		return (NULL);
	}

	sqlite3_set_auxdata(ctx, 0, cache, free_regexp_cache);
	return (cache);
}

static void regexp(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
	regexp_cache *cache;
	const unsigned char *subject_str;
	int     rc;
	int     subject_len;

	assert(argc == 2);

	if(sqlite3_value_type(argv[0]) == SQLITE_NULL ||
	   sqlite3_value_type(argv[1]) == SQLITE_NULL)
		return;

	cache = compile_regexp(ctx, argv[0]);
	if(cache == NULL)
		return;

	subject_str = sqlite3_value_text(argv[1]);
	if(!subject_str) {
		sqlite3_result_error(ctx, "no subject", -1);
		return;
	}

	subject_len = sqlite3_value_bytes(argv[1]);
	rc = pcre2_match(cache->pattern_code, subject_str, subject_len, 0, 0,
					 cache->match_data, NULL);

	if(rc >= 0) {
		sqlite3_result_int(ctx, 1);
	} else if(rc == PCRE2_ERROR_NOMATCH) {
		sqlite3_result_int(ctx, 0);
	} else {
		PCRE2_UCHAR error_buffer[256];
		char   *message;

		pcre2_get_error_message(rc, error_buffer, sizeof(error_buffer));
		message = sqlite3_mprintf("%s", error_buffer);
		sqlite3_result_error(ctx, message, -1);
		sqlite3_free(message);
	}
}

int sqlite3_extension_init(sqlite3 *db, char **err, const sqlite3_api_routines *api)
{
	SQLITE_EXTENSION_INIT2(api)
		(void)err;

	return sqlite3_create_function(db, "REGEXP", 2,
								   SQLITE_UTF8 | SQLITE_DETERMINISTIC,
								   NULL, regexp, NULL, NULL);
}

/* vim: set tabstop=4 shiftwidth=4 expandtab: */
