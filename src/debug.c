#include "common.h"
#include "enums.h"
#include "helper_functions.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>

void print_token(token token) {
    char token_type[16];

    switch (token.type) {
        case PUNCTUATOR: printf("%s", token.lexeme); break;
        case STRING_LITERAL: printf("\"%s\"", token.lexeme); break;
        case NEWLINE: printf("\n"); break;
        default: printf("%s ", token.lexeme); break;
    }
    return;
    switch (token.type) {
        case KEYWORD: strcpy(token_type, "Keyword"); break;
        case IDENTIFIER: strcpy(token_type, "Identifier"); break;
        case PUNCTUATOR: strcpy(token_type, "Punctuation"); break;
        case CONSTANT: strcpy(token_type, "Constant"); break;
        case STRING_LITERAL: strcpy(token_type, "String"); break;
        case DIRECTIVE: strcpy(token_type, "Directive"); break;
        case NEWLINE: strcpy(token_type, "New line"); break;
        case END: strcpy(token_type, "End of File"); break;

        default: break;
    }

    if (token.type == NEWLINE) {
        printf("%s: Line: %d\n", token_type, token.line);
    }
    else {
        printf("%s: %s, Line: %d\n", token_type, token.lexeme, token.line);
    }
}

void print_all_tokens(void) {
    size_t token_count = 0;
    printf("Tokens:\n");
    for (tk_node *ptr = tokens; ptr->next != NULL; ptr = ptr->next) {
        if (ptr->next->next == NULL) continue;
        if (ptr->token.type == BLANK) continue;
        if (ptr->token.type != NEWLINE || (ptr->token.type == NEWLINE && ptr->next->next->token.type != NEWLINE)) {
            print_token(ptr->token);
        }
        token_count++;
    }

    printf("Number of tokens: %ld\n", token_count);
}

void print_tokens(tk_node *list_ptr, size_t num_tokens) {
    for (size_t tokens_printed = 0; tokens_printed < num_tokens; tokens_printed++) {
        print_token(list_ptr->token);
        list_ptr = list_ptr->next;
    }
}

void print_list_segment(tk_list_segment segment) {
    for (tk_node *ptr = segment.start; ptr != NULL; ptr = advance_list(ptr, 1)) {
        char *format_string;

        if (ptr->token.type == STRING_LITERAL) {
            format_string = "\"%s\"";
        } else if (ptr->token.subtype == PUN_LEFT_PARENTHESIS ||
                   ptr->token.subtype == PUN_DOT) {
            format_string = "%s";
        } else if (ptr->next != NULL && (ptr->next->token.subtype == PUN_RIGHT_PARENTHESIS ||
                                         ptr->next->token.subtype == PUN_DOT)) {
            format_string = "%s";
        } else {
            format_string = "%s ";
        }

        printf(format_string, ptr->token.lexeme);
    }

    printf("\n");
}

macro *macro_exists_2(const char *identifier, bool function_like) {
    uint64_t identifier_hash = hash(identifier);
    for (size_t i = 0; i < num_macros; i++) {
        if (identifier_hash == macros[i].hash) {
            if (strcmp(identifier, macros[i].name) != 0)
            printf("QA\n");
        if (
            macros[i].is_function_like == function_like) {
            return &macros[i];
        }
        }
    }

    return NULL;
}

void print_replacement_tokens(token *replacement)
{
    while(replacement->line != 0) {
        printf("%s ", replacement->lexeme);
        replacement++;
    }
    printf("\n");
}
