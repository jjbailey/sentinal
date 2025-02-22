/*
 * Initially written by Alexey Tourbin <at@altlinux.org>.
 *
 * The author has dedicated the code to the public domain.  Anyone is free
 * to copy, modify, publish, use, compile, sell, or distribute the original
 * code, either in source code form or as a compiled binary, for any purpose,
 * commercial or non-commercial, and by any means.
 *
 * https://github.com/christian-proust/sqlite3-pcre2
 *
 * gcc -shared -o pcre2.so -fPIC -W -Werror pcre2.c -lpcre2-8
 *
 * Sat Aug 26 07:12:34 PM PDT 2023
 * edits to fix clang warnings
 *
 * clang-15 -shared -o pcre2.so -fPIC -W -Werror pcre2.c -lpcre2-8
 */

#define PCRE2_CODE_UNIT_WIDTH 8
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pcre2.h>
#include <sqlite3ext.h>

SQLITE_EXTENSION_INIT1 typedef struct {
	char   *pattern_str;
	int     pattern_len;
	pcre2_code *pattern_code;
} cache_entry;

#ifndef CACHE_SIZE
# define CACHE_SIZE 16
#endif

static void regexp(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
	const unsigned char *pattern_str, *subject_str;
	int     pattern_len, subject_len;
	pcre2_code *pattern_code;

	assert(argc == 2);
	/* check null */
	if(sqlite3_value_type(argv[0]) == SQLITE_NULL ||
	   sqlite3_value_type(argv[1]) == SQLITE_NULL) {
		return;
	}

	pattern_str = sqlite3_value_text(argv[0]);
	if(!pattern_str) {
		sqlite3_result_error(ctx, "no pattern", -1);
		return;
	}
	pattern_len = sqlite3_value_bytes(argv[0]);

	subject_str = sqlite3_value_text(argv[1]);
	if(!subject_str) {
		sqlite3_result_error(ctx, "no subject", -1);
		return;
	}
	subject_len = sqlite3_value_bytes(argv[1]);

	/* simple LRU cache */
	{
		int     i;
		int     found = 0;
		cache_entry *cache = sqlite3_user_data(ctx);

		assert(cache);

		for(i = 0; i < CACHE_SIZE && cache[i].pattern_str; i++)
			if(pattern_len == cache[i].pattern_len
			   && memcmp(pattern_str, cache[i].pattern_str, pattern_len) == 0) {
				found = 1;
				break;
			}
		if(found) {
			if(i > 0) {
				cache_entry c = cache[i];
				memmove(cache + 1, cache, i * sizeof(cache_entry));
				cache[0] = c;
			}
		} else {
			cache_entry c;
			int     error_code;
			PCRE2_SIZE error_position;
			c.pattern_code = pcre2_compile(pattern_str,	/* the pattern */
										   pattern_len,	/* the length of the pattern */
										   0,		/* default options */
										   &error_code,	/* for error number */
										   &error_position,	/* for error offset */
										   NULL);	/* use default compile context */
			if(!c.pattern_code) {
				PCRE2_UCHAR error_buffer[256];
				pcre2_get_error_message(error_code, error_buffer, sizeof(error_buffer));
				char   *e2 =
					sqlite3_mprintf("Cannot compile pattern \"%s\" at offset %d: %s",
									pattern_str, (int)error_position, error_buffer);
				sqlite3_result_error(ctx, e2, -1);
				sqlite3_free(e2);
				return;
			}
			c.pattern_str = malloc(pattern_len);
			if(!c.pattern_str) {
				sqlite3_result_error(ctx, "malloc: ENOMEM", -1);
				pcre2_code_free(c.pattern_code);
				return;
			}
			memcpy(c.pattern_str, pattern_str, pattern_len);
			c.pattern_len = pattern_len;
			i = CACHE_SIZE - 1;
			if(cache[i].pattern_str) {
				free(cache[i].pattern_str);
				assert(cache[i].pattern_code);
				pcre2_code_free(cache[i].pattern_code);
			}
			memmove(cache + 1, cache, i * sizeof(cache_entry));
			cache[0] = c;
		}
		pattern_code = cache[0].pattern_code;
	}

	{
		int     rc;
		pcre2_match_data *match_data;
		assert(pattern_code);

		match_data = pcre2_match_data_create_from_pattern(pattern_code, NULL);
		rc = pcre2_match(pattern_code,				/* the compiled pattern */
						 subject_str,				/* the subject string */
						 subject_len,				/* the length of the subject */
						 0,							/* start at offset 0 in the subject */
						 0,							/* default options */
						 match_data,				/* block for storing the result */
						 NULL);						/* use default match context */

		assert(rc != 0);							// because we have not set match_data
		if(rc >= 0) {
			// Normal case because we have not set match_data
			sqlite3_result_int(ctx, 1);
		} else if(rc == PCRE2_ERROR_NOMATCH) {
			sqlite3_result_int(ctx, 0);
		} else {									// (rc < 0 and the code is not one of the above)
			PCRE2_UCHAR error_buffer[256];
			pcre2_get_error_message(rc, error_buffer, sizeof(error_buffer));
			char   *e2 = sqlite3_mprintf("%s", error_buffer);
			sqlite3_result_error(ctx, e2, -1);
			sqlite3_free(e2);
			return;
		}
		pcre2_match_data_free(match_data);
		return;
	}
}

int sqlite3_extension_init(sqlite3 *db, char **err, const sqlite3_api_routines *api)
{
	SQLITE_EXTENSION_INIT2(api)
	cache_entry *cache = calloc(CACHE_SIZE, sizeof(cache_entry));
	if(!cache) {
		*err = "calloc: ENOMEM";
		return 1;
	}
	sqlite3_create_function(db, "REGEXP", 2, SQLITE_UTF8, cache, regexp, NULL, NULL);
	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 expandtab: */
