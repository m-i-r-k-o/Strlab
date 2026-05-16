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

#define strlab_expr(expr) (const strlab_string){{sizeof((expr)) - 1, sizeof((expr)), (expr), {0}, 0}}

#define strlab_const(expr) {{sizeof((expr)) - 1, sizeof((expr)), (expr), {0}, 0}}

enum {
    STRLAB_SUCCESS = 0,
    STRLAB_MAX_SIZE_REACHED,
    STRLAB_OUT_OF_MEMORY,
    STRLAB_OUT_OF_RANGE,
    STRLAB_INVALID_CHAR,
    STRLAB_FREE_ON_FIXED,
    STRLAB_IO_ERROR,
    STRLAB_NOT_FOUND
};

typedef struct strlab_string_struct {
    size_t len;
    size_t size;
    char *buf;
    char little[STRLAB_SSO_SIZE];
    uint8_t dynamic;
} strlab_string[1], strlab_struct;

extern const strlab_string STRLAB_LFIN;

void strlab_create(strlab_struct *str);

int strlab_free(strlab_struct *str);

void strlab_fixed(strlab_struct *str, char *buf, size_t size);

void strlab_log(const strlab_struct *str);

void strlab_clear(strlab_struct *str);

int strlab_isempty(const strlab_struct *str);

size_t strlab_length(const strlab_struct *str);

const char *strlab_cstring(const strlab_struct *str);

int strlab_greater(const strlab_struct *str1, const strlab_struct *str2);

int strlab_equals(const strlab_struct *str1, const strlab_struct *str2);

int strlab_less(const strlab_struct *str1, const strlab_struct *str2);

int strlab_compare(const strlab_struct *str1, const strlab_struct *str2);

int strlab_copy(strlab_struct *str, const strlab_struct *other);

int strlab_insert(strlab_struct *str, size_t index, const strlab_struct *other);

int strlab_append(strlab_struct *str, const strlab_struct *other);

int strlab_erase(strlab_struct *str, size_t index, size_t length);

int strlab_putchar(strlab_struct *str, int chr);

int strlab_substr(strlab_struct *str, size_t index, size_t length);

int strlab_repalce(strlab_struct *str, const strlab_struct *old, const strlab_struct *new);

int strlab_rchange(strlab_struct *str, const strlab_struct *old, const strlab_struct *new);

int strlab_lchange(strlab_struct *str, const strlab_struct *old, const strlab_struct *new);

int strlab_ensure(strlab_struct *str, size_t size);

size_t strlab_search(const strlab_struct *str, const strlab_struct *sub);

const char *strlab_lfind(const strlab_struct *str, const strlab_struct *sub);

const char *strlab_rfind(const strlab_struct *str, const strlab_struct *sub);

int strlab_charat(const strlab_struct *str, size_t index);

char *strlab_index(strlab_struct *str, size_t index);

int strlab_strip(strlab_struct *str, const strlab_struct *set);

void strlab_capitalize(strlab_struct *str);

void strlab_lowercase(strlab_struct *str);

int strlab_foreach(strlab_struct *str, int (*callback)(char*, size_t, void*), void *args);

int strlab_printf(strlab_struct *str, const char *format, ...);

int strlab_scanf(const strlab_struct *str, const char *format, ...);

char *strlab_relase(strlab_struct *str);

size_t strlab_count(const strlab_struct *str, const strlab_struct *sub);

int strlab_startswith(const strlab_struct *str, const strlab_struct *sub);

int strlab_endswith(const strlab_struct *str, const strlab_struct *sub);

int strlab_fgetln(strlab_struct *str, FILE *src);

int strlab_fread(strlab_struct *str, FILE *src, size_t size);

int strlab_fwrite(FILE *dest, const strlab_struct *str);

strlab_string *strlab_split(const strlab_struct *str, const strlab_struct *del);

int strlab_join(strlab_struct *str, const strlab_string *list);

int strlab_unlist(strlab_string *list);

#endif