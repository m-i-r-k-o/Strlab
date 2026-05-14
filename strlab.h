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

#ifndef STRLAB_MALLOC
#define STRLAB_MALLOC(siz) malloc(siz)
#endif

#ifndef STRLAB_FREE
#define STRLAB_FREE(ptr) free(ptr)
#endif

#ifndef STRLAB_ERROR
#define STRLAB_ERROR(funcname) perror(#funcname)
#endif

#ifndef STRLAB_SIZE
#define STRLAB_SIZE 16
#endif

#define strlab_const(str) {{sizeof(str) - 1, sizeof(str), str, {0}}}

typedef struct {
    size_t len; /**< Lunghezza della stringa */
    size_t size; /**< Gandezza del buffer utilizzato */
    char *dat; /**< Puntatore alla memoria in caso di stringa grande */
    char little[STRLAB_SIZE]; /**< Buffer piccolo per permettere l'SSO */
} strlab_string[1];

void strlab_create(strlab_string string);

void strlab_delete(strlab_string string);

void strlab_clear(strlab_string string);

int strlab_empty(const strlab_string string);

size_t strlab_length(const strlab_string string);

const char *strlab_cstring(const strlab_string string);

void strlab_copy(strlab_string string, const char *dat);

void strlab_append(strlab_string string, const char *dat);

void strlab_putchar(strlab_string string, char chr);

void strlab_substr(strlab_string string, size_t index, size_t length);

void strlab_freplace(strlab_string string, const char *old, const char *new);

void strlab_lreplace(strlab_string string, const char *old, const char *new);

size_t strlab_replace(strlab_string string, const char *old, const char *new);

void strlab_resize(strlab_string string, size_t size);

int strlab_charat(const strlab_string string, size_t index);

void strlab_setchar(strlab_string string, size_t index, int c);

void strlab_trim(strlab_string string);

void strlab_capitalize(strlab_string string);

void strlab_lower(strlab_string string);

void strlab_map(strlab_string string, int (*callback)(int, size_t, void*), void *args);

void strlab_printf(strlab_string string, const char *format, ...);

int strlab_scanf(const strlab_string string, const char *format, ...);

char *strlab_drop(strlab_string string);

size_t strlab_split(strlab_string **list, const strlab_string string, const char *delim);

size_t strlab_split_into(strlab_string *list, size_t size, const strlab_string string, const char *delim, int definded);

void strlab_join(strlab_string string, const strlab_string *list, size_t nel, const char *sep);

void strlab_work(strlab_string string, void (*wdat)(char*, size_t*), size_t ensure);

void strlab_fgetln(strlab_string string, FILE *file);

void strlab_fread(strlab_string string, FILE *file);

void strlab_fwrite(FILE *file, const strlab_string string);

size_t strlab_tokenize(strlab_string **list, const strlab_string string, const char *delim);

size_t strlab_tokenize_into(strlab_string *list, size_t size, const strlab_string string, const char *delim, int defined);

size_t strlab_count(const strlab_string string, const char *substr);

int strlab_startswith(const strlab_string string, const char *substr);

int strlab_endswith(const strlab_string string, const char *substr);

#endif
