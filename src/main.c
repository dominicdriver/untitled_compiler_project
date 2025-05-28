#include "common.h"
#include "debug.h"
#include "parser.h"
#include "helper_functions.h"
#include "strings.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern size_t max_macros;

static string predefined_string = create_const_string("PREDEFINED");

void add_predefined(void) {
    const char predefined[] = {
        "#define __x86_64__ 1\n"
        "#define __LP64__ 1\n"
    };

    files[++files_top] = (file_info) {.buffer = {.size = strlen(predefined), .pos = 0},
                                      .current_pos = 0, .current_line = 1};
    FILES_TOP.buffer.data = malloc(strlen(predefined) + 1);

    strcpy(FILES_TOP.buffer.data, predefined);
    FILES_TOP.filepath = predefined_string;
}

int main(void) {
    char *files_to_process[] = {
        ""
    };

    for (size_t i = 0; i < sizeof(files_to_process) / sizeof(files_to_process[0]); i++) {
        string filepath = create_local_string("", MAX_LEXEME_LENGTH);

        string_cat(&filepath, &files_to_process[i]);

        add_file(&filepath);
        add_predefined();

        token_arena = create_arena(TOKEN_ARENA_MAX_SIZE);

        tokens = allocate_from_arena(token_arena, sizeof(tk_node));
        tokens->token.lexeme = (string) {0};
        tokens->next = NULL;

        // Lexer
        scan_and_insert_tokens(tokens);

        // Preprocessor
        process_preprocessing_tokens(tokens);

        // Parser
        initialise_parser();
        create_ast_tree();


        string output_path = create_local_string("output/", MAX_FILEPATH_LENGTH);
        
        string filename = string_rstr(&files_to_process[i], '/');

        filename = string_slice(&filename, 1, filename.len);

        string_cat(&output_path, &filename);
output_path.data[output_path.len-1] = 'i';

        save_tokens_to_file(&output_path, tokens->next);

        debugf("File: %.*s\n", files_to_process[i].len, files_to_process[i].data);
        debugf("Bytes used: %ld\n", token_arena->bytes_used);
        debugf("Nodes created: %ld\n", token_arena->bytes_used / sizeof(tk_node));
        debugf("Max Macros: %ld\n\n", max_macros);

        delete_arena(token_arena);
        num_macros = 0;
    }
}
