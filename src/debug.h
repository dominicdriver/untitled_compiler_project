#ifndef DEBUG_H
#define DEBUG_H

#include "common.h"

#if defined(DEBUG)
#define debugf(fmt, ...) printf("[%s]: " fmt, __func__, __VA_ARGS__);
#else
#define debugf(...)
#endif

void print_token(token token);
void print_all_tokens(void);
void print_tokens(tk_node *list_ptr, size_t num_tokens);
void print_list_segment(tk_list_segment segment);
void print_replacement_tokens(token *replacement);
const macro *macro_exists_2(const char *identifier);

#endif //DEBUG_H
