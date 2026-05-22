/**
 * @file strlab.c
 * @brief Libreria per stringhe con sttimizzazione SSO
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
 * @brief Macro personalizzabile per allocare memoria
 * @param siz Grandezza in byte da allocare
 * @return Puntatore a memoria allocata
 * @code{.c}
 * #define STRLAB_MALLOC(siz) my_allocator(siz)
 * @endcode
 */
#define STRLAB_MALLOC(siz) malloc(siz)
#endif

#ifndef STRLAB_FREE
/**
 * @brief Macro per liberare memoria allocata
 * @param ptr Puntatore della memoria da deallocare
 * @code{.c}
 * #define STRLAB_FREE(ptr) my_free(ptr)
 * @endcode
 */
#define STRLAB_FREE(ptr) free(ptr)
#endif

#ifndef STRLAB_SSO_SIZE
/**
 * @brief Grandezza della stringa di SSO
 * @code{.c}
 * #define STRLAB_SSO_SIZE 32
 * @endcode
 */
#define STRLAB_SSO_SIZE 16
#endif

#ifndef STRLAB_NPOS
/**
 * @brief Valore che indica un indice non valido
 * @code{.c}
 * // str = "abc", sub = "123"
 * size_t index = strlab_search(str, sub);
 * // index = STRLAB_NPOS
 * @endcode
 */
#define STRLAB_NPOS ((size_t)(-1))
#endif

#ifndef STRLAB_LFIN
/**
 * @brief Stringa che indica la fine di una lista di stringhe
 * @code{.c}
 * // list = {"a", "b", "c", STRLAB_LFIN}
 * for(size_t n = 0; !strlab_equals(list[n], STRLAB_LFIN); n++) {
 *     printf("%s", strlab_cstring(list[n]));
 * }
 * // output -> abc
 * @endcode
 */
#define STRLAB_LFIN (const strlab_string){{1, 2, (char[2]){(char)(0xFF), '\0'}, {0}, 0}}
#endif

/**
 * @brief Crea una stringa costante temporanea utilizzando come risorsa una stringa costante C
 * @code{.c}
 * strlab_copy(str, strlab_expr("abc"));
 * // str = "abc"
 * @endcode
 */
#define strlab_expr(expr) ((const strlab_string){{sizeof((expr)) - 1, sizeof((expr)), (expr), {0}, 0}})

/**
 * @brief Inizializza una stringa costante utilizzando come risorsa una stringa costante C
 * @code{.c}
 * const strlab_string cstr = strlab_const("abc");
 * // cstr = "abc"
 * @endcode
 */
#define strlab_const(expr) {{sizeof((expr)) - 1, sizeof((expr)), (expr), {0}, 0}}

enum {
    STRLAB_SUCCESS = 0, /** Valore che indica il successo di una funzione, e' sempre 0 */
    STRLAB_MAX_SIZE_REACHED, /**< Valore che indica il raggiungimento massimo di una grandezza di una stringa non dinamica */
    STRLAB_OUT_OF_MEMORY, /**< Valore che indica che la memoria allocabile e' finita */
    STRLAB_OUT_OF_RANGE, /**< Valore che indica che e' stato applicato un offset troppo grande */
    STRLAB_INVALID_CHAR, /**< Valore che indica l'inserimento di un carattere errato in una stringa */
    STRLAB_FREE_ON_FIXED, /**< Valore che indica la tentata liberazione della memoria di una stringa non dinamica */
    STRLAB_IO_ERROR, /**< Valore che indica un fallimento da parte delle funzioni di IO */
    STRLAB_NOT_FOUND /**< Valore che indica che una ricerca di un elemento non e' andata a buon fine */
};

/**
 * @brief Struttura per una stringa SSO
 * @note E' preferibile non utilizzarla
 */
struct strlab_string_struct {
    size_t len;
    size_t size;
    char *buf;
    char little[STRLAB_SSO_SIZE];
    uint8_t dynamic;
};

/**
 * @brief Tipo per definire una stringa SSO
 * @code{.c}
 * strlab_string str;
 * @endcode
 */
typedef struct strlab_string_struct strlab_string[1];

/**
 * @brief Tipo che indica il puntatore a una stringa
 * @code{.c}
 * strlab_string str;
 * strlab_pointer ptr = str;
 * @endcode
 */
typedef struct strlab_string_struct *strlab_pointer;

/**
 * @brief Tipo che indica il puntatore a una stringa costante
 * @code{.c}
 * const strlab_string str = strlab_const("abc");
 * strlab_const_pointer ptr = str;
 * @endcode
 */
typedef const struct strlab_string_struct *strlab_const_pointer;

/**
 * @brief Funzione per inizializzare una stringa
 * @param str Stringa da inizializzare
 * @code{.c}
 * strlab_string str;
 * strlab_create(str);
 * strlab_close(str) // close after use
 * @endcode
 */
void strlab_create(strlab_pointer str);

/**
 * @brief Funzione per liberare la memoria di una stringa
 * @param str Stringa inizializzata
 * @return Codice di ritorno
 * @code{.c}
 * strlab_string str;
 * strlab_create(str); // create before close
 * strlab_close(str);
 * @endcode
 */
int strlab_close(strlab_pointer str);

/**
 * @brief Funzione per inizializzare una stringa con un buffer statico
 * @note Il buffer deve essere una stringa terminata
 * @param str Stringa da inizializzare
 * @param buf Buffer da far utilizzare alla stringa
 * @param size Grandezza massima del buffer
 * @code{.c}
 * char buf[1024] = "\0";
 * strlab_string str;
 * strlab_fixed(str, buf, sizeof(buf)); // do not close
 * @endcode
 */
void strlab_fixed(strlab_pointer str, char *buf, size_t size);

/**
 * @brief Funzione per eseguire del debug di una stringa
 * @param str Stringa da debuggare
 * @code{.c}
 * // str = "abc"
 * strlab_log(str);
 * // output -> 'abc' len=3 size=16 SSO=True dynamic=True
 * @endcode
 */
void strlab_log(strlab_const_pointer str);

/**
 * @brief Funzione per cancellare tutti i caratteri di una stringa
 * @param str Stringa da ripulire
 * @code{.c}
 * // str = "abc"
 * strlab_clear(str);
 * // str = ""
 * @endcode 
 */
void strlab_clear(strlab_pointer str);

/**
 * @brief Funzione per sapere se una stringa non contiene caratteri
 * @param str Stringa da analizzare
 * @return 1 se la stringa non contiene caratteri altrimenti 0
 * @code{.c}
 * // str = "abc"
 * strlab_isempty(str); // False
 * // str = ""
 * strlab_isempty(str); // True
 * @endcode
 */
int strlab_isempty(strlab_const_pointer str);

/**
 * @brief Funzione per conoscere la lunghezza di una stringa
 * @param str Stringa da analizzare
 * @return Lunghezza della stringa
 * @code{.c}
 * // str = "abc"
 * size_t len = strlab_length(str);
 * // len = 3
 * @endcode
 */
size_t strlab_length(strlab_const_pointer str);

/**
 * @brief Funzione per leggere la stringa come stringa C
 * @param str Stringa da leggere
 * @return Caratteri della stringa
 * @code{.c}
 * // str = "abc"
 * printf("%s\n", strlab_cstring(str));
 * // output -> abc
 * @endcode
 */
const char *strlab_cstring(strlab_const_pointer str);

/**
 * @brief Funzione per assicurare se una stringa e' maggiore di un altra
 * @param str1 Prima stringa da conforntare
 * @param str2 Seconda stringa da confrontare
 * @return 1 se la prima stringa e' piu' grande della seconda altrimenti 0
 * @code{.c}
 * // str1 = "ab"
 * // str2 = "bc"
 * strlab_greater(str1, str2); // True
 * @endcode
 */
int strlab_greater(strlab_const_pointer str1, strlab_const_pointer str2);

/**
 * @brief Funzione per assicurare se una stringa e' uquale ad un altra
 * @param str1 Prima stringa da confrontare
 * @param str2 Seconda stringa da confrontare
 * @return 1 se la prima stringa e' uguale alla seconda altrimenti 0
 * @code{.c}
 * // str1 = "ab"
 * // str2 = "ab"
 * strlab_equals(str1, str2); // True
 * @endcode
 */
int strlab_equals(strlab_const_pointer str1, strlab_const_pointer str2);

/**
 * @brief Funzione per assicurare se una stringa e' minore ad un altra
 * @param str1 Prima stringa da confrontare
 * @param str2 Seconda stringa da confrontare
 * @return 1 se la prima stringa e' piu' piccola della seconda altrmienti 0
 * @code{.c}
 * // str1 = "ab"
 * // str2 = "bc"
 * strlab_less(str1, str2); // False
 * @endcode
 */
int strlab_less(strlab_const_pointer str1, strlab_const_pointer str2);

/**
 * @brief Funzione per confrontare due stringhe
 * @param str1 Prima stringa da confrontare
 * @param str2 Seconda stringa da confrontare
 * @return Numero di confronto
 * @code{.c}
 * // str1 = "ab"
 * // str2 = "bc"
 * strlab_compare(str1, str2); // > 0
 * strlab_compare(str2, str1); // < 0
 * strlab_compare(str1, str1); // == 0
 * @endcode
 */
int strlab_compare(strlab_const_pointer str1, strlab_const_pointer str2);

/**
 * @brief Funzione per copiare una stringa in una altra
 * @param str Stringa di destinazione
 * @param other Stringa da copiare
 * @return Codice di ritorno
 * @code{.c}
 * // str = "abc", src = "def"
 * strlab_copy(str, src);
 * // str = "def"
 * @endcode
 */
int strlab_copy(strlab_pointer str, strlab_const_pointer other);

/**
 * @brief Funzione per inserire una stringa in un altra
 * @param str Stringa di destinazione
 * @param index Posizione di inserimento
 * @param other Stringa da copiare
 * @return Codice di ritorno
 * @code{.c}
 * // str = "abef", src = "cd"
 * strlab_insert(str, 2, src);
 * // str = "abcdef"
 * @endcode
 */
int strlab_insert(strlab_pointer str, size_t index, strlab_const_pointer other);

/**
 * @brief Funzione per appendere in una stringa una altra
 * @param str Stringa di destinazione
 * @param other Stringa da copiare
 * @return Codice di ritorno
 * @code{.c}
 * // str = "abc", src = "def"
 * strlab_append(str, src);
 * // str = "abcdef"
 * @endcode
 */
int strlab_append(strlab_pointer str, strlab_const_pointer other);

/**
 * @brief Funzione per cancellare dei caratteri da una stringa
 * @param str Stringa da cancellare
 * @param index Posizione dell'inizio della cancellazione
 * @param length Numero di caratteri da cancellare
 * @return Codice di ritorno
 * @code{.c}
 * // str = "abcdef"
 * strlab_erase(str, 3, 3);
 * // str = "abc"
 * @endcode
 */
int strlab_erase(strlab_pointer str, size_t index, size_t length);

/**
 * @brief Funzione per inserire in cima un carattera in una stringa
 * @param str Stringa di destinazione
 * @param chr Carattere ascii da inserire
 * @return Codice di ritorno
 * @code{.c}
 * // str = "ab"
 * strlab_putchar(str, 'c');
 * // str = "abc"
 * @endcode
 */
int strlab_putchar(strlab_pointer str, int chr);

/**
 * @brief Funzione per tagliare una stringa
 * @param str Stringa da tagliare
 * @param index Posizione di inizio del taglio
 * @param length Numero di caratteri da mantenere nella nuova stringa
 * @return Codice di ritorno
 * @code{.c}
 * // str = "__abc__"
 * strlab_substr(str, 2, 3);
 * // str = "abc"
 * @endcode
 */
int strlab_substr(strlab_pointer str, size_t index, size_t length);

/**
 * @brief Funzione per rimpiazzare una sottostringa con un altra sottostringa in una stringa
 * @param str Stringa da modificare
 * @param old Sottostringa da rimuovere
 * @param new Sottostringa da inserire
 * @return Codice di ritorno
 * @code{.c}
 * // str = "__def__def__", old = "def", new = "abc"
 * strlab_repalce(str, old, new);
 * // str = "__abc__abc__"
 * @endcode
 */
int strlab_repalce(strlab_pointer str, strlab_const_pointer old, strlab_const_pointer new);

/**
 * @brief Funzione per rimpiazzare la prima sottostringa trovata in una stringa con un altra sottostringa
 * @param str Stringa da modificare
 * @param old Sottostringa da rimuovere
 * @param new Sottostringa da inserire
 * @return Codice di ritorno
 * @code{.c}
 * // str = "__def__def__", old = "def", new = "abc"
 * strlab_rchange(str, old, new);
 * // str = "__abc__def__"
 * @endcode
 */
int strlab_lchange(strlab_pointer str, strlab_const_pointer old, strlab_const_pointer new);

/**
 * @brief Funzione per rimpiazzare l'ultima sottostringa trovata in una stringa con un altra sottostringa
 * @param str Stringa da modificare
 * @param old Sottostringa da rimovere
 * @param new Sottostringa da inserire
 * @return Codice di ritorno
 * @code{.c}
 * // str = "__def__def__", old = "def", new = "abc"
 * strlab_rchange(str, old, new);
 * // str = "__def__abc__"
 * @endcode
 */
int strlab_rchange(strlab_pointer str, strlab_const_pointer old, strlab_const_pointer new);

/**
 * @brief Funzione per assicurare ingrandire manualmente il buffer di una stringa
 * @param str Stringa da modificare
 * @param size Grandezza del buffer ingrandito
 * @return Codice di ritorno
 * @code{.c}
 * // str = "", src1 = "abc", src2 = "def"
 * size_t total = strlab_length(src1) + strlab_length(src2) + 1;
 * strlab_ensure(str, total);
 * strlab_append(str, src1);
 * strlab_append(str, src2);
 * // str = "abcdef"
 * @endcode
 */
int strlab_ensure(strlab_pointer str, size_t size);

/**
 * @brief Funzione per cercare una sottostringa in una stringa
 * @param str Stringa da analizzare
 * @param sub Sottostringa da cercare
 * @return Posizione di inizio della sottostringa, @ref STRLAB_NPOS se non la trova
 * @code{.c}
 * // str = "abcabcabc", sub = "cab"
 * size_t found = strlab_search(str, sub);
 * if(found == STRLAB_NPOS) // Not found
 * // found = 2
 * @endcode
 */
size_t strlab_search(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Funzione per trovare la prima sottostringa che compare in una stringa
 * @param str Stringa da analizzare
 * @param sub Sottostringa da cercare
 * @return Puntatore ai dati della stringa dove e' stata trovata la sottostringa, NULL se non la trova
 * @code{.c}
 * // str = "abcabcabc", sub = "cab"
 * const char *found = strlab_lfind(str, sub);
 * if(!found) // Not found
 * size_t index = found - strlab_cstring(str);
 * // index = 2
 * @endcode
 */
const char *strlab_lfind(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Funzione per trovare l'ultima sottostringa che compare in una stringa
 * @param str Stringa da analizzare
 * @param sub Sottostringa da cercare
 * @return Puntatore ai dati della stringa dove e' stata trovata la sottostringa, NULL se non la trova
 * @code{.c}
 * // str = "abcabcabc", sub = "cab"
 * const char *found = strlab_lfind(str, sub);
 * if(!found) // Not found
 * size_t index = found - strlab_cstring(str);
 * // index = 5
 * @endcode
 */
const char *strlab_rfind(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Funzione per prelevare un carattere da una stringa
 * @param str Stringa da analizzare
 * @param index Posizione del carattere da prelevare
 * @return Carattere trovato, EOF in caso di errore
 * @code{.c}
 * // str = "abc"
 * printf("%c\n", strlab_charat(str, 1));
 * // output -> b
 * @endcode
 */
int strlab_charat(strlab_const_pointer str, size_t index);

/**
 * @brief Funzione per ricevere il puntatore di un carattere di una stringa
 * @param str Stringa da analizzare
 * @param index Posizione del carattere da prelevare
 * @return Puntatore al carattere, NULL in caso di errore
 * // str = "aXc"
 * char *c = strlab_index(str, 1);
 * *c = 'b';
 * // str = "abc"
 */
char *strlab_index(strlab_pointer str, size_t index);

/**
 * @brief Funzione per rimuovere dei caratteri agli estremi di una stringa
 * @param str Stringa da modificare
 * @param set Set di caratteri da rimuovere, se NULL cancella gli spazi
 * @return Codice di ritorno
 * @code{.c}
 * // str1 = "__abc__", str2 = "  abc  ", del = "_"
 * strlab_strip(str1, del);
 * strlab_strip(str2, NULL);
 * // str1 = abc, str2 = abc
 * @endcode
 */
int strlab_strip(strlab_pointer str, strlab_const_pointer set);

/**
 * @brief Funzione per trasformare i caratteri di una stringa in maiuscolo
 * @param str Stringa da modificare
 * @code{.c}
 * // str = "abc"
 * strlab_capitalize(str);
 * // str = "ABC"
 * @endcode
 */
void strlab_capitalize(strlab_pointer str);

/**
 * @brief Funzione per trasformare i caratteri di una stringa in minuscolo
 * @param str Stringa da modificare
 * @code{.c}
 * // str = "ABC"
 * strlab_lowercase(str);
 * // str = "abc"
 * @endcode
 */
void strlab_lowercase(strlab_pointer str);

/**
 * @brief Funzione per eseguire un foreach su una stringa
 * @param str Stringa da modificare
 * @param callback
 * Funzione che riceve: puntatore al carattere, posizione del carattere, @ref args.
 * Ferma il ciclo se ritorna un valore diverso da 0
 * @param args Argomenti da passare alla funzione @ref callback
 * @return Valore ritornato dalla funzione se diverso da 0 altrimenti @ref STRLAB_SUCCESS
 * @code{.c}
 * int print_and_count_to_10(char *c, size_t index, void *args) {
 *     int *count = args;
 *     printf("str[%zu] = '%c'", index, *c);
 *     (*count)++;
 *     if(*count < 10) return 0;
 *     else return -1;
 * }
 * 
 * // str = "abc"
 * int count = 0;
 * strlab_foreach(str, print_and_count_to_10, &count);
 * // count = 3
 * // output -> str[0] = 'a'
 *              str[1] = 'b'
 *              str[2] = 'c
 * @endcode
 */
int strlab_foreach(strlab_pointer str, int (*callback)(char*, size_t, void*), void *args);

/**
 * @brief Funzione per copiare il risultato della funzione printf in una stringa
 * @param str Stringa di destinazione
 * @param format Formato da passare alla funzione printf
 * @param ... Argomenti da passare alla funzione printf
 * @return Valore ritornato da printf
 * @code{.c}
 * // str = "abc",
 * strlab_printf(str, "num%d", 123);
 * // str = "num123"
 * @endcode
 */
int strlab_printf(strlab_pointer str, const char *format, ...);

/**
 * @brief Funzione per scannerizzare una stringa utilizzando la funzione scanf
 * @param str Stringa da analizzare
 * @param format Formato da passare alla funzione scanf
 * @param ... Argomenti da passare alla funzione scanf
 * @return Valore ritornato da scanf
 * @code{.c}
 * // str = "123abc",
 * int n = 0;
 * char buf[4] = "\0";
 * strlab_scanf(str, "%d%s", &n, buf);
 * // n = 123, buf = "abc"
 * @endcode
 */
int strlab_scanf(strlab_const_pointer str, const char *format, ...);

/**
 * @brief Funzione per ricevere il buffer della stringa e cancellare la struttura estarna
 * @param str Stringa da cancellare
 * @return Puntatore dinamico di caratteri della stringa
 * @code{.c}
 * // str = "abc",
 * char *buf = strlab_relase(str);
 * buf[0] = A;
 * printf("%s\n", buf);
 * STRLAB_FREE(buf);
 * // output -> Abc
 * @endcode
 */
char *strlab_relase(strlab_pointer str);

/**
 * @brief Funzione per contare quante volta una sottostringa compare in una stringa
 * @param str Stringa da analizzare
 * @param sub Sottostringa da cercare
 * @return Numero di volte in cui compare @ref sub
 * @code{.c}
 * // str = "abcabcabc", sub = "abc"
 * size_t count = strlab_count(str, sub);
 * // count = 3
 * @endcode
 */
size_t strlab_count(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Funzione per verificare se una stringa comincia con una sottostringa
 * @param str Stringa da analizzare
 * @param sub Sottostringa da cercare
 * @return 1 se @ref str comincia con @ref sub altrimenti 0
 * @code{.c}
 * // str = "abcdef", sub1 = "abc", sub2 = "bcd"
 * strlab_startswith(str, sub1) // True
 * strlab_startswith(str, sub2) // False
 * @endcode
 */
int strlab_startswith(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Funzione per verificare se una stringa termina con una sottostringa
 * @param str Stringa da analizzare
 * @param sub Sottostringa da cercare
 * @return 1 se @ref str termina con @ref sub altrimenti 0
 * @code{.c}
 * // str = "abcdef", sub1 = "def", sub2 = "cde"
 * strlab_startswith(str, sub1) // True
 * strlab_startswith(str, sub2) // False
 * @endcode
 */
int strlab_endswith(strlab_const_pointer str, strlab_const_pointer sub);

/**
 * @brief Funzione per leggere una linea da un file
 * @param str Stringa di destinazione
 * @param src File leggibile
 * @return Codice di ritorno
 * @code{.c}
 * // src = FILE(abc\ndef\n)
 * // str = "!!!"
 * strlab_fgetln(str, src);
 * // str = "abc\n"
 * @endcode
 */
int strlab_fgetln(strlab_pointer str, FILE *src);

/**
 * @brief Funzione per leggere dei byte da un file
 * @param str Stringa di destinazione
 * @param src File leggibile
 * @param size Numero di byte da leggere
 * @return Codice di ritorno
 * @code{.c}
 * // src = FILE(abc\ndef\n)
 * // str = "!!!"
 * strlab_fread(str, src, 5);
 * // str = "abc\nd"
 * @endcode
 */
int strlab_fread(strlab_pointer str, FILE *src, size_t size);

/**
 * @brief Funzione per inserire una stringa in un file
 * @param dest File di destinazione
 * @param str Stringa da inserire
 * @return Codice di ritorno
 * @code{.c}
 * // dst = FILE(abc\n)
 * // str = "def\n"
 * strlab_fwrite(dst, str)
 * // dst = FILE(abc\ndef\n)
 * @endcode
 */
int strlab_fwrite(FILE *dest, strlab_const_pointer str);

/**
 * @brief Funzione per tagliare una stringa in una lista di sottosrighe
 * @param str Stringa da tagliare
 * @param del Sottostringa che indica la separazione delle sottostringhe nella stringa @ref str
 * @return Lista dinamica di stringhe terminata con la stringa @ref STRLAB_LFIN
 * @code{.c}
 * // str = "a, b, c, d", del = ", "
 * strlab_string *list = strlab_split(str, del);
 * // list = {"a", "b", "c", "d", STRLAB_LFIN}
 * strlab_unlist(list) // free the list
 * @endcode
 */
strlab_string *strlab_split(strlab_const_pointer str, strlab_const_pointer del);

/**
 * @brief Funzione per inserire in una stringa le stringhe di una lista delimitate dai caratteri che contiene @ref str
 * @param str Stringa di destinazione
 * @param list Lista di stringhe
 * @return Codice di ritorno
 * @code{.c}
 * // str = ", ", list = {"a", "b", "c", "d", STRLAB_LFIN}
 * strlab_join(str, list);
 * // str = "a, b, c, d"
 * @endcode
 */
int strlab_join(strlab_pointer str, const strlab_string *list);

/**
 * @brief Funzione per liberare una lista di stringhe dinamica
 * @param list Lista di stringhe dinamica
 * @return Codice di ritorno
 * @code{.c}
 * // list = {"ab", "cd", STRLAB_LFIN} (list is allocated)
 * strlab_unlist(list); 
 * @endcode
 */
int strlab_unlist(strlab_string *list);

#endif