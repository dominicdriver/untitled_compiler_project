#ifndef LEXER_H
#define LEXER_H

enum keyword {
    AUTO,
    BREAK,
    CASE,
    CHAR,
    CONST,
    CONTINUE,
    DEFAULT,
    DO,
    DOUBLE,
    ELSE,
    ENUM,
    EXTERN,
    FLOAT,
    FOR,
    GOTO,
    IF,
    INLINE,
    INT,
    LONG,
    REGISTER,
    RESTRICT,
    RETURN,
    SHORT,
    SIGNED,
    SIZEOF,
    STATIC,
    STRUCT,
    SWITCH,
    TYPEDEF,
    UNION,
    UNSIGNED,
    VOID,
    VOLATILE,
    WHILE,
    _BOOL,
    _COMPLEX,
    _IMAGINARY,
};

enum punctuator {
    LEFT_SQUARE_BRACKET,
    RIGHT_SQUARE_BRACKET,
    LEFT_PARENTHESIS,
    RIGHT_PARENTHESIS,
    LEFT_BRACE,
    RIGHT_BRACE,
    DOT,
    ARROW,
    INCREMENT,
    DECREMENT,
    AMPERSAND,
    ASTERISK,
    PLUS,
    MINUS,
    TILDE,
    EXCLIMATION_MARK,
    FWD_SLASH,
    REMAINDER,
    LEFT_BITSHIFT,
    RIGHT_BITSHIFT,
    LESS_THAN,
    GREATER_THAN,
    LESS_THAN_EQUAL,
    GREATER_THAN_EQUAL,
    EQUALITY,
    INEQUALITY,
    BITWISE_XOR,
    BITWISE_OR,
    LOGICAL_AND,
    LOGICAL_OR,
    QUESTION_MARK,
    COLON,
    SEMICOLON,
    ELLIPSIS,
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
    COMMA,
    HASH,
    DOUBLE_HASH
};

enum constant {
    // TODO: Add all the other constants
    INTEGER
};

enum token_type {
    KEYWORD,
    IDENTIFER,
    CONSTANT,
    STRING_LITERAL,
    PUNCTUATOR
};

enum preprocess_token_type {
    PP_HEADER_NAME,
    PP_IDENTIFER,
    PP_PP_NUMBER,
    PP_CHARACTER_CONSTANT,
    PP_STRING_LITERAL,
    PP_PUNCTUATOR,
    PP_OTHER
};

typedef struct {
    enum token_type type;
    union {
        enum keyword keyword;
        enum punctuator punctuator;
        enum constant constant;
    };
    char lexeme[32]; // TODO: Fixed size for now, will break for long lexemes
    int line;
} token;

#endif //LEXER_H
