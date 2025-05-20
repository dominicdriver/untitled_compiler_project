#ifndef ENUMS_H
#define ENUMS_H

enum subtype {
    // Keywords
    KW_AUTO,
    KW_BREAK,
    KW_CASE,
    KW_CHAR,
    KW_CONST,
    KW_CONTINUE,
    KW_DEFAULT,
    KW_DO,
    KW_DOUBLE,
    KW_ELSE,
    KW_ENUM,
    KW_EXTERN,
    KW_FLOAT,
    KW_FOR,
    KW_GOTO,
    KW_IF,
    KW_INLINE,
    KW_INT,
    KW_LONG,
    KW_REGISTER,
    KW_RESTRICT,
    KW_RETURN,
    KW_SHORT,
    KW_SIGNED,
    KW_SIZEOF,
    KW_STATIC,
    KW_STRUCT,
    KW_SWITCH,
    KW_TYPEDEF,
    KW_UNION,
    KW_UNSIGNED,
    KW_VOID,
    KW_VOLATILE,
    KW_WHILE,
    KW__BOOL,
    KW__COMPLEX,
    KW__IMAGINARY,

    // Punctuators
    PUN_LEFT_SQUARE_BRACKET,
    PUN_RIGHT_SQUARE_BRACKET,
    PUN_LEFT_PARENTHESIS,
    PUN_RIGHT_PARENTHESIS,
    PUN_LEFT_BRACE,
    PUN_RIGHT_BRACE,
    PUN_DOT,
    PUN_ARROW,
    PUN_INCREMENT,
    PUN_DECREMENT,
    PUN_AMPERSAND,
    PUN_ASTERISK,
    PUN_PLUS,
    PUN_MINUS,
    PUN_TILDE,
    PUN_EXCLAMATION_MARK,
    PUN_FWD_SLASH,
    PUN_REMAINDER,
    PUN_LEFT_BITSHIFT,
    PUN_RIGHT_BITSHIFT,
    PUN_LESS_THAN,
    PUN_GREATER_THAN,
    PUN_LESS_THAN_EQUAL,
    PUN_GREATER_THAN_EQUAL,
    PUN_EQUALITY,
    PUN_INEQUALITY,
    PUN_BITWISE_XOR,
    PUN_BITWISE_OR,
    PUN_LOGICAL_AND,
    PUN_LOGICAL_OR,
    PUN_QUESTION_MARK,
    PUN_COLON,
    PUN_SEMICOLON,
    PUN_ELLIPSIS,
    PUN_ASSIGNMENT,
    PUN_MULTIPLY_ASSIGNMENT,
    PUN_DIVIDE_ASSIGNMENT,
    PUN_MOD_ASSIGNMENT,
    PUN_PLUS_ASSIGNMENT,
    PUN_MINUS_ASSIGNMENT,
    PUN_LEFT_BITSHIFT_ASSIGNMENT,
    PUN_RIGHT_BITSHIFT_ASSIGNMENT,
    PUN_AND_ASSIGNMENT,
    PUN_XOR_ASSIGNMENT,
    PUN_OR_ASSIGNMENT,
    PUN_COMMA,
    PUN_HASH,
    PUN_DOUBLE_HASH,
    PUN_NONE,

    // Constants
    CONST_INTEGER,
    CONST_UNSIGNED_INT,
    CONST_LONG,
    CONST_UNSIGNED_LONG,
    CONST_LONG_LONG,
    CONST_UNSIGNED_LONG_LONG,
    CONST_CHAR,
    CONST_WIDE_CHAR,
    CONST_FLOAT,
    CONST_DOUBLE,
    CONST_LONG_DOUBLE,

    // Directives
    DIRECTIVE_IF,
    DIRECTIVE_IFDEF,
    DIRECTIVE_IFNDEF,
    DIRECTIVE_ELIF,
    DIRECTIVE_ELSE,
    DIRECTIVE_ENDIF,
    DIRECTIVE_INCLUDE,
    DIRECTIVE_DEFINE,
    DIRECTIVE_UNDEF,
    DIRECTIVE_LINE,
    DIRECTIVE_WARNING,
    DIRECTIVE_ERROR,
    DIRECTIVE_PRAGMA,
    DIRECTIVE_NULL,

    HEADER_Q,
    HEADER_H
};

enum token_type {
    KEYWORD,
    IDENTIFIER,
    CONSTANT,
    STRING_LITERAL,
    PUNCTUATOR,
    DIRECTIVE,
    HEADER_NAME,
    NEWLINE,
    BLANK,
    ARGUMENT,
    END
};

#define OPERATORS                                                                             \
    OP(NONE, PUN_NONE, "")                                                                    \
                                                                                              \
    OP(INCREMENT, PUN_INCREMENT, "INCREMENT") /* Used for postfix and prefix increment */     \
    OP(DECREMENT, PUN_DECREMENT, "DECREMENT") /* Used for postfix and prefix decrement */     \
    OP(DOT, PUN_DOT, "DOT")                                                                   \
    OP(ARROW, PUN_ARROW, "ARROW")                                                             \
                                                                                              \
    OP(LOGICAL_NOT, PUN_EXCLAMATION_MARK, "LOGICAL NOT")                                      \
    OP(BITWISE_NOT, PUN_TILDE, "BITWISE NOT")                                                 \
    OP(SIZEOF, KW_SIZEOF, "sizeof")                                                           \
                                                                                              \
    OP(PLUS, PUN_PLUS, "PLUS") /* Used for addition and unary plus */                         \
    OP(MINUS, PUN_MINUS, "MINUS") /* Used for subtraction and unary minus */                  \
                                                                                              \
    OP(ASTERISK, PUN_ASTERISK, "ASTERISK") /* Used for multiplication and dereference */      \
    OP(DIVIDE, PUN_FWD_SLASH, "DIV")                                                          \
    OP(MOD, PUN_REMAINDER, "MOD")                                                             \
                                                                                              \
                                                                                              \
    OP(LEFT_BITSHIFT, PUN_LEFT_BITSHIFT, "LEFT BITSHIFT")                                     \
    OP(RIGHT_BITSHIFT, PUN_RIGHT_BITSHIFT, "RIGHT BITSHIFT")                                  \
                                                                                              \
                                                                                              \
    OP(LESS_THAN, PUN_LESS_THAN, "LESS THAN")                                                 \
    OP(LESS_THAN_EQUAL, PUN_LESS_THAN_EQUAL, "LESS THAN EQUAL")                               \
    OP(GREATER_THAN, PUN_GREATER_THAN, "GREATER THAN")                                        \
    OP(GREATER_THAN_EQUAL, PUN_GREATER_THAN_EQUAL, "GREATER THAN EQUAL")                      \
                                                                                              \
    OP(EQUALITY, PUN_EQUALITY, "EQUALITY")                                                    \
    OP(INEQUALITY, PUN_INEQUALITY, "INEQUALITY")                                              \
                                                                                              \
    OP(AMPERSAND, PUN_AMPERSAND, "AMPERSAND") /* Used for bitwise AND and address-of */       \
                                                                                              \
    OP(BITWISE_XOR, PUN_BITWISE_XOR, "BITWISE XOR")                                           \
    OP(BITWISE_OR, PUN_BITWISE_OR, "BITWISE OR")                                              \
                                                                                              \
    OP(LOGICAL_AND, PUN_LOGICAL_AND, "LOGICAL AND")                                           \
                                                                                              \
    OP(LOGICAL_OR, PUN_LOGICAL_OR, "LOGICAL OR")                                              \
                                                                                              \
    OP(QUESTION_MARK, PUN_QUESTION_MARK, "QUESTION MARK")                                     \
                                                                                              \
    OP(ASSIGNMENT, PUN_ASSIGNMENT, "ASSIGNMENT")                                              \
    OP(PLUS_ASSIGNMENT, PUN_PLUS_ASSIGNMENT, "PLUS ASSIGNMENT")                               \
    OP(MINUS_ASSIGNMENT, PUN_MINUS_ASSIGNMENT, "MINUS ASSIGNMENT")                            \
    OP(MULTIPLY_ASSIGNMENT, PUN_MULTIPLY_ASSIGNMENT, "MULTIPLY ASSIGNMENT")                   \
    OP(DIVIDE_ASSIGNMENT, PUN_DIVIDE_ASSIGNMENT, "DIVIDE ASSIGNMENT")                         \
    OP(MOD_ASSIGNMENT, PUN_MOD_ASSIGNMENT, "MOD ASSIGNMENT")                                  \
    OP(LEFT_BITSHIFT_ASSIGNMENT, PUN_LEFT_BITSHIFT_ASSIGNMENT, "LEFT BITSHIFT ASSIGNMENT")    \
    OP(RIGHT_BITSHIFT_ASSIGNMENT, PUN_RIGHT_BITSHIFT_ASSIGNMENT, "RIGHT BITSHIFT ASSIGNMENT") \
    OP(AND_ASSIGNMENT, PUN_AND_ASSIGNMENT, "AND ASSIGNMENT")                                  \
    OP(XOR_ASSIGNMENT, PUN_XOR_ASSIGNMENT, "XOR ASSIGNMENT")                                  \
    OP(OR_ASSIGNMENT, PUN_OR_ASSIGNMENT, "OR ASSIGNMENT")                                     \
                                                                                              \
    OP(COMMA, PUN_COMMA, "COMMA")

#define OP(op, pun, str) op,
enum operator {
    OPERATORS
};
#undef OP

enum AST_type {
    none,
    unary,
    binary,
    ternary,
    assignment
};

#endif //ENUMS_H
