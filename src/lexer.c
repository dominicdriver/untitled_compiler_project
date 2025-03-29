#include "enums.h"
#include "common.h"
#include "lexer.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

memory_arena *token_arena;
tk_node *tokens;
size_t num_tokens = 0;

file_info files[MAX_NUM_FILES];
int files_top = -1;

bool escaped;

// Maps subtype enums to their string representation
char *subtype_strings[] = {
    // Keywords
    [KW_AUTO] = "auto",
    [KW_BREAK] = "break",
    [KW_CASE] = "case",
    [KW_CHAR] = "char",
    [KW_CONST] = "const",
    [KW_CONTINUE] = "continue",
    [KW_DEFAULT] = "default",
    [KW_DO] = "do",
    [KW_DOUBLE] = "double",
    [KW_ELSE] = "else",
    [KW_ENUM] = "enum",
    [KW_EXTERN] = "extern",
    [KW_FLOAT] = "float",
    [KW_FOR] = "for",
    [KW_GOTO] = "goto",
    [KW_IF] = "if",
    [KW_INLINE] = "inline",
    [KW_INT] = "int",
    [KW_LONG] = "long",
    [KW_REGISTER] = "register",
    [KW_RESTRICT] = "restrict",
    [KW_RETURN] = "return",
    [KW_SHORT] = "short",
    [KW_SIGNED] = "signed",
    [KW_SIZEOF] = "sizeof",
    [KW_STATIC] = "static",
    [KW_STRUCT] = "struct",
    [KW_SWITCH] = "switch",
    [KW_TYPEDEF] = "typedef",
    [KW_UNION] = "union",
    [KW_UNSIGNED] = "unsigned",
    [KW_VOID] = "void",
    [KW_VOLATILE] = "volatile",
    [KW_WHILE] = "while",
    [KW__BOOL] = "_Bool",
    [KW__COMPLEX] = "_Complex",
    [KW__IMAGINARY] = "_Imaginary",

    // Punctuators
    [PUN_LEFT_SQUARE_BRACKET] = "[",
    [PUN_RIGHT_SQUARE_BRACKET] = "]",
    [PUN_LEFT_PARENTHESIS] = "(",
    [PUN_RIGHT_PARENTHESIS] = ")",
    [PUN_LEFT_BRACE] = "{",
    [PUN_RIGHT_BRACE] = "}",
    [PUN_DOT] = ".",
    [PUN_ARROW] = "->",
    [PUN_INCREMENT] = "++",
    [PUN_DECREMENT] = "--",
    [PUN_AMPERSAND] = "&",
    [PUN_ASTERISK] = "*",
    [PUN_PLUS] = "+",
    [PUN_MINUS] = "-",
    [PUN_TILDE] = "~",
    [PUN_EXCLAMATION_MARK] = "!",
    [PUN_FWD_SLASH] = "/",
    [PUN_REMAINDER] = "%",
    [PUN_LEFT_BITSHIFT] = "<<",
    [PUN_RIGHT_BITSHIFT] = ">>",
    [PUN_LESS_THAN] = "<",
    [PUN_GREATER_THAN] = ">",
    [PUN_LESS_THAN_EQUAL] = "<=",
    [PUN_GREATER_THAN_EQUAL] = ">=",
    [PUN_EQUALITY] = "==",
    [PUN_INEQUALITY] = "!=",
    [PUN_BITWISE_XOR] = "^",
    [PUN_BITWISE_OR] = "|",
    [PUN_LOGICAL_AND] = "&&",
    [PUN_LOGICAL_OR] = "||",
    [PUN_QUESTION_MARK] = "?",
    [PUN_COLON] = ":",
    [PUN_SEMICOLON] = ";",
    [PUN_ELLIPSIS] = "...",
    [PUN_ASSIGNMENT] = "=",
    [PUN_MULTIPLY_ASSIGNMENT] = "*=",
    [PUN_DIVIDE_ASSIGNMENT] = "/=",
    [PUN_MOD_ASSIGNMENT] = "%=",
    [PUN_PLUS_ASSIGNMENT] = "+=",
    [PUN_MINUS_ASSIGNMENT] = "-=",
    [PUN_LEFT_BITSHIFT_ASSIGNMENT] = "<<=",
    [PUN_RIGHT_BITSHIFT_ASSIGNMENT] = ">>=",
    [PUN_AND_ASSIGNMENT] = "&=",
    [PUN_XOR_ASSIGNMENT] = "^=",
    [PUN_OR_ASSIGNMENT] = "|=",
    [PUN_COMMA] = ",",
    [PUN_HASH] = "#",
    [PUN_DOUBLE_HASH] = "##",

    [CONST_INTEGER] = "",
    [CONST_UNSIGNED_INT] = "",
    [CONST_LONG] = "",
    [CONST_UNSIGNED_LONG] = "",
    [CONST_LONG_LONG] = "",
    [CONST_UNSIGNED_LONG_LONG] = "",
    [CONST_FLOAT] = "",
    [CONST_DOUBLE] = "",
    [CONST_LONG_DOUBLE] = "",
    [CONST_CHAR] = "",
    [CONST_WIDE_CHAR] = "",

    // Directives
    [DIRECTIVE_IF] = "if",
    [DIRECTIVE_IFDEF] = "ifdef",
    [DIRECTIVE_IFNDEF] = "ifndef",
    [DIRECTIVE_ELIF] = "elif",
    [DIRECTIVE_ELSE] = "else",
    [DIRECTIVE_ENDIF] = "endif",
    [DIRECTIVE_INCLUDE] = "include",
    [DIRECTIVE_DEFINE] = "define",
    [DIRECTIVE_UNDEF] = "undef",
    [DIRECTIVE_LINE] = "line",
    [DIRECTIVE_WARNING] = "warning",
    [DIRECTIVE_ERROR] = "error",
    [DIRECTIVE_PRAGMA] = "pragma",
    [DIRECTIVE_NULL] = ""
};

#define NUM_SUBTYPES sizeof(subtype_strings) / sizeof(subtype_strings[0])

// Removes tokens from start (inclusive) to end (exclusive)
// Returns pointer to element before start
tk_node *remove_tokens(const tk_node *start, const tk_node *end) {
    tk_node *ptr = tokens;

    for (; ptr->next != start; ptr = ptr->next);

    tk_node *before_remove = ptr;

    for (; ptr != end; ptr = ptr->next);

    before_remove->next = ptr;

    return before_remove;
}

char consume_next_char(void) {
    if (FILES_TOP.buffer.pos == FILES_TOP.buffer.size) {
        if (escaped) error(FILES_TOP.filepath, FILES_TOP.current_line, "Lone \\");
        return EOF;
    }

    const char c = FILES_TOP.buffer.data[FILES_TOP.buffer.pos++];

    if (c == '\n') {
        FILES_TOP.current_pos = 0;
        FILES_TOP.current_line++;
    }

    escaped = !escaped && (c == '\\');

    return c;
}

char peek_next_char(void) {
    if (FILES_TOP.buffer.pos == FILES_TOP.buffer.size) {
        return EOF;
    }

    return FILES_TOP.buffer.data[FILES_TOP.buffer.pos];
}

char peek_ahead_char(size_t characters_ahead) {
    if (FILES_TOP.buffer.pos+characters_ahead-1 >= FILES_TOP.buffer.size) {
        return EOF;
    }


    return FILES_TOP.buffer.data[FILES_TOP.buffer.pos+characters_ahead-1];
}

char consume_escaped_char(void) {
    escaped = false;
    char consumed_char = consume_next_char();
    switch (consumed_char) {
        case 'r': return '\r';
        case 'n': return '\n';
        case 't': return '\t';
        case '"': return '"';
        case '\'': return '\'';
        case '\\': return '\\';

        default:
            error(FILES_TOP.filepath, FILES_TOP.current_line, "Unexpected escape character");
            return consumed_char;
    }
}

void consume_whitespace(void) {
    char c = peek_next_char();
    while (c == ' ' || c == '\t' || c == '\r') {
        consume_next_char();
        c = peek_next_char();
        FILES_TOP.current_pos++;
    }
}

bool match(const char expected) {
    return peek_next_char() == expected;
}

bool is_hex(const char c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

bool is_numeric(const char c) {
    return c >= '0' && c <= '9';
}

bool is_alpha(const char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z');
}

bool is_alphanumeric(const char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
            c == '_';
}

bool is_whitespace(const char c) {
    return c == ' ' || c == '\t' ||
           c == '\r';
}

// h headers can be most characters except >
bool is_h_header_char(const char c) {
    return (c >= 'a' && c <= '~') ||
           (c >= 'A' && c <= '_') ||
           (c >= '%' && c <= '=') ||
           (c >= '!' && c <= '#') ||
            c == '?';
}

// q headers can be most characters except "
bool is_q_header_char(const char c) {
    return (c >= 'a' && c <= '~') ||
           (c >= 'A' && c <= '_') ||
           (c >= '%' && c <= '?') ||
            c == '!' || c == '#'  ||
            c == '?';
}

bool not_end_of_string(const char c) {
    return escaped || (c != '"' && c != EOF);
}

bool not_end_of_char_const(const char c) {
    return escaped || (c != '\'' && c != EOF);
}

// Builds up a lexeme by consuming characters until
// a character does not satisfy the compare function
void build_lexme(bool compare_func(char), char *lexeme) {
    // TODO: Handle lexeme overflow
    char *lexeme_ptr = lexeme + strlen(lexeme);

    while (compare_func(peek_next_char())) {
        *lexeme_ptr = consume_next_char();

        if (escaped) {
            if (peek_next_char() == '\r') {consume_next_char();}
            if (peek_next_char() == '\n') {
                consume_next_char();
                continue;
            }

            char peeked_char = peek_next_char();

            // Skip \x, \0..\7, treat them as a normal character following a backslash
            if (peeked_char != 'x' && (peeked_char < '0' || peeked_char > '7')) {
                *lexeme_ptr = (char) (consume_escaped_char() | 0x80);
            }
        }

        lexeme_ptr++;
        *lexeme_ptr = '\0';
    }
}

void error(const char *filename, int line, char *message) {
    fprintf(stderr, "\033[91mError in %s on line %d: %s\033[0m\n", filename, line, message);
}

void create_punctuator_token(token* new_token, const enum subtype punctuator) {
    *new_token = (token) {.type = PUNCTUATOR, .subtype = punctuator,
                       .lexeme[0] = '\0', .line = FILES_TOP.current_line};

    strcpy(new_token->lexeme, subtype_strings[punctuator]);
}

void create_identifier_or_keyword_token(token* new_token, char c) {
    *new_token = (token) {.lexeme[0] = c, .lexeme[1] = '\0', .line = FILES_TOP.current_line};

    build_lexme(is_alphanumeric, new_token->lexeme);

    // Check for keyword
    for (enum subtype enum_num = 0; enum_num < NUM_SUBTYPES; enum_num++) {
        if (strcmp(new_token->lexeme, subtype_strings[enum_num]) == 0) {
            new_token->type = KEYWORD;
            new_token->subtype = enum_num;

            return;
        }
    }

    // If we're here, the lexeme is an identifier
    new_token->type = IDENTIFIER;
}

void create_string_literal_token(token* new_token) {
    *new_token = (token) {.type = STRING_LITERAL, .line = FILES_TOP.current_line};

    build_lexme(not_end_of_string, new_token->lexeme);

    if (peek_next_char() == EOF) {
        error(FILES_TOP.filepath, FILES_TOP.current_line, "Expected end of string");
        return;
    }

    consume_next_char(); // Consume the ending "
}

#define SUFFIX_NONE 0
#define SUFFIX_U 1
#define SUFFIX_L 2
#define SUFFIX_LL 4
#define SUFFIX_F 8

void convert_to_base_10(const size_t base, char *in_str, char *out_str) {
    size_t len = strlen(in_str);
    size_t result = 0;
    size_t digit = SIZE_MAX;
    size_t position_power = 1;

    for (size_t i = len - 1; i != SIZE_MAX; i--) {
        if (in_str[i] >= '0' && in_str[i] <= '9') digit = (size_t) in_str[i] - '0';
        if (in_str[i] >= 'a' && in_str[i] <= 'f') digit = (size_t) in_str[i] - 'a' + 10;
        if (in_str[i] >= 'A' && in_str[i] <= 'F') digit = (size_t) in_str[i] - 'A' + 10;

        if (digit == SIZE_MAX) {
            error(FILES_TOP.filepath, FILES_TOP.current_line, "Error converting to base 10: Invalid digit");
        }

        result += digit * position_power;
        position_power *= base;
    }

    out_str[0] = '\0'; // In case in_str and out_str are the same

    sprintf(out_str, "%ld", result);
}

void create_constant_token(token* new_token, const char c) {
    *new_token = (token) {.type = CONSTANT, .subtype = CONST_INTEGER,
                          .lexeme[0] = c, .lexeme[1] = '\0', .line = FILES_TOP.current_line};

    char next_char = peek_next_char();
    size_t base = 10;

    if (c == '0') {
        new_token->lexeme[0] = '\0';

        if ((next_char & 95) == 'X') {
            base = 16;
            consume_next_char();

            build_lexme(is_hex, new_token->lexeme);
        }
        else if (is_numeric(next_char)) {
            base = 8;
            build_lexme(is_numeric, new_token->lexeme);
        }
        else {
            new_token->lexeme[0] = '0';
            new_token->lexeme[1] = '\0';
        }
    } else {
        build_lexme(is_numeric, new_token->lexeme);
    }

    if (base != 10) convert_to_base_10(base, new_token->lexeme, new_token->lexeme);

    next_char = peek_next_char();

    // Floating point constant handling
    if (next_char == '.') {
        new_token->subtype = CONST_DOUBLE;
        strcat(new_token->lexeme, ".\0");
        consume_next_char();

        if (base == 10) {
            build_lexme(is_numeric, new_token->lexeme);
        } else if (base == 16) {
            build_lexme(is_hex, new_token->lexeme);
        } else {
            error(FILES_TOP.filepath, new_token->line, "Invalid base for floating constant");
            return;
        }

        next_char = peek_next_char();

        if ((next_char & 95) == 'P') {
            if (base == 10) {
                error(FILES_TOP.filepath, new_token->line, "Found binary exponent part in decimal floating constant");
                return;
            }

            consume_next_char();
            strcat(new_token->lexeme, "P");
            build_lexme(is_numeric, new_token->lexeme);
        }
    } else {
        new_token->subtype = CONST_INTEGER;
    }

    if ((next_char & 95) == 'E') {
        if (base == 16) {
            error(FILES_TOP.filepath, new_token->line, "Found exponent part in hexadecimal floating constant");
            return;
        }

        consume_next_char();
        strcat(new_token->lexeme, "E");

        next_char = peek_next_char();

        if (next_char == '+' || next_char == '-') {
            consume_next_char();
            strncat(new_token->lexeme, &next_char, 1);
        }

        build_lexme(is_numeric, new_token->lexeme);
    }

    next_char = peek_next_char();

    if (!is_alpha(next_char)) {
        return; // No suffix
    }

    char *suffix_ptr = new_token->lexeme + strlen(new_token->lexeme);

    build_lexme(is_alpha, new_token->lexeme);

    if (strlen(suffix_ptr) > 3) {
        error(FILES_TOP.filepath, new_token->line, "Unknown number suffix");
        return;
    }

    short suffix = 0;

    for (; *suffix_ptr != '\0'; suffix_ptr++) {
        // Convert to upper case
        *suffix_ptr &= 95;

        if (*suffix_ptr != 'U' && *suffix_ptr != 'L' && *suffix_ptr != 'F') {
            error(FILES_TOP.filepath, new_token->line, "Unknown number suffix");
            return;
        }

        switch (*suffix_ptr) {
            case 'U':
                if (*(suffix_ptr+1) == 'U') {
                    error(FILES_TOP.filepath, new_token->line, "Unknown number suffix");
                    return;
                }
                suffix |= SUFFIX_U;
                break;
            case 'L':
                if (*(suffix_ptr+1) == 'L') {
                    suffix |= SUFFIX_LL;
                    break;
                }
                suffix |= SUFFIX_L;
                break;
            case 'F':
                suffix |= SUFFIX_F;
        }
    }

    // TODO: Need to check the value can be represented by the suffix
    if (new_token->subtype == CONST_INTEGER) {
        switch (suffix) {
            case SUFFIX_U: new_token->subtype = CONST_UNSIGNED_INT; break;
            case SUFFIX_L: new_token->subtype = CONST_LONG; break;
            case SUFFIX_LL: new_token->subtype = CONST_LONG_LONG; break;
            case SUFFIX_U | SUFFIX_L: new_token->subtype = CONST_UNSIGNED_LONG; break;
            case SUFFIX_U | SUFFIX_LL: new_token->subtype = CONST_UNSIGNED_LONG_LONG; break;
            case SUFFIX_F: new_token->subtype = CONST_FLOAT; break;

            default: error(FILES_TOP.filepath, new_token->line, "Unknown integer suffix"); break;
        }
    }
    else if (new_token->subtype == CONST_DOUBLE) {
        switch (suffix) {
            case SUFFIX_F: new_token->subtype = CONST_FLOAT; break;
            case SUFFIX_L: new_token->subtype = CONST_LONG_DOUBLE; break;
            default: error(FILES_TOP.filepath, new_token->line, "Unknown integer suffix"); break;
        }
    }
}

void create_character_token(token* new_token, bool wide) {
    *new_token = (token) {.type = CONSTANT, .line = FILES_TOP.current_line};

    new_token->subtype = wide ? CONST_WIDE_CHAR : CONST_CHAR;

    if (wide) {consume_next_char();} // Consume the starting '

    build_lexme(not_end_of_char_const, new_token->lexeme);

    if (peek_next_char() != '\'') {
        error(FILES_TOP.filepath, new_token->line, "Expected \'");
    }

    consume_next_char(); // Consume the ending '
}

void create_directive_token(token* new_token) {
    consume_whitespace();

    *new_token = (token) {.type = DIRECTIVE, .lexeme[0] = '\0', .line = FILES_TOP.current_line};

    build_lexme(is_alpha, new_token->lexeme);

    // Check for directive
    for (enum subtype enum_num = DIRECTIVE_IF; enum_num < NUM_SUBTYPES; enum_num++) {
        if (strcmp(new_token->lexeme, subtype_strings[enum_num]) == 0) {
            new_token->subtype = enum_num;

            in_include = (new_token->subtype == DIRECTIVE_INCLUDE);
            in_define = (new_token->subtype == DIRECTIVE_DEFINE);

            return;
        }
    }

    error(FILES_TOP.filepath, FILES_TOP.current_line, "Unknown directive");
}

void create_header_token(token *new_token, header_type type) {
    consume_whitespace();

    *new_token = (token) {.type = HEADER_NAME, .lexeme[0] = '\0', .line = FILES_TOP.current_line};

    switch (type) {
        case H_HEADER:
            build_lexme(is_h_header_char, new_token->lexeme);
            consume_whitespace();
            consume_next_char(); // Consume the ending `>`
            break;
        case Q_HEADER:
            build_lexme(is_q_header_char, new_token->lexeme);
            consume_next_char(); // Consume the ending `"`
            break;
    }

    in_include = false;
}

void create_newline_token(token *new_token) {
    // FILES_TOP.current_line was incremented when the newline was read,
    // so -1 to get the actual line the newline is on
    *new_token = (token) {.type = NEWLINE, .lexeme[0] = '\n', .line = FILES_TOP.current_line-1};

    in_define = false;
}

void create_blank_token(token *new_token) {
    *new_token = (token) {.type = BLANK, .lexeme[0] = '\0', .line = FILES_TOP.current_line};
}

void create_end_token(token *new_token) {
    *new_token = (token) {.type = END, .lexeme[0] = '\0', .line = FILES_TOP.current_line};
}

token scan_token(void) {
    char *r_slash_ptr = NULL;
    bool follows_whitespace = false;
    token new_token = {0};
    new_token.line = -1;

    while (new_token.line == -1) {
        char c = consume_next_char();
        switch (c) {
            // Punctuator
            case '[': create_punctuator_token(&new_token, PUN_LEFT_SQUARE_BRACKET); break;
            case ']': create_punctuator_token(&new_token, PUN_RIGHT_SQUARE_BRACKET); break;
            case '(': create_punctuator_token(&new_token, PUN_LEFT_PARENTHESIS); break;
            case ')': create_punctuator_token(&new_token, PUN_RIGHT_PARENTHESIS); break;
            case '{': create_punctuator_token(&new_token, PUN_LEFT_BRACE); break;
            case '}': create_punctuator_token(&new_token, PUN_RIGHT_BRACE); break;
            case '.':
                switch (peek_next_char()) {
                    case '.': if (peek_ahead_char(2) == '.') {create_punctuator_token(&new_token, PUN_ELLIPSIS); consume_next_char();}
                    consume_next_char(); break;

                    default:  create_punctuator_token(&new_token, PUN_DOT); break;
                }
            break;

            case '-':
                switch (peek_next_char()) {
                    case '>': create_punctuator_token(&new_token, PUN_ARROW); consume_next_char(); break;
                    case '-': create_punctuator_token(&new_token, PUN_DECREMENT); consume_next_char(); break;
                    case '=': create_punctuator_token(&new_token, PUN_MINUS_ASSIGNMENT); consume_next_char(); break;

                    default:  create_punctuator_token(&new_token, PUN_MINUS); break;
                }
            break;

            case '&':
                switch(peek_next_char()) {
                    case '&': create_punctuator_token(&new_token, PUN_LOGICAL_AND); consume_next_char(); break;
                    case '=': create_punctuator_token(&new_token, PUN_AND_ASSIGNMENT); consume_next_char(); break;

                    default:  create_punctuator_token(&new_token, PUN_AMPERSAND); break;
                }
            break;

            case '*':
                switch (peek_next_char()) {
                    case '=': create_punctuator_token(&new_token, PUN_MULTIPLY_ASSIGNMENT);
                    consume_next_char(); break;

                    default:  create_punctuator_token(&new_token, PUN_ASTERISK); break;
                }
            break;

            case '+':
                switch (peek_next_char()) {
                    case '+': create_punctuator_token(&new_token, PUN_INCREMENT); consume_next_char(); break;
                    case '=': create_punctuator_token(&new_token, PUN_PLUS_ASSIGNMENT); consume_next_char(); break;

                    default:  create_punctuator_token(&new_token, PUN_PLUS); break;
                }
            break;

            case '~': create_punctuator_token(&new_token, PUN_TILDE); break;
            case '!':
                switch(peek_next_char()) {
                    case '=': create_punctuator_token(&new_token, PUN_INEQUALITY);
                    consume_next_char(); break;

                    default: create_punctuator_token(&new_token, PUN_EXCLAMATION_MARK); break;
                }
            break;

            case '/':
                switch (consume_next_char()) {
                    // If we're in a comment, consume characters until the next line
                    case '/': while (consume_next_char() != '\n'); create_newline_token(&new_token); break;
                    case '*': ; // Technically a variable declaration can't follow directly after "case"
                    int in_comment = 1;
                    while (in_comment) {
                        switch (consume_next_char()) {
                            case '*' : if (peek_next_char() == '/') { consume_next_char(); in_comment = 0;} break;
                            default  : break;
                        }
                    }
                    break;
                    case '=': create_punctuator_token(&new_token, PUN_DIVIDE_ASSIGNMENT); break;

                    default: create_punctuator_token(&new_token, PUN_FWD_SLASH); break;
                }
            break;

            case '%':
                switch (peek_next_char()) {
                    case '=': create_punctuator_token(&new_token, PUN_MOD_ASSIGNMENT);
                    consume_next_char(); break;

                    default:  create_punctuator_token(&new_token, PUN_REMAINDER); break;
                }
            break;

            case '<':
                switch (peek_next_char()) {
                    case '<':
                        if (peek_ahead_char(2) == '=') {
                            create_punctuator_token(&new_token, PUN_LEFT_BITSHIFT_ASSIGNMENT);
                            consume_next_char();
                        }
                        else {
                            create_punctuator_token(&new_token, PUN_LEFT_BITSHIFT);
                        }
                    consume_next_char();
                    break;

                    case '=': create_punctuator_token(&new_token, PUN_LESS_THAN_EQUAL); consume_next_char(); break;

                    default:
                        if (in_include) {
                            create_header_token(&new_token, H_HEADER);
                        } else {
                            create_punctuator_token(&new_token, PUN_LESS_THAN);
                        }

                        break;
                }
            break;

            case '>':
                switch (peek_next_char()) {
                    case '>':
                        if (peek_ahead_char(2) == '=') {
                            create_punctuator_token(&new_token, PUN_RIGHT_BITSHIFT_ASSIGNMENT);
                            consume_next_char();
                        }
                        else {
                            create_punctuator_token(&new_token, PUN_RIGHT_BITSHIFT);
                        }
                    consume_next_char();
                    break;

                    case '=': create_punctuator_token(&new_token, PUN_GREATER_THAN_EQUAL); consume_next_char(); break;

                    default:  create_punctuator_token(&new_token, PUN_GREATER_THAN); break;
                }
            break;

            case '^':
                switch(peek_next_char()) {
                    case '=': create_punctuator_token(&new_token, PUN_XOR_ASSIGNMENT);
                    consume_next_char(); break;

                    default:  create_punctuator_token(&new_token, PUN_BITWISE_XOR); break;
                }
            break;

            case '|':
                switch(peek_next_char()) {
                    case '|': create_punctuator_token(&new_token, PUN_LOGICAL_OR); consume_next_char(); break;
                    case '=': create_punctuator_token(&new_token, PUN_XOR_ASSIGNMENT); consume_next_char(); break;

                    default: create_punctuator_token(&new_token, PUN_BITWISE_OR); break;
                }
            break;

            case '?': create_punctuator_token(&new_token, PUN_QUESTION_MARK); break;
            case ':': create_punctuator_token(&new_token, PUN_COLON); break;
            case ';': create_punctuator_token(&new_token, PUN_SEMICOLON); break;
            case '=':
                switch (peek_next_char()) {
                    case '=': create_punctuator_token(&new_token, PUN_EQUALITY);
                    consume_next_char(); break;

                    default: create_punctuator_token(&new_token, PUN_ASSIGNMENT); break;
                }
            break;

            case ',': create_punctuator_token(&new_token, PUN_COMMA); break;
            case '#':
                switch (peek_next_char()) {
                    case '#': create_punctuator_token(&new_token, PUN_DOUBLE_HASH);
                    consume_next_char(); break;

                    default:
                        if (in_define) {
                            create_punctuator_token(&new_token, PUN_HASH);
                        }
                        else {
                            create_directive_token(&new_token); break;
                        }
                }
            break;


            // White space characters
            case '\n': create_newline_token(&new_token); break;
            case '\r':
            case '\t':
            case ' ' :
                while (is_whitespace(peek_next_char())) consume_next_char();
                follows_whitespace = true; break;

            // String literal
            case '"':
                if (in_include) {
                    create_header_token(&new_token, Q_HEADER); break;
                } else {
                    create_string_literal_token(&new_token); break;
                }

            // Character constant
            case '\'': create_character_token(&new_token, false); break;
            case 'L':
                if (peek_next_char() == '\'') {create_character_token(&new_token, true);}
                else {create_identifier_or_keyword_token(&new_token, c);}
                break;

            // End of file
            case EOF:
                create_end_token(&new_token);

                strcpy(new_token.src_filepath, FILES_TOP.filepath);
                r_slash_ptr = strrchr(new_token.src_filepath, '/');
                if (r_slash_ptr != NULL) {
                    new_token.filename_index = r_slash_ptr - new_token.src_filepath + 1;
                }

                free(FILES_TOP.buffer.data);
                files_top--;
                return new_token;

            // Backslash followed by new line
            case '\\':
                if (peek_next_char() == '\r') {consume_next_char();}
                if (peek_next_char() == '\n') {consume_next_char();}
                break;

            default:
                // Number
                if (is_numeric(c)) {
                    create_constant_token(&new_token, c);
                }
                // Keyword/Identifier
                else if (is_alphanumeric(c)) {
                    create_identifier_or_keyword_token(&new_token, c);
                }
                else {
                    error(FILES_TOP.filepath, FILES_TOP.current_line, "Unknown token");
                }
        }
    }

    new_token.follows_whitespace = follows_whitespace;

    strcpy(new_token.src_filepath, FILES_TOP.filepath);
    r_slash_ptr = strrchr(new_token.src_filepath, '/');
    if (r_slash_ptr != NULL) {
        new_token.filename_index = r_slash_ptr - new_token.src_filepath + 1;
    }

    return new_token;
}

void scan_and_insert_tokens(tk_node *insert_point) {
    tk_node *ptr = insert_point->next;
    while (files_top >= 0) {
        insert_point->next = allocate_from_arena(token_arena, sizeof(tk_node));
        insert_point->next->token = scan_token();
        insert_point->next->next = ptr;
        insert_point = insert_point->next;
    }
}

bool add_file(const char *file_path) {
    FILE *file_stream = fopen(file_path, "rb");
    size_t file_size;

    if (file_stream == NULL) {
        return NULL;
    }

    fseek(file_stream, 0, SEEK_END);
    file_size = (size_t) ftell(file_stream);
    rewind(file_stream);

    files[++files_top] = (file_info) {.buffer = {.size = file_size, .pos = 0},
                                                   .current_pos = 0, .current_line = 1};
    files[files_top].buffer.data = malloc(file_size);

    fread(FILES_TOP.buffer.data, 1, file_size, file_stream);
    strcpy(files[files_top].filepath, file_path);

    fclose(file_stream);

    return true;
}
