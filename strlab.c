#include "strlab.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

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

static inline int strlab_string_little(const strlab_string string) {
    return string->dat == NULL;
}

static inline char *strlab_string_dat(const strlab_string string) {
    if(strlab_string_little(string)) return (char*)string->little;
    return string->dat;
}

static int strlab_string_ensure(strlab_string string, size_t newsiz) {
    if(string->size >= newsiz) return 0;

    newsiz = strlab_round_size(newsiz);

    char *newdat = STRLAB_MALLOC(newsiz);
    if(!newdat) return ENOMEM;

    char *src = strlab_string_dat(string);
    memcpy(newdat, src, string->len);

    if(!strlab_string_little(string)) {
        STRLAB_FREE(string->dat);
    }

    string->size = newsiz;
    string->dat = newdat;

    return 0;
}

static void strlab_string_create(strlab_string string) {
    string->size = STRLAB_SIZE;
    string->len = 0;
    string->dat = NULL;
    memset(string->little, 0, STRLAB_SIZE);
}

static void strlab_string_delete(strlab_string string) {
    if(strlab_string_little(string)) return;
    STRLAB_FREE(string->dat);
}

static int strlab_string_copy(strlab_string string, const char *str, size_t len) {
    int res = strlab_string_ensure(string, len + 1);
    if(res != 0) return res;

    char *dat = strlab_string_dat(string);
    memcpy(dat, str, len);

    string->len = len;
    dat[len] = '\0';

    return 0;
}

static int strlab_string_append(strlab_string string, const char *str, size_t len) {
    int res = strlab_string_ensure(string, string->len + len + 1);
    if(res != 0) return res;

    char* dat = strlab_string_dat(string);
    memcpy(dat + string->len, str, len);

    string->len += len;
    dat[string->len] = '\0';

    return 0;
}

static int strlab_string_insert(strlab_string string, size_t index, const char *str, size_t len) {
    int res = strlab_string_ensure(string, string->len + len + 1);
    if(res != 0) return res;

    char *dat = strlab_string_dat(string);
    memmove(dat + index + len, dat + index, len);
    memcpy(dat + index, str, len);

    string->len += len;
    dat[string->len] = '\0';

    return 0;
}

static int strlab_string_remove(strlab_string string, size_t index, size_t length) {
    if(index + length >= string->len) return ERANGE;

    char *dat = strlab_string_dat(string);
    memmove(dat + index, dat + index + length, length);

    return 0;
}

static void *strlab_string_memrmem(const void *_haystack, size_t hlen, const void *_needle, size_t nlen) {
    if(nlen == 0 || hlen < nlen) return NULL;

    const uint8_t *haystack = _haystack;
    const uint8_t *needle = _needle;

    for(size_t i = hlen - nlen + 1; i-- > 0;) {
        if(memcmp(haystack + i, needle, nlen) == 0) {
            return (void*)(haystack + i);
        }
    }

    return NULL;
}

static int strlab_string_pos_replace(
    strlab_string string, const char *old, size_t oldlen, const char *new, size_t newlen,
    void *(*mem)(const void*, size_t, const void*, size_t)
) {
    if(oldlen == 0) return 0;

    char *dat = strlab_string_dat(string);
    char *pos = mem(dat, string->len, old, oldlen);
    if(!pos) return 0;

    size_t offset = pos - dat;
    size_t totlen = string->len - oldlen + newlen;

    if(totlen + 1 > string->size) {
        int res = strlab_string_ensure(string, totlen + 1);
        if(res != 0) return ENOMEM;

        dat = strlab_string_dat(string);
        pos = dat + offset;
    }

    size_t tailoff = offset + oldlen;
    size_t taillen = string->len - tailoff + 1;

    memmove(pos + newlen, pos + oldlen, taillen);
    memcpy(pos, new, newlen);

    string->len = totlen;
    return 0;
}

static int strlab_string_replace(
    size_t *countout, strlab_string string,
    const char *old, size_t oldlen, const char *new, size_t newlen
) {
    if(oldlen == 0) return 0;

    int res = 0;
    size_t count = 0;
    size_t curlen = string->len;
    size_t curoff = 0;

    while(1) {
        char *dat = strlab_string_dat(string);
        char *pos = memmem(dat + curoff, curlen - curoff, old, oldlen);
        if(!pos) break;

        size_t offset = pos - dat;
        size_t totlen = curlen - oldlen + newlen;

        if(totlen + 1 > string->size) {
            res = strlab_string_ensure(string, totlen + 1);
            if(res != 0) break;

            dat = strlab_string_dat(string);
            pos = dat + offset;
        }

        size_t tailoff = offset + oldlen;
        size_t taillen = curlen - tailoff + 1;

        memmove(pos + newlen, pos + oldlen, taillen);
        memcpy(pos, new, newlen);

        string->len = curlen = totlen;

        curoff = offset + newlen;
        count++;
    }
    
    char *dat = strlab_string_dat(string);
    dat[string->len] = '\0';
    *countout = count;

    return res;
}

void strlab_create(strlab_string string) {
    strlab_string_create(string);
}

void strlab_delete(strlab_string string) {
    strlab_string_delete(string);
}

void strlab_clear(strlab_string string) {
    if(!strlab_string_little(string)) {
        STRLAB_FREE(string->dat);
    }

    string->len = 0;
    string->size = STRLAB_SIZE;
    string->dat = NULL;
    memset(string->little, 0, STRLAB_SIZE);
}

int strlab_empty(const strlab_string string) {
    const char *dat = strlab_string_dat(string);
    return dat[0] == '\0';
}

size_t strlab_length(const strlab_string string) {
    return string->len;
}

const char *strlab_cstring(const strlab_string string) {
    return strlab_string_dat(string);
}

void strlab_copy(strlab_string string, const char *dat) {
    size_t len = strlen(dat);
    int res = strlab_string_copy(string, dat, len);
    if(res != 0) {
        errno = res;
        STRLAB_ERROR(strlab_copy);
    }
}

void strlab_append(strlab_string string, const char *dat) {
    size_t len = strlen(dat);
    int res = strlab_string_append(string, dat, len);
    if(res != 0) {
        errno = res;
        STRLAB_ERROR(strlab_append);
    }
}

void strlab_putchar(strlab_string string, char chr) {
    int res = strlab_string_ensure(string, string->len + 2);
    if(res != 0) {
        errno = res;
        STRLAB_ERROR(strlab_putchar);
        return;
    }

    char *dest = strlab_string_dat(string);
    dest[string->len++] = chr;
}

void strlab_substr(strlab_string string, size_t index, size_t length) {
    if(index > string->len) return;

    if(index + length >= string->len) {
        length = string->len - index;
    }

    char *dat = strlab_string_dat(string);
    memmove(dat, dat + index, length);

    dat[length] = '\0';
    string->len = length;
}

void strlab_freplace(strlab_string string, const char *old, const char *new) {
    size_t oldlen = strlen(old);
    size_t newlen = strlen(new);

    int res = strlab_string_pos_replace(string, old, oldlen, new, newlen, memmem);
    if(res != 0) {
        errno = res;
        STRLAB_ERROR(strlab_freplace);
    }
}

void strlab_lreplace(strlab_string string, const char *old, const char *new) {
    size_t oldlen = strlen(old);
    size_t newlen = strlen(new);

    int res = strlab_string_pos_replace(string, old, oldlen, new, newlen, strlab_string_memrmem);
    if(res != 0) {
        errno = res;
        STRLAB_ERROR(strlab_lreplace);
    }
}

size_t strlab_replace(strlab_string string, const char *old, const char *new) {
    size_t oldlen = strlen(old);
    size_t newlen = strlen(new);
    size_t count = 0; 

    int res = strlab_string_replace(&count, string, old, oldlen, new, newlen);
    if(res != 0) {
        errno = res;
        STRLAB_ERROR(strlab_replace);
    }
    return count;
}

void strlab_resize(strlab_string string, size_t size) {
    int res = strlab_string_ensure(string, size);
    if(res != 0) {
        errno = ENOMEM;
        STRLAB_ERROR(strlab_resize);
    }
}

int strlab_charat(const strlab_string string, size_t index) {
    if(index >= string->len) return EOF;
    char *dat = strlab_string_dat(string);
    return dat[index];
}

void strlab_setchar(strlab_string string, size_t index, int c) {
    if(index >= string->len || !isascii(c) ) return;
    char *dat = strlab_string_dat(string);
    dat[index] = c;
}

void strlab_trim(strlab_string string) {
    char *dat = strlab_string_dat(string);

    size_t tail = 0;
    while(isspace(dat[tail])) tail++;

    size_t head = 0;

    if(tail != string->len) {
        while(isspace(dat[string->len - head - 1])) head++;
    }
    
    size_t newlen = string->len - tail - head;

    memmove(dat, dat + tail, newlen);
    dat[newlen] = '\0';
    string->len = newlen;
}

void strlab_capitalize(strlab_string string) {
    char *dat = strlab_string_dat(string);

    for(size_t n = 0; n < string->len; n++) {
        dat[n] = toupper(dat[n]);
    }
}

void strlab_lower(strlab_string string) {
    char *dat = strlab_string_dat(string);

    for(size_t n = 0; n < string->len; n++) {
        dat[n] = tolower(dat[n]);
    }
}

void strlab_map(strlab_string string, int (*callback)(int, size_t, void*), void *args) {
    char *dat = strlab_string_dat(string);

    for(size_t n = 0; n < string->len; n++) {
        int c = callback(dat[n], n, args);
        if(c == EOF) break;
        dat[n] = c;
    }
}

void strlab_printf(strlab_string string, const char *format, ...) {
    va_list args, copy;
    va_start(args, format);

    va_copy(copy, args);
    int ilen = vsnprintf(NULL, 0, format, copy);
    va_end(copy);

    if(ilen < 0) {
        va_end(args);
        errno = EILSEQ;
        STRLAB_ERROR(strlab_printf);
        return;
    }

    size_t len = (size_t)(ilen);
    size_t size = string->len + len + 1;

    int res = strlab_string_ensure(string, size);
    if(res != 0) {
        va_end(args);
        errno = res;
        STRLAB_ERROR(strlab_printf);
        return;
    }

    char *dat = strlab_string_dat(string);
    int error = vsnprintf(dat + string->len, len + 1, format, args) < 0;
    va_end(args);

    if(error) {
        errno = EILSEQ;
        STRLAB_ERROR(strlab_printf);
        return;
    }

    string->len += len;
}

int strlab_scanf(const strlab_string string, const char *format, ...) {
    va_list args;
    va_start(args, format);

    const char *dat = strlab_string_dat(string);
    int res = vsscanf(dat, format, args);

    va_end(args);
    return res;
}

char *strlab_drop(strlab_string string) {
    if(strlab_string_little(string)) {
        char *dat = STRLAB_MALLOC(string->len + 1);
        if(!dat) {
            errno = ENOMEM;
            STRLAB_ERROR(strlab_drop);
            return NULL;
        }

        memcpy(dat, string->dat, string->len + 1);
        return dat;
    }

    return string->dat;
}

size_t strlab_split(strlab_string **list, const strlab_string string, const char *delim) {
    const char *base = strlab_string_dat(string);
    const char *end = base + string->len;
    const char *ptr = base;
    const char *term = NULL;

    size_t dellen = strlen(delim);
    if(dellen == 0) return 0;

    size_t lstlen = 0;
    size_t lstsiz = 0;

    while(1) {
        size_t n = string->len - (ptr - base);
        term = memmem(ptr, n, delim, dellen);
        if(!term) term = end;

        if(lstlen >= lstsiz) {
            size_t newsiz = strlab_round_size(lstsiz + 1);

            strlab_string *newlst = STRLAB_MALLOC(newsiz * sizeof(strlab_string));
            if(!newlst) {
                errno = ENOMEM;
                STRLAB_ERROR(slbdsplt);
                goto freelist;
            }

            if(*list) {
                memcpy(newlst, *list, lstlen * sizeof(strlab_string));
                STRLAB_FREE(*list);
            }

            lstsiz = newsiz;
            *list = newlst;
        }

        strlab_string *elem = &(*list)[lstlen++];
        strlab_string_create(*elem);

        size_t len = term - ptr;
        if(strlab_string_copy(*elem, ptr, len)) {
            errno = ENOMEM;
            STRLAB_ERROR(strlab_split);
            goto freelist;
        }

        if(term == end) break;
        ptr = term + dellen;
    }

    return lstlen;

    freelist:
    if(!*list) return 0;
    for(size_t n = 0; n < lstlen; n++) {
        strlab_string_delete(*list[n]);
    }
    STRLAB_FREE(*list);
    return 0;
}

size_t strlab_split_into(strlab_string *list, size_t size, const strlab_string string, const char *delim, int defined) {
    const char *base = strlab_string_dat(string);
    const char *end = base + string->len;
    const char *ptr = base;
    const char *term = NULL;

    size_t dellen = strlen(delim);
    if(dellen == 0) return 0;

    size_t lstlen = 0;

    while(1) {
        size_t n = string->len - (ptr - base);
        term = memmem(ptr, n, delim, dellen);
        if(!term) term = end;

        if(lstlen >= size) break;

        strlab_string *elem = &list[lstlen++];
        if(!defined) strlab_string_create(*elem);

        size_t len = term - ptr;
        if(strlab_string_copy(*elem, ptr, len)) {
            errno = ENOMEM;
            STRLAB_ERROR(strlab_split_into);
            goto freelist;
        }

        if(term == end) break;
        ptr = term + dellen;
    }

    return lstlen;

    freelist:
    if(defined) return 0;
    for(size_t n = 0; n < lstlen; n++) {
        strlab_string_delete(list[n]);
    }
    return 0;
}

void strlab_join(strlab_string string, const strlab_string *list, size_t nel, const char *sep) {
    size_t seplen = strlen(sep);

    for(size_t n = 0; n < nel; n++) {
        const strlab_string *elem = &list[n];

        if(n != 0) {
            if(strlab_string_append(string, sep, seplen)) {
                errno = ENOMEM;
                STRLAB_ERROR(strlab_join);
                return;
            }
        }

        char *dat = strlab_string_dat(*elem);
        if(strlab_string_append(string, dat, (*elem)->len)) {
            errno = ENOMEM;
            STRLAB_ERROR(strlab_join);
            return;
        }
    }
}

void strlab_work(strlab_string string, void (*wdat)(char*, size_t*), size_t ensure) {
    if(strlab_string_ensure(string, ensure)) {
        errno = ENOMEM;
        STRLAB_ERROR(strlab_work);
        return;
    }

    char *dat = strlab_string_dat(string);
    wdat(dat, &string->len);
}

void strlab_fgetln(strlab_string string, FILE *file) {
    int c = 0;

    while((c = fgetc(file)) != EOF) {
        char ch = c;
        if(strlab_string_append(string, &ch, 1)) {
            errno = ENOMEM;
            STRLAB_ERROR(slbline);
            break;
        }

        if(c == '\n') break;
    }
}

void strlab_fread(strlab_string string, FILE *file) {
    int c = 0;
    
    while((c = fgetc(file)) != EOF) {
        char ch = c;
        if(strlab_string_append(string, &ch, 1)) {
            errno = ENOMEM;
            STRLAB_ERROR(slbread);
            break;
        }
    }
}

void strlab_fwrite(FILE *file, const strlab_string string) {
    const char *dat = strlab_string_dat(string);
    if(fputs(dat, file) == EOF) {
        STRLAB_ERROR(strlab_fwrite);
        return;
    }
}

size_t strlab_tokenize(strlab_string **list, const strlab_string string, const char *delim) {
    const char *base = strlab_string_dat(string);
    const char *end = base + string->len;
    const char *ptr = base;
    const char *term = NULL;

    size_t dellen = strlen(delim);
    if(dellen == 0) return 0;

    size_t lstlen = 0;
    size_t lstsiz = 0;

    while(1) {
        term = strpbrk(ptr, delim);
        if(!term) term = end;

        if(lstlen >= lstsiz) {
            size_t newsiz = strlab_round_size(lstsiz + 1);

            strlab_string *newlst = STRLAB_MALLOC(newsiz * sizeof(strlab_string));
            if(!newlst) {
                errno = ENOMEM;
                STRLAB_ERROR(strlab_tokenize);
                goto freelist;
            }

            if(*list) {
                memcpy(newlst, *list, lstlen * sizeof(strlab_string));
                STRLAB_FREE(*list);
            }

            lstsiz = newsiz;
            *list = newlst;
        }

        strlab_string *elem = &(*list)[lstlen++];
        strlab_string_create(*elem);

        size_t len = term - ptr;
        if(strlab_string_copy(*elem, ptr, len)) {
            errno = ENOMEM;
            STRLAB_ERROR(strlab_tokenize);
            goto freelist;
        }

        if(term == end) break;

        ptr = term + 1;
        while(memchr(delim, *ptr, dellen)) ptr++;
    }

    return lstlen;

    freelist:
    if(!*list) return 0;
    for(size_t n = 0; n < lstlen; n++) {
        strlab_string_delete(*list[n]);
    }
    STRLAB_FREE(*list);
    return 0;
}

size_t strlab_tokenize_into(strlab_string *list, size_t size, const strlab_string string, const char *delim, int defined) {
    const char *base = strlab_string_dat(string);
    const char *end = base + string->len;
    const char *ptr = base;
    const char *term = NULL;

    size_t dellen = strlen(delim);
    if(dellen == 0) return 0;

    size_t lstlen = 0;

    while(1) {
        term = strpbrk(ptr, delim);
        if(!term) term = end;

        if(lstlen >= size) break;

        strlab_string *elem = &list[lstlen++];
        if(!defined) strlab_string_create(*elem);

        size_t len = term - ptr;
        if(strlab_string_copy(*elem, ptr, len)) {
            errno = ENOMEM;
            STRLAB_ERROR(strlab_tokenize);
            goto freelist;
        }

        if(term == end) break;

        ptr = term + 1;
        while(memchr(delim, *ptr, dellen)) ptr++;
    }

    return lstlen;

    freelist:
    if(defined) return 0;
    for(size_t n = 0; n < lstlen; n++) {
        strlab_string_delete(list[n]);
    }
    return 0;
}

size_t strlab_count(const strlab_string string, const char *substr) {
    const char *base = strlab_string_dat(string);
    const char *end = base + string->len;

    const char *ptr = base;
    size_t count = 0;
    size_t sublen = strlen(substr);

    while((ptr = memmem(ptr, end - ptr, substr, sublen))) {
        count++;
        ptr += sublen;
    }

    return count;
}

int strlab_startswith(const strlab_string string, const char *substr) {
    size_t sublen = strlen(substr);
    if(sublen > string->len) return 0;

    const char *dat = strlab_string_dat(string);
    return memcmp(dat, substr, sublen);
}

int strlab_endswith(const strlab_string string, const char *substr) {
    size_t sublen = strlen(substr);
    if(sublen > string->len) return 0;

    const char *dat = strlab_string_dat(string);
    size_t diff = string->len - sublen;

    return memcmp(dat - diff, substr, sublen) == 0;
}

size_t strlab_search(const strlab_string string, const char *substr) {
    const char *dat = strlab_string_dat(string);

    size_t sublen = strlen(substr);
    const char *found = memmem(dat, string->len, substr, sublen);
    return found - dat;
}
