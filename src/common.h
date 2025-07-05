#ifndef COMMON_H
#define COMMON_H

#include "enums.h"
#include "memory.h"
#include "strings.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_PARAMETERS 8
#define MAX_PARAMETER_LENGTH 32
#define MAX_NUM_MACROS 32768
#define MAX_NUM_FILES 512
#define MAX_TOKENS (1 << 20)

#define TOKEN_ARENA_MAX_SIZE sizeof(tk_node) * MAX_TOKENS

#define FILES_TOP files[files_top]

extern int operator_precedence[];

typedef struct {
    enum token_type type;
    enum subtype subtype;
    string src_filepath;
    uint16_t filename_index;
    string lexeme;
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
    string filepath;
    buff buffer;

    int current_pos;
    int current_line;
} file_info;

typedef struct {
    string name;
    uint64_t hash;
    short num_params;
    string parameters[MAX_PARAMETERS];
    bool is_function_like;
    token replacement[512];

    string defined_file;
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
extern size_t num_macros;

extern bool in_define;
extern bool in_include;

void error(const string *filename, int line, char *message);
void scan_and_insert_tokens(tk_node *insert_point);
void process_preprocessing_tokens(tk_node *token_node);
void expand_macro_tokens(tk_node *token_node);

const macro *macro_exists(tk_node *token_node);
tk_node *remove_tokens(const tk_node *start, const tk_node *end);
tk_list_segment expand_macro(tk_node *token_node);

bool add_file(const string *file_path);

#endif //COMMON_H
