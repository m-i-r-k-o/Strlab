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

static inline char *strlab_getbuf(const strlab_string str) {
    if(str->dat) return str->dat;
    return (char*)(str->little);
}

static inline void strlab_init_struct(strlab_string str) {
    str->len = 0;
    str->size = STRLAB_SSO_SIZE;
    str->dat = NULL;
    str->little[0] = '\0';
    str->dynamic = 1;
}

static inline int strlab_free_struct(strlab_string str) {
    if(!str->dynamic) return STRLAB_FREE_ON_FIXED;
    if(str->dat) STRLAB_FREE(str->dat);
    return STRLAB_SUCCESS;
}

static inline void strlab_clear_struct(strlab_string str) {
    char *strbuf = strlab_getbuf(str);
    strbuf[0] = '\0';
    str->len = 0;
}

static int strlab_ensure_size(strlab_string str, size_t request) {
    if(str->size >= request) return STRLAB_SUCCESS;
    if(!str->dynamic) return STRLAB_MAX_SIZE_REACHED;

    size_t newsiz = strlab_round_size(request);
    
    char *newdat = STRLAB_MALLOC(newsiz);
    if(!newdat) return STRLAB_OUT_OF_MEMORY;

    memcpy(newdat, strlab_getbuf(str), str->len);
    if(str->dat) STRLAB_FREE(str->dat);

    str->dat = newdat;
    str->size = newsiz;
    
    return STRLAB_SUCCESS;
}

static inline int strlab_low_copy(
    strlab_string str, size_t offset, const char *src, size_t srclen
) {
    if(offset > str->len) return STRLAB_OUT_OF_RANGE;
    if(srclen == 0) return STRLAB_SUCCESS;

    size_t newlen = strlab_max(str->len, offset + srclen);
    size_t maxsiz = newlen + 1;
    
    int error = strlab_ensure_size(str, maxsiz);
    if(error) return error;

    char *strbuf = strlab_getbuf(str);
    memcpy(strbuf + offset, src, srclen);
    str->len = newlen;
    strbuf[newlen] = '\0';

    return STRLAB_SUCCESS;
}

static inline int strlab_low_makegap(strlab_string str, size_t offset, size_t space, char fill) {
    if(offset > str->len) return STRLAB_OUT_OF_RANGE;
    if(space == 0) return STRLAB_SUCCESS;

    size_t newlen = str->len + space;
    size_t newsiz = newlen + 1;

    int error = strlab_ensure_size(str, newsiz);
    if(error) return error;
    
    char *strbuf = strlab_getbuf(str);
    memmove(strbuf + offset + space, strbuf + offset, str->len - offset + 1);
    memset(strbuf + offset, fill, space);
    str->len = newlen;

    return STRLAB_SUCCESS;
}

static inline int strlab_low_remove(strlab_string str, size_t offset, size_t torm) {
    if(offset > str->len) return STRLAB_OUT_OF_RANGE;
    if(torm == 0) return STRLAB_SUCCESS;

    if(offset + torm > str->len) torm = str->len - offset;
    size_t newlen = str->len - torm;

    char *strbuf = strlab_getbuf(str);
    memmove(strbuf + offset, strbuf + offset + torm, newlen - offset + 1);
    str->len = newlen;

    return STRLAB_SUCCESS;
}

static size_t strlab_low_search(
    const strlab_string str, size_t offset, const char *target, size_t tarlen
) {
    if(offset > str->len) return STRLAB_NPOS;
    if(tarlen > str->len) return STRLAB_NPOS;
    if(tarlen == 0) return STRLAB_NPOS;
    if(tarlen == str->len) return 0;

    size_t area = str->len - offset;
    const char *strbuf = strlab_getbuf(str);
    const char *found = memmem(strbuf + offset, area, target, tarlen);
    
    if(!found) return STRLAB_NPOS;
    return (size_t)(found - strbuf);
}

static size_t strlab_low_search_backwards(
    const strlab_string str, const char *target, size_t tarlen
) {
    if(tarlen > str->len) return STRLAB_NPOS;
    if(tarlen == 0) return STRLAB_NPOS;
    if(tarlen == str->len) return 0;

    const char *base = strlab_getbuf(str);
    const char *ptr = base + str->len - tarlen;

    while(ptr > base) {
        int found = memcmp(ptr, target, tarlen) == 0;
        if(found) return (size_t)(ptr - base);
        ptr--;
    }

    return STRLAB_NPOS;
}

static int strlab_low_replace(
    strlab_string str, size_t index, size_t oldlen, const char *new, size_t newlen
) {
    if(index > str->len) return STRLAB_OUT_OF_RANGE;

    int error = STRLAB_SUCCESS;

    if(oldlen < newlen) {
        size_t length = newlen - oldlen;
        error = strlab_low_makegap(str, index + oldlen, length, '\0');
    } else if(oldlen > newlen) {
        size_t length = oldlen - newlen;
        error = strlab_low_remove(str, index + newlen, length);
    }

    if(error) return error;
    error = strlab_low_copy(str, index, new, newlen);
    index += oldlen;
    return error;
}

static int strlab_low_list_append(
    strlab_string **list, size_t *size, size_t *len, const char *src, size_t srclen
) {
    if(*len + 1 >= *size) {
        size_t newsiz = strlab_round_size(*size + 1);

        strlab_string *newdat = STRLAB_MALLOC(newsiz * sizeof(strlab_string));
        if(!newdat) return STRLAB_OUT_OF_MEMORY;

        if(*list) {
            memcpy(newdat, *list, *size * sizeof(strlab_string));
            STRLAB_FREE(*list);
        }

        *list = newdat;
        *size = newsiz;
    }

    strlab_string *elem = &(*list)[*len];
    strlab_init_struct(*elem);

    int error = strlab_low_copy(*elem, 0, src, srclen);
    if(error) {
        strlab_free_struct(*elem);
        return error;
    }

    *len += 1;
    return STRLAB_SUCCESS;
}

void strlab_init(strlab_string str) {
    strlab_init_struct(str);
}

int strlab_free(strlab_string str) {
    return strlab_free_struct(str);
}

void strlab_attach(strlab_string str, char *buf, size_t size) {
    str->len = strlen(buf);
    str->size = size;
    str->dat = buf;
    str->little[0] = '\0';
    str->dynamic = 0;
}

void strlab_log(const strlab_string string) {
    printf("'%s' len=%zu size=%zu\n", strlab_cstring(string), string->len, string->size);
}

void strlab_clear(strlab_string str) {
    if(!str->dynamic) strlab_free_struct(str);
    strlab_init_struct(str);
}

int strlab_isempty(const strlab_string str) {
    const char *strbuf = strlab_getbuf(str);
    return strbuf[0] == '\0';
}

size_t strlab_length(const strlab_string str) {
    return str->len;
}

const char *strlab_cstring(const strlab_string str) {
    return strlab_getbuf(str);
}

int strlab_greater(const strlab_string str1, const strlab_string str2) {
    const char *buf1 = strlab_getbuf(str1);
    const char *buf2 = strlab_getbuf(str2);
    return strcmp(buf1, buf2) > 0;
}

int strlab_equals(const strlab_string str1, const strlab_string str2) {
    const char *buf1 = strlab_getbuf(str1);
    const char *buf2 = strlab_getbuf(str2);
    return str1->len == str2->len && memcmp(buf1, buf2, str1->len) == 0;
}

int strlab_less(const strlab_string str1, const strlab_string str2) {
    const char *buf1 = strlab_getbuf(str1);
    const char *buf2 = strlab_getbuf(str2);
    return strcmp(buf1, buf2) < 0;
}

int strlab_compare(const strlab_string str1, const strlab_string str2) {
    const char *buf1 = strlab_getbuf(str1);
    const char *buf2 = strlab_getbuf(str2);
    return strcmp(buf1, buf2);
}

int strlab_copy(strlab_string str, const strlab_string other) {
    const char *otherbuf = strlab_getbuf(other);
    return strlab_low_copy(str, 0, otherbuf, other->len);
}

int strlab_insert(strlab_string str, size_t index, const strlab_string other) {
    int error = strlab_low_makegap(str, index, other->len, '\0');
    if(error) return error;

    const char *otherbuf = strlab_getbuf(other);
    return strlab_low_copy(str, index, otherbuf, other->len); 
}

int strlab_append(strlab_string str, const strlab_string other) {
    const char *otherbuf = strlab_getbuf(other);
    return strlab_low_copy(str, str->len, otherbuf, other->len);
}

int strlab_erase(strlab_string str, size_t index, size_t length) {
    return strlab_low_remove(str, index, length);
}

int strlab_putchar(strlab_string str, int chr) {
    if(!isascii(chr)) return STRLAB_INVALID_CHAR;
    char tmp = chr;
    return strlab_low_copy(str, str->len, &tmp, 1);
}

int strlab_substr(strlab_string str, size_t index, size_t length) {
    int error = strlab_low_remove(str, 0, index);
    if(error) return error;
    return strlab_low_remove(str, index, str->len - index);
}

int strlab_repalce(strlab_string str, const strlab_string old, const strlab_string new) {
    size_t offset = 0;
    int error = STRLAB_SUCCESS;

    const char *oldbuf = strlab_getbuf(old);
    const char *newbuf = new ? strlab_getbuf(new) : "";
    size_t newlen = new ? new->len : 0;

    while(error == STRLAB_SUCCESS) {
        size_t index = strlab_low_search(str, offset, oldbuf, old->len);
        if(index == STRLAB_NPOS) break;
        error = strlab_low_replace(str, index, old->len, newbuf, newlen);
    }

    return error;
}

int strlab_rchange(strlab_string str, const strlab_string old, const strlab_string new) {
    const char *oldbuf = strlab_getbuf(old);
    const char *newbuf = new ? strlab_getbuf(new) : "";
    size_t newlen = new ? new->len : 0;

    size_t index = strlab_low_search(str, 0, oldbuf, old->len);
    if(index == STRLAB_NPOS) return STRLAB_SUCCESS;

    return strlab_low_replace(str, index, old->len, newbuf, newlen);
}

int strlab_lchange(strlab_string str, const strlab_string old, const strlab_string new) {
    const char *oldbuf = strlab_getbuf(old);
    const char *newbuf = new ? strlab_getbuf(new) : "";
    size_t newlen = new ? new->len : 0;

    size_t index = strlab_low_search_backwards(str, oldbuf, old->len);
    if(index == STRLAB_NPOS) return STRLAB_SUCCESS;

    return strlab_low_replace(str, index, old->len, newbuf, newlen);
}

int strlab_ensure(strlab_string str, size_t size) {
    return strlab_ensure_size(str, size);
}

size_t strlab_search(const strlab_string str, const strlab_string sub) {
    const char *subbuf = strlab_getbuf(sub);
    return strlab_low_search(str, 0, subbuf, sub->len);
}

const char *strlab_lfind(const strlab_string str, const strlab_string sub) {
    const char *subbuf = strlab_getbuf(sub);
    const char *strbuf = strlab_getbuf(str);

    size_t index = strlab_low_search(str, 0, subbuf, sub->len);
    if(index == STRLAB_NPOS) return NULL;
    return strbuf + index;
}

const char *strlab_rfind(const strlab_string str, const strlab_string sub) {
    const char *subbuf = strlab_getbuf(sub);
    const char *strbuf = strlab_getbuf(str);

    size_t index = strlab_low_search_backwards(str, subbuf, sub->len);
    if(index == STRLAB_NPOS) return NULL;
    return strbuf + index;
}

int strlab_index(const strlab_string str, size_t index) {
    if(index >= str->len) return EOF;
    const char *strbuf = strlab_getbuf(str);
    return strbuf[index];
}

char *strlab_offset(strlab_string str, size_t index) {
    if(index >= str->len) return NULL;
    char *strbuf = strlab_getbuf(str);
    return strbuf + index;
}

int strlab_strip(strlab_string str, const strlab_string set) {
    const char *strbuf = strlab_getbuf(str);
    const char *setbuf = set ? strlab_getbuf(set) : " \f\n\r\t\v";
    size_t setlen = set ? set->len : strlen(setbuf);

    size_t length = 0;
    while(memmem(setbuf, setlen, strbuf + length, 1)) length++;

    int error = strlab_low_remove(str, 0, length);
    if(error) return error;

    size_t index = str->len - 1;
    while(memmem(setbuf, setlen, strbuf - index, 1)) index++;

    return strlab_low_remove(str, index, str->len - index);
}

void strlab_capitalize(strlab_string str) {
    char *strbuf = strlab_getbuf(str);
    for(size_t n = 0; n < str->len; n++) {
        strbuf[n] = toupper((unsigned char)strbuf[n]);
    }
}

void strlab_lowercase(strlab_string str) {
    char *strbuf = strlab_getbuf(str);
    for(size_t n = 0; n < str->len; n++) {
        strbuf[n] = tolower((unsigned char)strbuf[n]);
    }
}

int strlab_foreach(strlab_string str, int (*callback)(char*, size_t, void*), void *args) {
    char *strbuf = strlab_getbuf(str);

    for(size_t n = 0; n < str->len; n++) {
        int error = callback(strbuf + n, n, args);
        if(error) return error;
    }

    return STRLAB_SUCCESS;
}

int strlab_printf(strlab_string str, const char *format, ...) {
    va_list args, copy;
    va_start(args, format);

    va_copy(copy, args);
    int ilen = vsnprintf(NULL, 0, format, copy);
    va_end(copy);

    if(ilen < 0) {
        va_end(args);
        return STRLAB_PRINTF_ERROR;
    }

    size_t len = (size_t)(ilen);
    size_t size = str->len + 1;

    int error = strlab_ensure_size(str, size);
    if(error) {
        va_end(args);
        return error;
    }

    char *strbuf = strlab_getbuf(str);
    error = vsnprintf(strbuf, size, format, args) < 0;
    va_end(args);

    if(error) return STRLAB_PRINTF_ERROR;
    str->len = len;

    return STRLAB_SUCCESS;
}

int strlab_scanf(const strlab_string str, const char *format, ...) {
    va_list args;
    va_start(args, format);

    const char *strbuf = strlab_getbuf(str);
    int count = vsscanf(strbuf, format, args);

    va_end(args);
    if(count <= 0) return STRLAB_SCANF_ERROR;
    return STRLAB_SUCCESS;
}

char* strlab_relase(strlab_string str) {
    size_t size = str->len + 1;
    char *raw = STRLAB_MALLOC(size);
    if(!raw) return NULL;

    const char *buf = strlab_getbuf(str);
    memcpy(raw, buf, size);

    if(strlab_free_struct(str)) {
        STRLAB_FREE(raw);
        return NULL;
    }

    return raw;
}

size_t strlab_count(const strlab_string str, const strlab_string sub) {
    const char *ptr = strlab_getbuf(str);
    const char *endbuf = ptr + str->len;
    
    const char *subbuf = strlab_getbuf(sub);
    size_t count = 0;

    while((ptr = memmem(ptr, endbuf - ptr, sub, sub->len))) {
        count++;
        ptr += sub->len;
    }

    return count;
}

int strlab_startswith(const strlab_string str, const strlab_string sub) {
    if(sub->len > str->len) return 0;

    const char *strbuf = strlab_getbuf(str);
    const char *subbuf = strlab_getbuf(sub);
    return memcmp(strbuf, subbuf, sub->len) == 0;
}

int strlab_endswith(const strlab_string str, const strlab_string sub) {
    if(sub->len > str->len) return 0;

    const char *endbuf = strlab_getbuf(str);
    const char *subbuf = strlab_getbuf(sub);
    return memcmp(endbuf - sub->len, subbuf, sub->len) == 0;
}

int strlab_fgetln(strlab_string str, FILE *src) {
    char buf[1024];
    size_t total = 0;

    while(fgets(buf, sizeof(buf), src)) {
        size_t buflen = strlen(buf);        
        
        int error = strlab_low_copy(str, total, buf, buflen);
        if(error) return error;

        total += buflen;

        if(buf[buflen - 1] == '\n') break;
    }

    if(ferror(src)) return STRLAB_IO_ERROR;
    return STRLAB_SUCCESS;
}

int strlab_fread(strlab_string str, FILE *src, size_t size) {
    int error = strlab_ensure_size(str, size + 1);
    if(error) return error;

    char *strbuf = strlab_getbuf(str);

    if(size == 0) {
        strlab_clear_struct(str);
        return STRLAB_SUCCESS;
    }

    size_t read = fread(strbuf, 1, size, src);
    strbuf[read] = '\0';

    if(read == 0) return STRLAB_IO_ERROR;
    return STRLAB_SUCCESS;
}

int strlab_fwrite(FILE *dest, const strlab_string str) {
    const char *strbuf = strlab_getbuf(str);
    size_t writed = fwrite(strbuf, 1, str->len, dest);
    if(writed == 0) return STRLAB_IO_ERROR;
    return STRLAB_SUCCESS;
}

strlab_string *strlab_split(const strlab_string str, const strlab_string del) {
    strlab_string *list = NULL;
    size_t listsiz = 0;
    size_t listlen = 0;

    const char *strbuf = strlab_getbuf(str);
    const char *delbuf = strlab_getbuf(del);

    size_t offset = 0;
    size_t start = 0;
    int error = STRLAB_SUCCESS;

    while(offset < str->len) {
        offset = strlab_low_search(str, start, delbuf, del->len);
        if(offset == STRLAB_NPOS) offset = str->len;

        error = strlab_low_list_append(
            &list, &listsiz, &listlen, strbuf + start, offset - start
        );

        if(error) break;

        start = offset + del->len;
    }

    const char *fin = strlab_getbuf(STRLAB_LFIN);
    int termineted = !strlab_low_list_append(
        &list, &listsiz, &listlen, fin, STRLAB_LFIN->len
    );

    if(error || !termineted) {
        for(size_t n = 0; n < listlen; n++) {
            strlab_free_struct(list[n]);
        }
        if(list) STRLAB_FREE(list);
        return NULL;
    }

    return list;
}

int strlab_join(strlab_string str, const strlab_string *const list) {
    int error = STRLAB_SUCCESS;
    strlab_string sep;

    strlab_init_struct(sep);
    error =  strlab_copy(sep, str);
    if(error) return error;

    strlab_clear_struct(str);

    const char *sepbuf = strlab_getbuf(sep);
    const char *finbuf = strlab_getbuf(STRLAB_LFIN);
    size_t n = 0;

    while(1) {
        const strlab_string *val = &list[n];
        const char *valbuf = strlab_getbuf(*val);
        if(strcmp(valbuf, finbuf) == 0) break;

        if(n > 0) {
            error = strlab_low_copy(str, str->len, sepbuf, sep->len);
            if(error) break;
        }

        error = strlab_low_copy(str, str->len, valbuf, (*val)->len);
        if(error) break;

        n++;
    }

    strlab_free_struct(sep);
    return error;
}

int strlab_unlist(strlab_string *list) {
    const char *finbuf = strlab_getbuf(STRLAB_LFIN);
    int error = STRLAB_SUCCESS;
    size_t n = 0;

    while(1) {
        strlab_string *val = &list[n];
        const char *valbuf = strlab_getbuf(*val);
        if(strcmp(valbuf, finbuf) == 0) break;
        
        error = error || strlab_free_struct(*val);
        n++;
    }

    STRLAB_FREE(list);

    return error;
}
