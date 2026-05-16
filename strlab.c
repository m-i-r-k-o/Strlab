#include "strlab.h"

#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

#define strlab_max(a,b) (((a) > (b)) ? (a) : (b))

static inline size_t strlab_round_size(size_t size) {
    if(size == 0) return 1;
    size--;

    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;

    return size + 1;
}

static inline void strlab_init_struct(strlab_struct *str) {
    str->len = 0;
    str->size = STRLAB_SSO_SIZE;
    str->buf = str->little;
    str->dynamic = 1;
    str->buf[0] = '\0';
}

static inline int strlab_free_struct(strlab_struct *str) {
    if(!str->dynamic) return STRLAB_FREE_ON_FIXED;
    if(str->size > STRLAB_SSO_SIZE) STRLAB_FREE(str->buf);
    return STRLAB_SUCCESS;
}

static inline void strlab_clear_struct(strlab_struct *str) {
    str->buf[0] = '\0';
    str->len = 0;
}

static inline int strlab_swap_and_destroy(strlab_struct *destroy, strlab_struct *dest) {
    if(!dest->dynamic) {
        if(destroy->len > dest->len) return STRLAB_MAX_SIZE_REACHED;
        memcpy(dest->buf, destroy->buf, destroy->len);
        strlab_free_struct(destroy);
        return STRLAB_SUCCESS;
    }

    strlab_free_struct(dest);
    dest->len = destroy->len;
    dest->size = destroy->size;

    if(destroy->size > STRLAB_SSO_SIZE) {
        dest->buf = destroy->buf;
    } else {
        dest->buf = dest->little;
        memcpy(dest->buf, destroy->buf, destroy->len);
    }

    dest->dynamic = destroy->dynamic;
    return STRLAB_SUCCESS;
}

static int strlab_ensure_size(strlab_struct *str, size_t request) {
    if(str->size >= request) return STRLAB_SUCCESS;
    if(!str->dynamic) return STRLAB_MAX_SIZE_REACHED;

    size_t newsiz = strlab_round_size(request);

    char *newbuf = STRLAB_MALLOC(newsiz);
    if(!newbuf) return STRLAB_OUT_OF_MEMORY;

    memcpy(newbuf, str->buf, str->len);
    if(str->size > STRLAB_SSO_SIZE) STRLAB_FREE(str->buf);

    str->buf = newbuf;
    str->size = newsiz;

    return STRLAB_SUCCESS;
}

static int strlab_copy_offset(strlab_struct *str, size_t offset, const char *src, size_t srclen) {
    if(offset > str->len) return STRLAB_OUT_OF_RANGE;
    if(srclen == 0) return STRLAB_SUCCESS;

    size_t newlen = strlab_max(str->len, offset + srclen);
    size_t maxsiz = newlen + 1;
    
    int error = strlab_ensure_size(str, maxsiz);
    if(error) return error;

    memcpy(str->buf + offset, src, srclen);
    str->len = newlen;
    str->buf[newlen] = '\0';

    return STRLAB_SUCCESS;
}

static int strlab_fill_offset(strlab_struct *str, size_t offset, int fill, size_t space) {
    if(offset > str->len) return STRLAB_OUT_OF_RANGE;
    if(space == 0) return STRLAB_SUCCESS;

    size_t newlen = strlab_max(str->len, offset + space);
    size_t maxsiz = newlen + 1;

    int error = strlab_ensure_size(str, maxsiz);
    if(error) return error;

    memset(str->buf + offset, fill, space);
    str->len = newlen;
    str->buf[newlen] = '\0';

    return STRLAB_SUCCESS;
}

static int strlab_remove_offset(strlab_struct *str, size_t offset, size_t torm) {
    if(offset > str->len) return STRLAB_OUT_OF_RANGE;
    if(torm == 0) return STRLAB_SUCCESS;

    if(offset + torm > str->len) torm = str->len - offset;
    size_t newlen = str->len - torm;

    memmove(str->buf + offset, str->buf + offset + torm, newlen - offset + 1);
    str->len = newlen;

    return STRLAB_SUCCESS;
}

static int strlab_shift_offset(strlab_struct *str, size_t offset, size_t oldlen, size_t newlen) {
    if(offset > str->len) return STRLAB_OUT_OF_RANGE;

    size_t len = str->len - oldlen + newlen;
    if(newlen > oldlen) {
        int error = strlab_ensure_size(str, len + 1);
        if(error) return error;
    }

    char *ptr = str->buf + offset;
    size_t tomove = str->len - offset - oldlen + 1;

    memmove(ptr + newlen, ptr + oldlen, tomove);
    str->len = len;

    return STRLAB_SUCCESS;
}

static size_t strlab_search_offset(
    const strlab_struct *str, size_t offset, const char *target, size_t tarlen
) {
    if(offset > str->len) return STRLAB_NPOS;
    if(tarlen > str->len) return STRLAB_NPOS;
    if(tarlen == 0) return STRLAB_NPOS;
    if(tarlen == str->len) return 0;

    size_t area = str->len - offset;
    const char *found = memmem(str->buf + offset, area, target, tarlen);
    
    if(!found) return STRLAB_NPOS;
    return (size_t)(found - str->buf);
}

static size_t strlab_search_backwards(const strlab_struct *str, const char *target, size_t tarlen) {
    if(tarlen > str->len) return STRLAB_NPOS;
    if(tarlen == 0) return STRLAB_NPOS;
    if(tarlen == str->len) return 0;

    const char *ptr = str->buf + str->len - tarlen;

    while(ptr > str->buf) {
        int found = memcmp(ptr, target, tarlen) == 0;
        if(found) return (size_t)(ptr - str->buf);
        ptr--;
    }

    return STRLAB_NPOS;
}

static int strlab_list_append(
    strlab_string **list, size_t *size, size_t *len, const char *src, size_t srclen
) {
    if(*len >= *size) {
        size_t newsiz = strlab_round_size(*size + 1);

        strlab_string *newdat = STRLAB_MALLOC(newsiz * sizeof(strlab_string));
        if(!newdat) return STRLAB_OUT_OF_MEMORY;

        if(*list) {
            memcpy(newdat, *list, *len * sizeof(strlab_string));
            STRLAB_FREE(*list);
            for(size_t n = 0; n < *len; n++) {
                newdat[n]->buf = newdat[n]->little;
            }
        }

        *list = newdat;
        *size = newsiz;
    }

    strlab_struct *elem = (*list)[*len];
    strlab_init_struct(elem);

    int error = strlab_copy_offset(elem, 0, src, srclen);
    if(error) {
        strlab_free_struct(elem);
        return error;
    }

    *len += 1;

    return STRLAB_SUCCESS;
}

void strlab_create(strlab_struct *str) {
    strlab_init_struct(str);
}

int strlab_free(strlab_struct *str) {
    return strlab_free_struct(str);
}

void strlab_fixed(strlab_struct *str, char *buf, size_t size) {
    str->len = strlen(buf);
    str->size = size;
    str->buf = buf;
    str->little[0] = '\0';
    str->dynamic = 0;
}

void strlab_log(const strlab_struct *string) {
    printf(
        "'%s' len=%zu size=%zu SSO=%s dynamic=%s\n",
        string->buf, string->len, string->size,
        (string->size <= STRLAB_SSO_SIZE) ? "True" : "False",
        string->dynamic ? "True" : "Flase"
    );
}

void strlab_clear(strlab_struct *str) {
    strlab_clear_struct(str);
}

int strlab_isempty(const strlab_struct *str) {
    return str->buf[0] == '\0';
}

size_t strlab_length(const strlab_struct *str) {
    return str->len;
}

const char *strlab_cstring(const strlab_struct *str) {
    return str->buf;
}

int strlab_greater(const strlab_struct *str1, const strlab_struct *str2) {
    return strcmp(str1->buf, str2->buf) > 0;
}

int strlab_equals(const strlab_struct *str1, const strlab_struct *str2) {
    return str1->len == str2->len && memcmp(str1->buf, str2->buf, str1->len) == 0;
}

int strlab_less(const strlab_struct *str1, const strlab_struct *str2) {
    return strcmp(str1->buf, str2->buf) < 0;
}

int strlab_compare(const strlab_struct *str1, const strlab_struct *str2) {
    return strcmp(str1->buf, str2->buf);
}

int strlab_copy(strlab_struct *str, const strlab_struct *other) {
    return strlab_copy_offset(str, 0, other->buf, other->len);
}

int strlab_insert(strlab_struct *str, size_t index, const strlab_struct *other) {
    strlab_string last;
    strlab_init_struct(last);
    
    int error = strlab_copy_offset(last, 0, str->buf + index, str->len - index);
    if(!error) error = strlab_copy_offset(str, index, other->buf, other->len);
    if(!error) error = strlab_copy_offset(str, index + other->len, last->buf, last->len);

    strlab_free_struct(last);
    return error; 
}

int strlab_append(strlab_struct *str, const strlab_struct *other) {
    return strlab_copy_offset(str, str->len, other->buf, other->len);
}

int strlab_erase(strlab_struct *str, size_t index, size_t length) {
    return strlab_remove_offset(str, index, length);
}

int strlab_putchar(strlab_struct *str, int chr) {
    if(!isascii(chr)) return STRLAB_INVALID_CHAR;
    char tmp = chr;
    return strlab_copy_offset(str, str->len, &tmp, 1);
}

int strlab_substr(strlab_struct *str, size_t index, size_t length) {
    int error = strlab_remove_offset(str, 0, index);
    if(!error) return error;
    str->len = length;
    str->buf[length] = '\0';
    return error;
}

int strlab_repalce(strlab_struct *str, const strlab_struct *old, const strlab_struct *new) {
    strlab_string result;
    strlab_init_struct(result);

    int error = STRLAB_SUCCESS;
    size_t offset = 0;
    size_t start = 0;

    while(!error && offset < str->len) {
        offset = strlab_search_offset(str, start, old->buf, old->len);
        if(offset == STRLAB_NPOS) offset = str->len;

        error = strlab_copy_offset(result, result->len, str->buf + start, offset - start);
        if(offset != str->len || error) {
            error = strlab_copy_offset(result, result->len, new->buf, new->len);
        }

        start = offset + old->len;
    }

    if(error) {
        strlab_free_struct(result);
        return error;
    }

    return strlab_swap_and_destroy(result, str);
}

int strlab_rchange(strlab_struct *str, const strlab_struct *old, const strlab_struct *new) {
    size_t newlen = new ? new->len : 0;
    const char *newbuf = new ? new->buf : "";

    size_t index = strlab_search_offset(str, 0, old->buf, old->len);
    if(index == STRLAB_NPOS) return STRLAB_NOT_FOUND;

    int error = strlab_shift_offset(str, index, old->len, newlen);
    if(error) return error;
    memcpy(str->buf + index, newbuf, newlen);
    
    return STRLAB_SUCCESS;
}

int strlab_lchange(strlab_struct *str, const strlab_struct *old, const strlab_struct *new) {
    size_t newlen = new ? new->len : 0;
    const char *newbuf = new ? new->buf : "";

    size_t index = strlab_search_backwards(str, old->buf, old->len);
    if(index == STRLAB_NPOS) return STRLAB_NOT_FOUND;

    int error = strlab_shift_offset(str, index, old->len, newlen);
    if(error) return error;
    memcpy(str->buf + index, newbuf, newlen);
    
    return STRLAB_SUCCESS;
}

int strlab_ensure(strlab_struct *str, size_t size) {
    return strlab_ensure_size(str, size);
}

size_t strlab_search(const strlab_struct *str, const strlab_struct *sub) {
    return strlab_search_offset(str, 0, sub->buf, sub->len);
}

const char *strlab_lfind(const strlab_struct *str, const strlab_struct *sub) {
    size_t index = strlab_search_offset(str, 0, sub->buf, sub->len);
    if(index == STRLAB_NPOS) return NULL;
    return str->buf + index;
}

const char *strlab_rfind(const strlab_struct *str, const strlab_struct *sub) {
    size_t index = strlab_search_backwards(str, sub->buf, sub->len);
    if(index == STRLAB_NPOS) return NULL;
    return str->buf + index;
}

int strlab_charat(const strlab_struct *str, size_t index) {
    if(index >= str->len) return EOF;
    return (unsigned char)(str->buf[index]);
}

char *strlab_index(strlab_struct *str, size_t index) {
    if(index >= str->len) return NULL;
    return str->buf + index;
}

int strlab_strip(strlab_struct *str, const strlab_struct *set) {
    const char *setbuf = set ? set->buf : " \f\n\r\t\v";
    size_t setlen = set ? set->len : strlen(setbuf);

    size_t length = 0;
    while(memchr(setbuf, str->buf[length], setlen)) length++;

    if(length > 0) {
        int error = strlab_remove_offset(str, 0, length);
        if(error) return error;
    }

    size_t index = str->len;
    while(memchr(setbuf, str->buf[index - 1], setlen)) index--;

    str->len = index;
    str->buf[index] = '\0';

    return STRLAB_SUCCESS;
}

void strlab_capitalize(strlab_struct *str) {
    for(size_t n = 0; n < str->len; n++) {
        str->buf[n] = toupper((unsigned char)(str->buf[n]));
    }
}

void strlab_lowercase(strlab_struct *str) {
    for(size_t n = 0; n < str->len; n++) {
        str->buf[n] = tolower((unsigned char)(str->buf[n]));
    }
}

int strlab_foreach(strlab_struct *str, int (*callback)(char*, size_t, void*), void *args) {
    for(size_t n = 0; n < str->len; n++) {
        int error = callback(str->buf + n, n, args);
        if(error) return error;
    }
    return STRLAB_SUCCESS;
}

int strlab_printf(strlab_struct *str, const char *format, ...) {
    va_list args, copy;
    va_start(args, format);

    va_copy(copy, args);
    int ilen = vsnprintf(NULL, 0, format, copy);
    va_end(copy);

    if(ilen < 0) {
        va_end(args);
        return ilen;
    }

    size_t len = (size_t)(ilen);
    size_t size = str->len + 1;

    int error = strlab_ensure_size(str, size);
    if(error) {
        va_end(args);
        return error;
    }

    ilen = vsnprintf(str->buf, size, format, args);
    va_end(args);

    if(ilen >= 0) str->len = len;
    return ilen;
}

int strlab_scanf(const strlab_struct *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int count = vsscanf(str->buf, format, args);
    va_end(args);
    return count;
}

char *strlab_relase(strlab_struct *str) {
    if(!str->dynamic) return str->buf;
    if(str->size > STRLAB_SSO_SIZE) return str->buf;

    char *buf = STRLAB_MALLOC(str->len + 1);
    if(!buf) return NULL;
    return memcpy(buf, str->buf, str->len + 1);
}

size_t strlab_count(const strlab_struct *str, const strlab_struct *sub) {
    const char *ptr = str->buf;
    const char *end = ptr + str->len;
    size_t count = 0;

    while((ptr = memmem(ptr, end - ptr, sub->buf, sub->len))) {
        ptr += sub->len;
        count++;
    }

    return count;
}

int strlab_startswith(const strlab_struct *str, const strlab_struct *sub) {
    if(sub->len > str->len) return 0;
    return memcmp(str->buf, sub->buf, str->len) == 0;
}

int strlab_endswith(const strlab_struct *str, const strlab_struct *sub) {
    if(sub->len > str->len) return 0;
    return memcmp(str->buf + str->len - sub->len, sub->buf, sub->len) == 0;
}

int strlab_fgetln(strlab_struct *str, FILE *src) {
    char buf[1024];
    size_t total = 0;

    while(fgets(buf, sizeof(buf), src)) {
        size_t buflen = strlen(buf);        

        int error = strlab_copy_offset(str, total, buf, buflen);
        if(error) return error;

        total += buflen;

        if(buf[buflen - 1] == '\n') break;
    }

    if(ferror(src)) return STRLAB_IO_ERROR;
    return STRLAB_SUCCESS;
}

int strlab_fread(strlab_struct *str, FILE *src, size_t size) {
    int error = strlab_ensure_size(str, size + 1);
    if(error) return error;

    if(size == 0) {
        strlab_clear_struct(str);
        return STRLAB_SUCCESS;
    }

    size_t read = fread(str->buf, 1, size, src);
    str->buf[read] = '\0';

    if(read == 0) return STRLAB_IO_ERROR;
    return STRLAB_SUCCESS;
}

int strlab_fwrite(FILE *dest, const strlab_struct *str) {
    size_t writed = fwrite(str->buf, 1, str->len, dest);
    if(writed) return STRLAB_SUCCESS;
    return STRLAB_IO_ERROR;
}

strlab_string *strlab_split(const strlab_struct *str, const strlab_struct *del) {
    strlab_string *list = NULL;
    size_t listsiz = 0;
    size_t listlen = 0;

    size_t start = 0;
    int error = STRLAB_SUCCESS;

    while(1) {
        size_t offset = strlab_search_offset(str, start, del->buf, del->len);
        if(offset == STRLAB_NPOS) offset = str->len;

        error = strlab_list_append(
            &list, &listsiz, &listlen, str->buf + start, offset - start
        );

        if(error) break;
        if(offset == str->len) break;

        start = offset + del->len;
        if(start > str->len) start = str->len;
    }

    int terminated = !strlab_list_append(
        &list, &listsiz, &listlen, STRLAB_LFIN->buf, STRLAB_LFIN->len
    );

    if(error || !terminated) {
        for(size_t n = 0; n < listlen; n++) {
            strlab_free_struct(list[n]);
        }
        if(list) STRLAB_FREE(list);
        return NULL;
    }

    return list;
}

int strlab_join(strlab_struct *str, const strlab_string *list) {
    int error = STRLAB_SUCCESS;
    strlab_string result;
    strlab_init_struct(result);
    size_t count = 0;

    while(1) {
        const strlab_struct *val = list[count];
        if(strcmp(val->buf, STRLAB_LFIN->buf) == 0) break;

        if(count > 0) {
            error = strlab_copy_offset(result, result->len, str->buf, str->len);
            if(error) break;
        }

        error = strlab_copy_offset(result, result->len, val->buf, val->len);
        if(error) break;

        count++;
    }

    if(error) error = strlab_free_struct(result);
    else error = strlab_swap_and_destroy(result, str);

    return error;
}

int strlab_unlist(strlab_string *list) {
    int error = STRLAB_SUCCESS;
    size_t count = 0;

    while(1) {
        strlab_struct *val = list[count];
        if(strcmp(val->buf, STRLAB_LFIN->buf) == 0) break;
        
        error = error || strlab_free_struct(val);
        count++;
    }

    STRLAB_FREE(list);

    return error;
}

const strlab_string STRLAB_LFIN = {
    {1, 2, (char[2]){[0] = (char)(0xFF), [1] = '\0'}, {0}, 0}
};
