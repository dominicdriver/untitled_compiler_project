#include "enums.h"
#include "common.h"
#include "memory.h"
#include "parser.h"
#include "helper_functions.h"

#include <stdbool.h>
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

typedef enum {
    MULTIPLICATIVE_EXPR,
    ADDITIVE_EXPR,
    SHIFT_EXPR,
    RELATIONAL_EXPR,
    EQUALITY_EXPR,
    BITWISE_AND_EXPR,
    EXCLUSIVE_OR_EXPR,
    INCLUSIVE_OR_EXPR,
    LOGICAL_AND_EXPR,
    LOGICAL_OR_EXPR,

    ARGUMENT_EXPR_LIST
} binary_expression_names;

#define OP(op, pun, str) [pun] = op,
enum operator pun_to_op[] = {
    OPERATORS
};
#undef OP

enum operator assignment_ops[] = {
    ASSIGNMENT,
    MULTIPLY_ASSIGNMENT,
    DIVIDE_ASSIGNMENT,
    MOD_ASSIGNMENT,
    PLUS_ASSIGNMENT,
    MINUS_ASSIGNMENT,
    LEFT_BITSHIFT_ASSIGNMENT,
    RIGHT_BITSHIFT_ASSIGNMENT,
    AND_ASSIGNMENT,
    XOR_ASSIGNMENT,
    OR_ASSIGNMENT,
    NONE
};

// Based on C grammar, doesn't include increment, decrement, etc.
enum operator unary_ops[] = {
    AMPERSAND,
    ASTERISK,
    PLUS,
    MINUS,
    BITWISE_NOT,
    LOGICAL_NOT,
    NONE
};

uint8_t binary_op_precedence[] = {
    [ASTERISK] = 12,
    [DIVIDE] = 12,
    [MOD] = 12,

    [PLUS] = 11,
    [MINUS] = 11,

    [LEFT_BITSHIFT] = 10,
    [RIGHT_BITSHIFT] = 10,

    [GREATER_THAN] = 9,
    [GREATER_THAN_EQUAL] = 9,
    [LESS_THAN] = 9,
    [LESS_THAN_EQUAL] = 9,

    [EQUALITY] = 8,
    [INEQUALITY] = 8,

    [AMPERSAND] = 7,

    [BITWISE_XOR] = 6,

    [BITWISE_OR] = 5,

    [LOGICAL_AND] = 4,

    [LOGICAL_OR] = 3,
};

binary_expr binary_expressions[] = {
    [MULTIPLICATIVE_EXPR] =
    {
        .name  = "multiplicative_expression",
        .ops   = (enum operator[]) {ASTERISK, DIVIDE, MOD, NONE},
        .expr  = NULL
    },
    [ADDITIVE_EXPR] =
    {
        .name = "additive_expression",
        .ops  = (enum operator[]) {PLUS, MINUS, NONE},
        .expr = &binary_expressions[0]
    },
    [SHIFT_EXPR] =
    {
        .name = "shift_expression",
        .ops  = (enum operator[]) {LEFT_BITSHIFT, RIGHT_BITSHIFT, NONE},
        .expr = &binary_expressions[1]
    },
    [RELATIONAL_EXPR] =
    {
        .name = "relational_expression",
        .ops = (enum operator[]) {LESS_THAN, GREATER_THAN, LESS_THAN_EQUAL, GREATER_THAN_EQUAL, NONE},
        .expr = &binary_expressions[2]
    },
    [EQUALITY_EXPR] =
    {
        .name = "equality_expression",
        .ops = (enum operator[]) {EQUALITY, INEQUALITY, NONE},
        .expr = &binary_expressions[3]
    },
    [BITWISE_AND_EXPR] =
    {
        .name = "bitwise_and_expression",
        .ops = (enum operator[]) {AMPERSAND, NONE},
        .expr = &binary_expressions[4]
    },
    [EXCLUSIVE_OR_EXPR] =
    {
        .name = "exclusive_or_expression",
        .ops = (enum operator[]) {BITWISE_XOR, NONE},
        .expr = &binary_expressions[5]
    },
    [INCLUSIVE_OR_EXPR] =
    {
        .name = "inclusive_or_expression",
        .ops = (enum operator[]) {BITWISE_OR, NONE},
        .expr = &binary_expressions[6]
    },
    [LOGICAL_AND_EXPR] =
    {
        .name = "logical_and_expression",
        .ops = (enum operator[]) {LOGICAL_AND, NONE},
        .expr = &binary_expressions[7]
    },
    [LOGICAL_OR_EXPR] =
    {
        .name = "logical_or_expression",
        .ops = (enum operator[]) {LOGICAL_OR, NONE},
        .expr = &binary_expressions[8]
    },

    [ARGUMENT_EXPR_LIST] =
    {
        .name = "argument_expression_list",
        .ops = (enum operator[]) {COMMA, NONE},
        .expr = NULL
    }
};

#define OP(op, pun, str) str,
char *operator_strings[] = {
    OPERATORS
};
#undef OP

void ast_error(token *error_token, char *message);
void print_ast(const AST_node *root, uint8_t level);

AST_node *create_cast_expression(void);
AST_node *create_expression(int precedence);
AST_node *create_assignment_expression(void);

token *consume_token(void) {
    if (tk_stream_pos == tk_stream_len) {
        return &end_token;
    }

    current_token = &token_stream[tk_stream_pos];
    return &token_stream[tk_stream_pos++];
}

token *peek_token(void) {
    if (tk_stream_pos == tk_stream_len) {
        return &end_token;
    }

    return &token_stream[tk_stream_pos];
}

bool operator_in_array(enum operator op, enum operator ops[]) {
    for (enum operator *op_ptr = &ops[0]; *op_ptr != NONE; op_ptr++) {
        if (op == *op_ptr) return true;
    }

    return false;
}

AST_node *create_unary_node(enum operator op, AST_node *node) {
    if (node == &error_node) {
        return &error_node;
    }

    AST_node *unary_node = allocate_from_arena(ast_arena, sizeof(AST_node));
    *unary_node = (AST_node) {0};

    unary_node->type = unary;
    unary_node->op = op;
    unary_node->child = node;

    return unary_node;
}

AST_node *create_binary_node(enum operator op, AST_node *left, AST_node *right) {
    if (left == &error_node || right == &error_node) {
        return &error_node;
    }

    AST_node *binary_node = allocate_from_arena(ast_arena, sizeof(AST_node));
    *binary_node = (AST_node) {0};

    binary_node->type = binary;
    binary_node->op = op;
    binary_node->child = left;
    binary_node->child->sibling = right;

    return binary_node;
}

AST_node *create_ternary_node(enum operator op, AST_node *left, AST_node *mid, AST_node *right) {
    if (left == &error_node || mid == &error_node || right == &error_node) {
        return &error_node;
    }

    AST_node *ternary_node = allocate_from_arena(ast_arena, sizeof(AST_node));
    *ternary_node = (AST_node) {0};

    ternary_node->type = ternary;
    ternary_node->op = op;
    ternary_node->child = left;
    ternary_node->child->sibling = mid;
    ternary_node->child->sibling->sibling = right;

    return ternary_node;
}


AST_node *create_primary_expression(void) {
    AST_node *node = allocate_from_arena(ast_arena, sizeof(AST_node));
    *node = (AST_node) {0};

    token *token = consume_token();

    if (token->type == PUNCTUATOR && token->subtype == PUN_LEFT_PARENTHESIS) {
        node = create_expression(0);

        token = consume_token();

        if (!(token->type == PUNCTUATOR && token->subtype == PUN_RIGHT_PARENTHESIS)) {
            ast_error(token, "Expected ')' after expression");
        }

        return node;
    }

    switch (token->type) {
        case IDENTIFIER:
        case CONSTANT:
        case STRING_LITERAL:
            node->tk = *current_token;
            break;

        default:
            ast_error(current_token, "Expected primary expression");
            return &error_node;
    }

    return node;
}

AST_node *create_postfix_expression(void) {
    // TODO: Implement
    return create_primary_expression();
}

AST_node *create_unary_expression(void) {
    AST_node *node = NULL;

    enum operator unary_op;
    token *token = peek_token();

    // Check if the next non-terminal should be a primary expression
    if (token->type == IDENTIFIER || token->type == CONSTANT || token->type == STRING_LITERAL ||
        token->subtype == PUN_LEFT_PARENTHESIS) {

            node = create_postfix_expression();
            node->type = unary;

            return node;
        }

    token = consume_token();
    unary_op = pun_to_op[token->subtype];

    if (operator_in_array(unary_op, unary_ops)) {
        node = create_cast_expression();
        node->type = unary;

        return node;
    }

    if (unary_op == SIZEOF && peek_token()->subtype == PUN_LEFT_PARENTHESIS) {
        // TODO: '(' typename ')'
    }

    switch (unary_op) {
        case INCREMENT:
        case DECREMENT:
        case SIZEOF:
            break;

        default:
            ast_error(current_token, "Expected unary operator");
            return &error_node;
    }

    node = create_unary_expression();
    node = create_unary_node(unary_op, node);

    return node;
}

AST_node *create_cast_expression(void) {
    AST_node *node = create_unary_expression();

    return node;
}

AST_node *create_binary_expression(binary_expr *expr, int precedence) {
    AST_node *left = NULL;
    AST_node *right = NULL;

    if (expr == &binary_expressions[MULTIPLICATIVE_EXPR]) {
        left = create_cast_expression();
    } else if (expr == &binary_expressions[ARGUMENT_EXPR_LIST]) {
        left = create_assignment_expression();
    } else {
        left = create_binary_expression(expr->expr, precedence);
    }

    token *token = peek_token();

    if (token->type != PUNCTUATOR) {
        if (left != &error_node) {
            ast_error(current_token, "Expected punctuator");
        }
        return &error_node;
    }

    enum operator op = pun_to_op[token->subtype];

    while (operator_in_array(op, expr->ops) &&
           binary_op_precedence[op] >= precedence) {

        consume_token();

        if (expr == &binary_expressions[MULTIPLICATIVE_EXPR]) {
            right = create_cast_expression();
        } else {
            right = create_binary_expression(expr->expr, binary_op_precedence[op] + 1);
        }

        left = create_binary_node(op, left, right);
        token = peek_token();
        op = pun_to_op[token->subtype];
    }

    return left;
}

AST_node *create_conditional_expression(void) {
    AST_node *left = create_binary_expression(&binary_expressions[LOGICAL_OR_EXPR], 0);
    AST_node *mid = NULL;
    AST_node *right = NULL;

    token *token = peek_token();

    if (token->subtype != PUN_QUESTION_MARK) {
        return left;
    }

    mid = create_expression(0);

    if (token->subtype != PUN_COLON) {
        ast_error(current_token, "Expected : in ternary expression");
        return &error_node;
    }

    right = create_conditional_expression();

    return create_ternary_node(QUESTION_MARK, left, mid, right);
}

AST_node *create_assignment_expression(void) {
    AST_node *left = create_conditional_expression();
    AST_node *right = NULL;

    token *token = peek_token();
    enum operator op = pun_to_op[token->subtype];

    if (!operator_in_array(op, assignment_ops)) {
         return left;
    }

    // If here, the next token is an assignment operator,
    // so the conditional expression is treated as a unary expression

    consume_token();

    right = create_assignment_expression();
    left = create_binary_node(op, left, right);

    return left;
}

AST_node *create_expression(int precedence) {
    AST_node *left = create_assignment_expression();
    AST_node *right = NULL;

    token *token = peek_token();

    while (token->subtype == PUN_COMMA && operator_precedence[token->subtype] >= precedence) {
        enum operator op = COMMA;

        consume_token();

        right = create_assignment_expression();
        left = create_binary_node(op, left, right);
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
    AST_node *node = NULL;
    token *token = NULL;
    ast_arena = create_arena(AST_ARENA_MAX_SIZE);


    while ((token = peek_token()) != NULL) {
        switch (token->type) {
            case IDENTIFIER:
            case CONSTANT:
            default:
                node = create_expression(0);
                print_ast(node, 0);
                exit(0);
                break;
        }
    }

    return ast_tree;
}

void ast_error(token *error_token, char *message) {
    error(error_token->src_filepath, error_token->line, message);
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
