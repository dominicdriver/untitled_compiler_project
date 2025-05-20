#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "enums.h"

typedef struct AST_node
{
    enum AST_type type;
    enum operator op;
    struct AST_node *child;
    struct AST_node *sibling;
    token tk;

} AST_node;

typedef struct binary_expr
{
    char *name;
    enum operator *ops;
    struct binary_expr *expr;
} binary_expr;

AST_node *create_ast_tree(void);
void initialise_parser(void);

#endif //PARSER_H
