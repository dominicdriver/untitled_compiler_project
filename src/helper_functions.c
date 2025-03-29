#include "helper_functions.h"
#include "common.h"
#include "enums.h"
#include "memory.h"

#include <stdint.h>
#include <stdio.h>

const char ESCAPED_CHAR_MAPPINGS[256] = {
    ['\''] = '\'',
    ['\"'] = '\"',
    ['\?'] = '\?',
    ['\\'] = '\\',
    ['\a'] = 'a',
    ['\b'] = 'b',
    ['\f'] = 'f',
    ['\n'] = 'n',
    ['\r'] = 'r',
    ['\t'] = 't',
    ['\v'] = 'v',

    // Escaped Octal Digital
    [0x80] = '0',
    [0x81] = '1',
    [0x82] = '2',
    [0x83] = '3',
    [0x84] = '4',
    [0x85] = '5',
    [0x86] = '6',
    [0x87] = '7',
};

tk_node *advance_list(tk_node *list, size_t amount) {
    for (size_t i = 0; i < amount; i++) list = list->next;
    return list;
}

// Returns the end of the segment, or dest if segment is NULL
tk_node *insert_list_segment(tk_node *dest, tk_list_segment segment) {
    if (segment.start == NULL) {
        return dest;
    }

    segment.end->next = dest->next;
    dest->next = segment.start;

    return segment.end;
}

void insert_token_into_list(tk_node *list_ptr, token token, memory_arena *arena) {
    tk_node *new_entry = allocate_from_arena(arena, sizeof(tk_node));

    new_entry->token = token;

    new_entry->next = list_ptr->next;
    list_ptr->next = new_entry;
}

// Removes tokens from start (inclusive) to end (exclusive)
void remove_from_list(tk_node *list, const tk_node *start, const tk_node *end) {
    tk_node *ptr = list;

    for (; ptr->next != start; ptr = ptr->next);

    tk_node *before_remove = ptr;

    for (; ptr != end; ptr = ptr->next);

    before_remove->next = ptr;
}

void save_tokens_to_file(const char *file_path, tk_node *start_node) {
    FILE *out_file = fopen(file_path, "w");

    for (tk_node *ptr = start_node; ptr != NULL; ptr = advance_list(ptr, 1)) {
        if (ptr->token.type == STRING_LITERAL) {
            fputc('\"', out_file);
        }

        if (ptr->token.subtype == CONST_CHAR) {
            fputc('\'', out_file);
        }

        if (ptr->token.type == DIRECTIVE) {
            fputc('#', out_file);
        }

        if (ptr->token.type != BLANK && ptr->token.type != NEWLINE) {
            for (size_t i = 0; ptr->token.lexeme[i]; i++) {
                if (ptr->token.lexeme[i] & 0x80) {
                    fputc('\\', out_file);
                    fputc(ESCAPED_CHAR_MAPPINGS[(uint8_t) ptr->token.lexeme[i] & 0x7F], out_file);
                } else {
                    fputc(ptr->token.lexeme[i], out_file);
                }
            }
        }

        if (ptr->token.type == STRING_LITERAL) {
            fputs("\"", out_file);
        }

        if (ptr->token.subtype == CONST_CHAR) {
            fputs("\' ", out_file);
        }

        if (ptr->next == NULL) continue;

        if (ptr->next->token.subtype != PUN_DOT && ptr->token.type != NEWLINE) {
            fputs(" ", out_file);
        }

        if (ptr->token.type == NEWLINE && (ptr->next->token.type != NEWLINE && ptr->next->token.type != END)) {
            fputs("\n", out_file);
        }
    }

    fclose(out_file);
}

uint64_t hash(const char *str) {
    uint64_t complete_hash = 0;

    while (*str) {
        uint64_t current_hash = 0;
        for (uint8_t i = 0; i < 64 && *str; i+=8) {
            current_hash |= ((uint64_t)(*str++) << i);
        }
        complete_hash ^= current_hash;
    }

    return complete_hash;
}
