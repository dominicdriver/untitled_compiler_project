#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "enums.h"

typedef struct AST_node
{
    enum operator op;
    struct AST_node *child;
    struct AST_node *sibling;
    token tk;

} AST_node;

AST_node *create_ast_tree(void);
void initialise_parser(void);

#endif //PARSER_H
