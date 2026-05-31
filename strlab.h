/**
 * @file strlab.c
 * @brief SSO optimized string library
 * @author Vitolo Mirko
 * @date 2026-04-30
 */

#ifndef STRLAB_H
#define STRLAB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef STRLAB_MALLOC
/**
 * @brief Customizable macro to allocate memory
 * @param siz Size in bytes to allocate
 * @return Pointer to allocated memory
 * @code{.c}
 * #define STRLAB_MALLOC(siz) my_allocator(siz)
 * @endcode
 */
#define STRLAB_MALLOC(siz) malloc(siz)
#endif

#ifndef STRLAB_FREE
/**
 * @brief Macro to free allocated memory
 * @param ptr Memory pointer to deallocate
 * @code{.c}
 * #define STRLAB_FREE(ptr) my_free(ptr)
 * @endcode
 */
#define STRLAB_FREE(ptr) free(ptr)
#endif

#ifndef STRLAB_SSO_SIZE
/**
 * @brief Size of the SSO string
 * @code{.c}
 *#define STRLAB_SSO_SIZE 32
 * @endcode
 */
#define STRLAB_SSO_SIZE 16
#endif

#ifndef STRLAB_NPOS
/**
 * @brief Value indicating an invalid index
 * @code{.c}
 * //str = "abc", sub = "123"
 * size_t index = strlab_search(str, sub);
 * //index = STRLAB_NPOS
 * @endcode
 */
#define STRLAB_NPOS ((size_t)(-1))
#endif

#ifndef STRLAB_LFIN
/**
 * @brief String indicating the end of a list of strings
 * @code{.c}
 * //list = {"a", "b", "c", STRLAB_LFIN}
 * for(size_t n = 0; !strlab_equals(list[n], STRLAB_LFIN); n++) {
 *     printf("%s", strlab_cstring(list[n]));
 *}
 * //output -> abc
 * @endcode
 */
#define STRLAB_LFIN (const strlab_string){{1, 2, (char[2]){(char)(0xFF), '\0'}, {0}, 0}}
#endif

/**
 * @brief Create a temporary constant string using a C constant string as a resource
 * @code{.c}
 * strlab_copy(str, strlab_expr("abc"));
 * //str = "abc"
 * @endcode
 */
#define strlab_expr(expr) ((const strlab_string){{sizeof((expr)) - 1, sizeof((expr)), (expr), {0}, 0}})

/**
 * @brief Initialize a constant string using a C constant string as a resource
 * @code{.c}
 *const strlab_string cstr = strlab_const("abc");
 * //cstr = "abc"
 * @endcode
 */
#define strlab_const(expr) {{sizeof((expr)) - 1, sizeof((expr)), (expr), {0}, 0}}

enum {
    STRLAB_SUCCESS = 0, /** Value indicating the success of a function is always 0 */
    STRLAB_MAX_SIZE_REACHED, /**< Value indicating the maximum achievement of a size of a non-dynamic string */
    STRLAB_OUT_OF_MEMORY, /**< Value indicating that the allocable memory is finite */
    STRLAB_OUT_OF_RANGE, /**< Value indicating that too large an offset has been applied */
    STRLAB_INVALID_CHAR, /**< Value indicating the insertion of an incorrect character in a string */
    STRLAB_FREE_ON_FIXED, /**< Value indicating the attempted freeing of memory of a non-dynamic string */
    STRLAB_IO_ERROR, /**< Value indicating a failure of the IO functions */
    STRLAB_NOT_FOUND /**< Value indicating that a search for an element was unsuccessful */
};

/**
 * @brief Structure for an SSO string
 * @note It is preferable not to use it
 */
struct strlab_string_struct {
    size_t len;
    size_t size;
    char *buf;
    char little[STRLAB_SSO_SIZE];
    uint8_t dynamic;
};

/**
 * @brief Type to define an SSO string
 * @code{.c}
 *strlab_string str;
 * @endcode
 */
typedef struct strlab_string_struct strlab_string[1];

/**
 * @brief Type indicating the pointer to a string
 * @code{.c}
 *strlab_string str;
 *strlab_pointer ptr = str;
 * @endcode
 */
typedef struct strlab_string_struct *strlab_pointer;

/**
 * @brief Type indicating pointer to a constant string
 * @code{.c}
 *const strlab_string str = strlab_const("abc");
 *strlab_const_pointer ptr = str;
 * @endcode
 */
typedef const struct strlab_string_struct *strlab_const_pointer;

/**
 * @brief Function to initialize a string
 * @param str String to initialize
 * @code{.c}
 *strlab_string str;
 * strlab_create(str);
 *strlab_close(str) //close after use
 * @endcode
 */
void strlab_create(strlab_pointer str);

/**
 * @brief Function to free the memory of a string
 * @param str Initialized string
 * @return Return code
 * @code{.c}
 *strlab_string str;
 * strlab_create(str); // create before close
 * strlab_close(str);
 * @endcode
 */
int strlab_close(strlab_pointer str);

/**
 * @brief Function to initialize a string with a static buffer
 * @note The buffer must be a terminated string
 * @param str String to initialize
 * @param buf Buffer to use for the string
 * @param size Maximum buffer size
 * @code{.c}
 * char buf[1024] = "\0";
 *strlab_string str;
 * strlab_fixed(str, buf, sizeof(buf)); // do not close
 * @endcode
 */
void strlab_fixed(strlab_pointer str, char *buf, size_t size);

/**
 * @brief Function to debug a string
 * @note Print all non-visible characters as escape characters
 * @param str String to debug
 * @code{.c}
 * //str = "abc\n\r"
 * strlab_log(str);
 * //output -> 'abc\n\r' len=3 size=16 SSO=True dynamic=True
 * @endcode
 */
void strlab_log(strlab_const_pointer str);

/**
 * @brief Function to delete all characters of a string
 * @param str String to clean up
 * @code{.c}
 * //str = "abc"
 * strlab_clear(str);
 * //str = ""
 * @endcode 
 */
void strlab_clear(strlab_pointer str);

/**
 * @brief Function to know if a string contains no characters
 * @param str String to parse
 * @return 1 if the string contains no characters otherwise 0
 * @code{.c}
 * //str = "abc"
 * strlab_isempty(str); // False
 * //str = ""
 * strlab_isempty(str); // True
 * @endcode
 */
int strlab_isempty(strlab_const_pointer str);

/**
 * @brief Function to know the length of a string
 * @param str String to parse
 * @return Length of the string
 * @code{.c}
 * //str = "abc"
 * size_t len = strlab_length(str);
 * //len = 3
 * @endcode
 */
size_t strlab_length(strlab_const_pointer str);

/**
 * @brief Function to read string as C string
 * @param str String to read
 * @return Characters of the string
 * @code{.c}
 * //str = "abc"
 * printf("%s\n", strlab_cstring(str));
 * //output -> abc
 * @endcode
 */
const char *strlab_cstring(strlab_const_pointer str);

/**
 * @brief Function to ensure if one string is greater than another
 * @param str1 First string to match
 * @param str2 Second string to compare
 * @return 1 if the first string is larger than the second otherwise 0
 * @code{.c}
 * //str1 = "ab"
 * //str2 = "bc"
 * strlab_greater(str1, str2); // True
 * @endcode
 */
int strlab_greater(strlab_const_pointer str1, strlab_const_pointer str2);

/**
 * @brief Function to ensure whether one string is equal to another
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @return 1 if the first string is equal to the second otherwise 0
 * @code{.c}
 * //str1 = "ab"
 * //str2 = "ab"
 * strlab_equals(str1, str2); // True
 * @endcode
 */
int strlab_equals(strlab_const_pointer str1, strlab_const_pointer str2);

/**
 * @brief Function to ensure if one string is less than another
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @return 1 if the first string is smaller than the second, otherwise 0
 * @code{.c}
 * //str1 = "ab"
 * //str2 = "bc"
 * strlab_less(str1, str2); // False
 * @endcode
 */
int strlab_less(strlab_const_pointer str1, strlab_const_pointer str2);

/**
 * @brief Function to compare two strings
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @return Comparison number
 * @code{.c}
 * //str1 = "ab"
 * //str2 = "bc"
 * strlab_compare(str1, str2); // > 0
 * strlab_compare(str2, str1); // < 0
 * strlab_compare(str1, str1); // == 0
 * @endcode
 */
int strlab_compare(strlab_const_pointer str1, strlab_const_pointer str2);

/**
 * @brief Function to copy one string into another
 * @param str Destination string
 * @param other String to copy
 * @return Return code
 * @code{.c}
 * //str = "abc", src = "def"
 * strlab_copy(str, src);
 * //str = "def"
 * @endcode
 */
int strlab_copy(strlab_pointer str, strlab_const_pointer other);

/**
 * @brief Function to insert one string into another
 * @param str Destination string
 * @param index Insertion position
 * @param other String to copy
 * @return Return code
 * @code{.c}
 * //str = "abef", src = "cd"
 * strlab_insert(str, 2, src);
 * //str = "abcdef"
 * @endcode
 */
int strlab_insert(strlab_pointer str, size_t index, strlab_const_pointer other);

/**
 * @brief Function to append one string to another
 * @param str Destination string
 * @param other String to copy
 * @return Return code
 * @code{.c}
 * //str = "abc", src = "def"
 * strlab_append(str, src);
 * //str = "abcdef"
 * @endcode
 */
int strlab_append(strlab_pointer str, strlab_const_pointer other);

/**
 * @brief Function to delete characters from a string
 * @param str String to delete
 * @param index Position of the start of the deletion
 * @param length Number of characters to delete
 * @return Return code
 * @code{.c}
 * //str = "abcdef"
 * strlab_erase(str, 3, 3);
 * //str = "abc"
 * @endcode
 */
int strlab_erase(strlab_pointer str, size_t index, size_t length);

/**
 * @brief Function to top a character in a string
 * @param str Destination string
 * @param chr Ascii character to insert
 * @return Return code
 * @code{.c}
 * //str = "ab"
 * strlab_putchar(str, 'c');
 * //str = "abc"
 * @endcode
 */
int strlab_putchar(strlab_pointer str, int chr);

/**
 * @brief Function to cut a string
 * @param str String to trim
 * @param index Cut start position
 * @param length Number of characters to keep in the new string
 * @return Return code
 * @code{.c}
 * //str = "__abc__"
 * strlab_substr(str, 2, 3);
 * //str = "abc"
 * @endcode
 */
int strlab_substr(strlab_pointer str, size_t index, size_t length);

/**
 * @brief Function for replacing a substring with another substring in a string
 * @param str String to modify
 * @param old Substring to remove
 * @param new Substring to insert
 * @return Return code
 * @code{.c}
 * //str = "__def__def__", old = "def", new = "abc"
 * strlab_repalce(str, old, new);
 * //str = "__abc__abc__"
 * @endcode
 */
int strlab_repalce(strlab_pointer str, strlab_const_pointer old, strlab_const_pointer new);

/**
 * @brief Function to replace the first substring found in a string with another substring
 * @param str String to modify
 * @param old Substring to remove
 * @param new Substring to insert
 * @return Return code
 * @code{.c}
 * //str = "__def__def__", old = "def", new = "abc"
 * strlab_rchange(str, old, new);
 * //str = "__abc__def__"
 * @endcode
 */
int strlab_lchange(strlab_pointer str, strlab_const_pointer old, strlab_const_pointer new);

/**
 * @brief Function to replace the last substring found in a string with another substring
 * @param str String to modify
 * @param old Substring to remove
 * @param new Substring to insert
 * @return Return code
 * @code{.c}
 * //str = "__def__def__", old = "def", new = "abc"
 * strlab_rchange(str, old, new);
 * //str = "__def__abc__"
 * @endcode
 */
int strlab_rchange(strlab_pointer str, strlab_const_pointer old, strlab_const_pointer new);

/**
 * @brief Function to ensure manually enlarging the buffer of a string
 * @param str String to modify
 * @param size Size of the enlarged buffer
 * @return Return code
 * @code{.c}
 * //str = "", src1 = "abc", src2 = "def"
 * size_t total = strlab_length(src1) + strlab_length(src2) + 1;
 * strlab_ensure(str, total);
 * strlab_append(str, src1);
 * strlab_append(str, src2);
 * //str = "abcdef"
 * @endcode
 */
int strlab_ensure(strlab_pointer str, size_t size);

/**
 * @brief Function to search for a substring in a string
 * @param str String to parse
 * @param sub Substring to search for
 * @return Start position of the substring, @ref STRLAB_NPOS if not found
 * @code{.c}
 * //str = "abcabcabc", sub = "cab"
 * size_t found = strlab_search(str, sub);
 *if(found == STRLAB_NPOS) //Not found
 * //found = 2
 * @endcode
 */
size_t strlab_search(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Function to find the first substring that appears in a string
 * @param str String to parse
 * @param sub Substring to search for
 * @return Pointer to the string data where the substring was found, NULL if not found
 * @code{.c}
 * //str = "abcabcabc", sub = "cab"
 * const char *found = strlab_lfind(str, sub);
 *if(!found) //Not found
 * size_t index = found - strlab_cstring(str);
 * //index = 2
 * @endcode
 */
const char *strlab_lfind(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Function to find the last substring that appears in a string
 * @param str String to parse
 * @param sub Substring to search for
 * @return Pointer to the string data where the substring was found, NULL if not found
 * @code{.c}
 * //str = "abcabcabc", sub = "cab"
 * const char *found = strlab_lfind(str, sub);
 *if(!found) //Not found
 * size_t index = found - strlab_cstring(str);
 * //index = 5
 * @endcode
 */
const char *strlab_rfind(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Function to get a character from a string
 * @param str String to parse
 * @param index Position of the character to fetch
 * @return Character found, EOF on error
 * @code{.c}
 * //str = "abc"
 * printf("%c\n", strlab_charat(str, 1));
 * //output -> b
 * @endcode
 */
int strlab_charat(strlab_const_pointer str, size_t index);

/**
 * @brief Function to receive the pointer of a character of a string
 * @param str String to parse
 * @param index Position of the character to fetch
 * @return Pointer to the character, NULL on error
 * //str = "aXc"
 * char *c = strlab_index(str, 1);
 **c = 'b';
 * //str = "abc"
 */
char *strlab_index(strlab_pointer str, size_t index);

/**
 * @brief Function to remove characters at the ends of a string
 * @param str String to modify
 * @param set Set of characters to remove, if NULL clears spaces
 * @return Return code
 * @code{.c}
 * //str1 = "__abc__", str2 = "abc", del = "_"
 * strlab_strip(str1, del);
 * strlab_strip(str2, NULL);
 * //str1 = abc, str2 = abc
 * @endcode
 */
int strlab_strip(strlab_pointer str, strlab_const_pointer set);

/**
 * @brief Function to transform the characters of a string into uppercase
 * @param str String to modify
 * @code{.c}
 * //str = "abc"
 * strlab_capitalize(str);
 * //str = "ABC"
 * @endcode
 */
void strlab_capitalize(strlab_pointer str);

/**
 * @brief Function to transform the characters of a string to lowercase
 * @param str String to modify
 * @code{.c}
 * //str = "ABC"
 * strlab_lowercase(str);
 * //str = "abc"
 * @endcode
 */
void strlab_lowercase(strlab_pointer str);

/**
 * @brief Function to perform a foreach on a string
 * @param str String to modify
 * @param callback
 *Function receiving: character pointer, character position, @ref args.
 *Stop the loop if it returns a value other than 0
 * @param args Arguments to pass to the @ref callback function
 * @return Value returned by the function if different from 0 otherwise @ref STRLAB_SUCCESS
 * @code{.c}
 * int print_and_count_to_10(char *c, size_t index, void *args) {
 *int *count = args;
 *     printf("str[%zu] = '%c'", index, *c);
 *     (*count)++;
 *     if(*count < 10) return 0;
 *     else return -1;
 *}
 * 
 * //str = "abc"
 *int count = 0;
 * strlab_foreach(str, print_and_count_to_10, &count);
 * //count = 3
 * //output -> str[0] = 'a'
 *str[1] = 'b'
 *str[2] = 'c
 * @endcode
 */
int strlab_foreach(strlab_pointer str, int (*callback)(char*, size_t, void*), void *args);

/**
 * @brief Function to copy the result of the printf function into a string
 * @param str Destination string
 * @param format Format to pass to the printf function
 * @param ... Arguments to pass to the printf function
 * @return Value returned by printf
 * @code{.c}
 * //str = "abc",
 * strlab_printf(str, "num%d", 123);
 * //str = "num123"
 * @endcode
 */
int strlab_printf(strlab_pointer str, const char *format, ...);

/**
 * @brief Function to scan a string using the scanf function
 * @param str String to parse
 * @param format Format to pass to the scanf function
 * @param ... Arguments to pass to the scanf function
 * @return Value returned by scanf
 * @code{.c}
 * //str = "123abc",
 * int n = 0;
 * char buf[4] = "\0";
 * strlab_scanf(str, "%d%s", &n, buf);
 * //n = 123, buf = "abc"
 * @endcode
 */
int strlab_scanf(strlab_const_pointer str, const char *format, ...);

/**
 * @brief Function to receive the string buffer and clear the external structure
 * @param str String to delete
 * @return Dynamic character pointer of the string
 * @code{.c}
 * //str = "abc",
 * char *buf = strlab_relase(str);
 * buf[0] = A;
 * printf("%s\n", buf);
 * STRLAB_FREE(buf);
 * //output -> Abc
 * @endcode
 */
char *strlab_relase(strlab_pointer str);

/**
 * @brief Function to count how many times a substring appears in a string
 * @param str String to parse
 * @param sub Substring to search for
 * @return Number of times @ref sub appears
 * @code{.c}
 * //str = "abcabcabc", sub = "abc"
 * size_t count = strlab_count(str, sub);
 * //count = 3
 * @endcode
 */
size_t strlab_count(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Function to check if a string begins with a substring
 * @param str String to parse
 * @param sub Substring to search for
 * @return 1 if @ref str begins with @ref sub otherwise 0
 * @code{.c}
 * //str = "abcdef", sub1 = "abc", sub2 = "bcd"
 * strlab_startswith(str, sub1) // True
 * strlab_startswith(str, sub2) // False
 * @endcode
 */
int strlab_startswith(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Function to check if a string ends in a substring
 * @param str String to parse
 * @param sub Substring to search for
 * @return 1 if @ref str ends with @ref sub otherwise 0
 * @code{.c}
 * //str = "abcdef", sub1 = "def", sub2 = "cde"
 * strlab_startswith(str, sub1) // True
 * strlab_startswith(str, sub2) // False
 * @endcode
 */
int strlab_endswith(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Function for reading a line from a file
 * @param str Destination string
 * @param src Readable file
 * @return Return code
 * @code{.c}
 * //src = FILE(abc\ndef\n)
 * //str = "!!!"
 * strlab_fgetln(str, src);
 * //str = "abc\n"
 * @endcode
 */
int strlab_fgetln(strlab_pointer str, FILE *src);

/**
 * @brief Function for reading bytes from a file
 * @param str Destination string
 * @param src Readable file
 * @param size Number of bytes to read
 * @return Return code
 * @code{.c}
 * //src = FILE(abc\ndef\n)
 * //str = "!!!"
 * strlab_fread(str, src, 5);
 * //str = "abc\nd"
 * @endcode
 */
int strlab_fread(strlab_pointer str, FILE *src, size_t size);

/**
 * @brief Function to insert a string into a file
 * @param dest Destination file
 * @param str String to insert
 * @return Return code
 * @code{.c}
 * //dst = FILE(abc\n)
 * //str = "def\n"
 * strlab_fwrite(dst, str)
 * //dst = FILE(abc\ndef\n)
 * @endcode
 */
int strlab_fwrite(FILE *dest, strlab_const_pointer str);

/**
 * @brief Function to trim a string into a list of substrings
 * @param str String to trim
 * @param del Substring indicating the separation of substrings in the string @ref str
 * @return Dynamic list of strings terminated with the string @ref STRLAB_LFIN
 * @code{.c}
 * //str = "a, b, c, d", del = ", "
 *strlab_string *list = strlab_split(str, del);
 * //list = {"a", "b", "c", "d", STRLAB_LFIN}
 *strlab_unlist(list) //free the list
 * @endcode
 */
strlab_string *strlab_split(strlab_const_pointer str, strlab_const_pointer del);

/**
 * @brief Function to insert into a string the strings of a list delimited by the characters that contains @ref str
 * @param str Destination string
 * @param list List of strings
 * @return Return code
 * @code{.c}
 * //str = ", ", list = {"a", "b", "c", "d", STRLAB_LFIN}
 * strlab_join(str, list);
 * //str = "a, b, c, d"
 * @endcode
 */
int strlab_join(strlab_pointer str, const strlab_string *list);

/**
 * @brief Function to free a dynamic list of strings
 * @param list Dynamic list of strings
 * @return Return code
 * @code{.c}
 * //list = {"ab", "cd", STRLAB_LFIN} (list is allocated)
 * strlab_unlist(list); 
 * @endcode
 */
int strlab_unlist(strlab_string *list);

#endif