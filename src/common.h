#ifndef COMMON_H
#define COMMON_H

#include "enums.h"
#include "memory.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_PARAMETERS 8
#define MAX_PARAMETER_LENGTH 32
#define MAX_NUM_MACROS 16384
#define MAX_NUM_FILES 512

#define TOKEN_ARENA_MAX_SIZE sizeof(tk_node) * (1 << 20)

#define FILES_TOP files[files_top]

extern int operator_precedence[];

typedef struct {
    enum token_type type;
    enum subtype subtype;
    char src_filepath[128]; // TODO: Fixed size for now
    ptrdiff_t filename_index;
    char lexeme[128]; // TODO: Fixed size for now, will break for long lexemes
    int32_t line;
    bool irreplaceable;
    bool follows_whitespace;
} token;

typedef struct tk_node {
    token token;
    struct tk_node *next;
} tk_node;

typedef struct {
    tk_node *start;
    tk_node *end;
    bool cond;
} ifgroup;

typedef struct {
    char *data;
    size_t size;
    size_t pos;
} buff;

typedef struct {
    char filepath[128];
    buff buffer;

    int current_pos;
    int current_line;
} file_info;

typedef struct {
    char name[256];
    uint64_t hash;
    short num_params;
    char parameters[MAX_PARAMETERS][MAX_PARAMETER_LENGTH];
    bool is_function_like;
    token replacement[256];

    char defined_file[128];
    int defined_line;
} macro;

typedef struct {
    tk_node *start;
    tk_node *end;
    size_t len;
} tk_list_segment;

// Defined in lexer:
extern memory_arena *token_arena;
extern tk_node *tokens;
extern size_t num_tokens;

extern file_info files[MAX_NUM_FILES];
extern int files_top;


// Defined in preprocessor:
extern macro macros[MAX_NUM_MACROS];
extern size_t num_macros;

extern bool in_define;
extern bool in_include;

void error(const char *filename, int line, char *message);
void scan_and_insert_tokens(tk_node *insert_point);
void process_preprocessing_tokens(tk_node *token_node);
void expand_macro_tokens(tk_node *token_node);

macro *macro_exists(tk_node *token_node);
tk_node *remove_tokens(const tk_node *start, const tk_node *end);
tk_list_segment expand_macro(tk_node *token_node);

bool add_file(const char *file_path);

#endif //COMMON_H
