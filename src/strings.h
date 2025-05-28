#ifndef STRINGS_H
#define STRINGS_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_LEXEME_LENGTH 512
#define MAX_FILEPATH_LENGTH 512

#define create_const_string(char_array) {.data = char_array, .cap = sizeof(char_array), .len = sizeof(char_array) - 1}

#define create_local_string(char_array, size) \
    ((string) {.data = (char[size]) {char_array}, .cap = size, .len = sizeof(char_array) - 1})

#define create_heap_string(size, arena) \
    ((string) {.data = allocate_from_arena(arena, size), .cap = size, .len = 0})

typedef struct {
    char *data;
    uint16_t cap; // Includes NULL byte
    uint16_t len; // Excludes NULL byte
} string;

bool string_cat(string *dest, const string *src);
bool string_cat_c(string *dest, char src);
bool string_copy(string *dest, const string *src);
int16_t string_cmp(const string *s1, const string *s2);
string string_rstr(const string *s, uint8_t c);
string string_slice(const string *s, uint16_t start, uint16_t end);

#endif // STRINGS_H
