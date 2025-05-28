#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include "common.h"

tk_node *advance_list(tk_node *list, size_t amount);
tk_node *insert_list_segment(tk_node *dest, tk_list_segment segment);

void insert_token_into_list(tk_node *list_ptr, token token, memory_arena *arena);
void remove_from_list(tk_node *list, const tk_node *start, const tk_node *end);
void save_tokens_to_file(const string *file_path, tk_node *start_node);

uint64_t hash(const string *str);

#endif // HELPER_FUNCTIONS_H
