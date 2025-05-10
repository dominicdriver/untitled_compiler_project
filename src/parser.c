#include "enums.h"
#include "common.h"
#include "memory.h"
#include "parser.h"
#include "helper_functions.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define AST_ARENA_MAX_SIZE (sizeof(AST_node) * (1 << 20))
#define TK_STREAM_ARENA_MAX_SIZE (sizeof(token) * (1 << 20))

memory_arena *ast_arena;
memory_arena *token_stream_arena;

AST_node *ast_tree;
token *token_stream;
uint64_t tk_stream_pos = 0;
uint64_t tk_stream_len = 0;

token *current_token;
token end_token = {.type = END, 0};

AST_node error_node = {0};

#define OP(op, str) str,
char *operator_strings[] = {
    OPERATORS
};
#undef OP

void print_ast(const AST_node *root, uint8_t level);

token *consume_token() {
    if (tk_stream_pos == tk_stream_len) {
        return &end_token;
    }

    current_token = &token_stream[tk_stream_pos];
    return &token_stream[tk_stream_pos++];
}

token *peek_token() {
    if (tk_stream_pos == tk_stream_len) {
        return &end_token;
    }

    return &token_stream[tk_stream_pos];
}

AST_node *create_binary_node(enum operator op, AST_node *left, AST_node *right) {
    AST_node *binary_node = allocate_from_arena(ast_arena, sizeof(AST_node));
    *binary_node = (AST_node) {0};

    binary_node->op = op;
    binary_node->child = left;
    binary_node->child->sibling = right;

    return binary_node;
}


AST_node *create_primary_expression() {
    AST_node *node = allocate_from_arena(ast_arena, sizeof(AST_node));
    *node = (AST_node) {0};

    token *tk = consume_token();

    switch (tk->type) {
        case IDENTIFIER:
        case CONSTANT:
        case STRING_LITERAL:
            node->tk = *current_token;
            break;

        default:
            error(current_token->src_filepath, current_token->line, "Expected primary expression");
            return &error_node;
    }

    return node;
}

AST_node *create_postfix_expression() {
    return create_primary_expression();
}

AST_node *create_unary_expression() {
    AST_node *node = create_postfix_expression();

    return node;
}

AST_node *create_cast_expression() {
    AST_node *node = create_unary_expression();

    return node;
}

AST_node *create_multiplicative_expression(int precedence) {
    AST_node *left = create_cast_expression();
    AST_node *right = NULL;

    token *next_token = peek_token();

    while ((next_token->subtype == PUN_ASTERISK  ||
            next_token->subtype == PUN_FWD_SLASH ||
            next_token->subtype == PUN_REMAINDER) && operator_precedence[next_token->subtype] >= precedence) {

        enum operator op;
        switch (next_token->subtype) {
            case PUN_ASTERISK:  op = MULTIPLY; break;
            case PUN_FWD_SLASH: op = DIVIDE; break;
            case PUN_REMAINDER: op = MOD; break;
            default:
                error(current_token->src_filepath, current_token->line, "Unknown operator");
                return &error_node;
        }

        consume_token();

        right = create_multiplicative_expression(operator_precedence[next_token->subtype] + 1);
        left = create_binary_node(op, left, right);
        next_token = peek_token();
    }

    return left;
}

AST_node *create_additive_expression(int precedence) {
    AST_node *left = create_multiplicative_expression(precedence);
    AST_node *right = NULL;

    token *next_token = peek_token();

    while ((next_token->subtype == PUN_PLUS ||
            next_token->subtype == PUN_MINUS) && operator_precedence[next_token->subtype] >= precedence) {

        enum operator op;
        switch (next_token->subtype) {
            case PUN_PLUS:  op = ADD; break;
            case PUN_MINUS: op = SUB; break;
            default:
                error(current_token->src_filepath, current_token->line, "Unknown operator");
                return &error_node;
        }

        consume_token();

        right = create_additive_expression(operator_precedence[next_token->subtype] + 1);
        left = create_binary_node(op, left, right);
        next_token = peek_token();
    }

    return left;
}

void initialise_parser(void) {
    size_t token_count = 0;

    for (tk_node *ptr = tokens->next; ptr != NULL; ptr = advance_list(ptr, 1)) {
        // END and NEWLINE tokens aren't needed from the parser onwards
        if (ptr->token.type == END || ptr->token.type == NEWLINE) continue;

        token_count++;
    }

    ast_arena = create_arena(AST_ARENA_MAX_SIZE);
    token_stream_arena = create_arena(TK_STREAM_ARENA_MAX_SIZE);

    token_stream = allocate_from_arena(token_stream_arena, token_count * sizeof(token));
    tk_stream_len = token_count;
    token_count = 0;

    for (tk_node *ptr = tokens->next; ptr != NULL; ptr = advance_list(ptr, 1)) {
        // END and NEWLINE tokens aren't needed from the parser onwards
        if (ptr->token.type == END || ptr->token.type == NEWLINE) continue;

        token_stream[token_count] = ptr->token;
        token_count++;
    }
}

AST_node *create_ast_tree(void) {
    token *next_token = NULL;
    ast_arena = create_arena(AST_ARENA_MAX_SIZE);


    while ((next_token = peek_token()) != NULL) {
        switch (next_token->type) {
            case CONSTANT:;
                AST_node *node = create_additive_expression(0);
                print_ast(node, 0);
                exit(0);
            default:
                break;
        }
    }

    return ast_tree;
}

void print_ast(const AST_node *root, uint8_t level) {
    char *op_str = operator_strings[root->op];

    for (uint8_t i = 0; i < level; i++) printf("  ");
    printf("%s%s\n", op_str, root->tk.lexeme);

    if (root->child != NULL) print_ast(root->child, level + 1);

    while (root->sibling != NULL) {
        root = root->sibling;

        print_ast(root, level);
    }
}
