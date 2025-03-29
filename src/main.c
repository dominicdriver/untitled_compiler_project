#include "common.h"
#include "debug.h"
#include "helper_functions.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern size_t max_macros;

void add_predefined(void) {
    const char predefined[] = {
        "#define __x86_64__ 1\n"
        "#define __LP64__ 1\n"
    };

    files[++files_top] = (file_info) {.buffer = {.size = strlen(predefined), .pos = 0},
                                      .current_pos = 0, .current_line = 1};
    FILES_TOP.buffer.data = malloc(strlen(predefined) + 1);

    strcpy(FILES_TOP.buffer.data, predefined);
    strcpy(FILES_TOP.filepath, "PREDEFINED");
}

int main(void) {
    char *files_to_process[] = {
        ""
    };


    for (size_t i = 0; i < sizeof(files_to_process) / sizeof(files_to_process[0]); i++) {
        char filepath[128] = {0};

        strcat(filepath, files_to_process[i]);

        add_file(filepath);
        add_predefined();

        token_arena = create_arena(TOKEN_ARENA_MAX_SIZE);

        tokens = allocate_from_arena(token_arena, sizeof(tk_node));
        tokens->token.lexeme[0] = '\0';
        tokens->next = NULL;

        scan_and_insert_tokens(tokens);

        process_preprocessing_tokens(tokens);

        char output_path[64] = {0};

        strcpy(output_path, "output/");
        strcat(output_path, files_to_process[i]);
        output_path[strlen(output_path)-1] = 'i';

        save_tokens_to_file(output_path, tokens);

        debugf("File: %s\n", files_to_process[i]);
        debugf("Bytes used: %ld\n", token_arena->bytes_used);
        debugf("Nodes created: %ld\n", token_arena->bytes_used / sizeof(tk_node));
        debugf("Max Macros: %ld\n\n", max_macros);

        delete_arena(token_arena);
        num_macros = 0;
    }
}
