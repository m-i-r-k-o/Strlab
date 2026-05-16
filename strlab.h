/**
 * @file strlab_string.c
 * @brief Libreria per stringhe SSO
 * @author Vitolo Mirko
 * @date 2026-04-30
 */

#ifndef STRLAB_H
#define STRLAB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef STRLAB_MALLOC
#define STRLAB_MALLOC(siz) malloc(siz)
#endif

#ifndef STRLAB_FREE
#define STRLAB_FREE(ptr) free(ptr)
#endif

#ifndef STRLAB_SSO_SIZE
#define STRLAB_SSO_SIZE 16
#endif

#ifndef STRLAB_NPOS
#define STRLAB_NPOS ((size_t)(-1))
#endif

#ifndef STRLAB_LFIN
#define STRLAB_LFIN (const strlab_string){{1, 2, NULL, {[0] = (char)0xFF, [1] = '\0'}, 0}}
#endif

#if STRLAB_SSO_SIZE < 2
#error strlab: SSO size too small
#endif

#define strlab_expr(expr) (const strlab_string){{sizeof((expr)) - 1, sizeof((expr)), (expr), {0}, 0}}

#define strlab_const(expr) {{sizeof((expr)) - 1, sizeof((expr)), (expr), {0}, 0}}

#define strlab_create() {{0, STRLAB_SSO_SIZE, NULL, {0}, 1}}

#define strlab_fixed(buf,siz) {{(buf)[0] = '\0', (siz), (buf), {0}, 0}}

enum {
    STRLAB_SUCCESS = 0,
    STRLAB_MAX_SIZE_REACHED,
    STRLAB_OUT_OF_MEMORY,
    STRLAB_OUT_OF_RANGE,
    STRLAB_INVALID_CHAR,
    STRLAB_FREE_ON_FIXED,
    STRLAB_PRINTF_ERROR,
    STRLAB_SCANF_ERROR,
    STRLAB_IO_ERROR,
};

typedef struct strlab_string_struct {
    size_t len;
    size_t size;
    char *dat;
    char little[STRLAB_SSO_SIZE];
    uint8_t dynamic;
} strlab_string[1];

void strlab_init(strlab_string string);

int strlab_free(strlab_string string);

void strlab_attach(strlab_string string, char *buf, size_t size);

void strlab_log(const strlab_string string);

void strlab_clear(strlab_string string);

int strlab_isempty(const strlab_string string);

size_t strlab_length(const strlab_string string);

const char *strlab_cstring(const strlab_string string);

int strlab_greater(const strlab_string string1, const strlab_string string2);

int strlab_equals(const strlab_string string1, const strlab_string string2);

int strlab_less(const strlab_string string1, const strlab_string string2);

int strlab_compare(const strlab_string string1, const strlab_string string2);

int strlab_copy(strlab_string string, const strlab_string other);

int strlab_insert(strlab_string string, size_t index, const strlab_string other);

int strlab_append(strlab_string string, const strlab_string other);

int strlab_erase(strlab_string string, size_t index, size_t length);

int strlab_putchar(strlab_string string, int chr);

int strlab_substr(strlab_string string, size_t index, size_t length);

int strlab_repalce(strlab_string string, const strlab_string old, const strlab_string new);

int strlab_rchange(strlab_string string, const strlab_string old, const strlab_string new);

int strlab_lchange(strlab_string string, const strlab_string old, const strlab_string new);

int strlab_ensure(strlab_string string, size_t size);

size_t strlab_search(const strlab_string string, const strlab_string substr);

const char *strlab_lfind(const strlab_string string, const strlab_string substr);

const char *strlab_rfind(const strlab_string string, const strlab_string substr);

int strlab_index(const strlab_string string, size_t index);

char *strlab_offset(strlab_string string, size_t index);

int strlab_strip(strlab_string string, const strlab_string set);

void strlab_capitalize(strlab_string string);

void strlab_lowercase(strlab_string string);

int strlab_foreach(strlab_string string, int (*callback)(char*, size_t, void*), void *args);

int strlab_printf(strlab_string string, const char *format, ...);

int strlab_scanf(const strlab_string string, const char *format, ...);

char* strlab_relase(strlab_string string);

size_t strlab_count(const strlab_string str, const strlab_string sub);

int strlab_startswith(const strlab_string str, const strlab_string sub);

int strlab_endswith(const strlab_string str, const strlab_string sub);

int strlab_fgetln(strlab_string str, FILE *src);

int strlab_fread(strlab_string str, FILE *src, size_t size);

int strlab_fwrite(FILE *dest, const strlab_string str);

strlab_string *strlab_split(const strlab_string str, const strlab_string del);

int strlab_join(strlab_string str, const strlab_string *const list);

int strlab_unlist(strlab_string *list);

int strlab_expandtabs(strlab_string str, size_t n);

#endif
