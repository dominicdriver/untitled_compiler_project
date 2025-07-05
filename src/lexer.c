#include "lexer.h"
#include "common.h"
#include "enums.h"
#include "strings.h"
#include "lexer.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

memory_arena *token_arena;
tk_node *tokens;
size_t num_tokens = 0;

file_info files[MAX_NUM_FILES];
int files_top = -1;

bool escaped;

static string newline_string = create_const_string("\n");
static string empty_string = create_const_string("");

// Maps subtype enums to their string representation
const string subtype_strings[] = {
    // Keywords
    [KW_AUTO] = create_const_string("auto"),
    [KW_BREAK] = create_const_string("break"),
    [KW_CASE] = create_const_string("case"),
    [KW_CHAR] = create_const_string("char"),
    [KW_CONST] = create_const_string("const"),
    [KW_CONTINUE] = create_const_string("continue"),
    [KW_DEFAULT] = create_const_string("default"),
    [KW_DO] = create_const_string("do"),
    [KW_DOUBLE] = create_const_string("double"),
    [KW_ELSE] = create_const_string("else"),
    [KW_ENUM] = create_const_string("enum"),
    [KW_EXTERN] = create_const_string("extern"),
    [KW_FLOAT] = create_const_string("float"),
    [KW_FOR] = create_const_string("for"),
    [KW_GOTO] = create_const_string("goto"),
    [KW_IF] = create_const_string("if"),
    [KW_INLINE] = create_const_string("inline"),
    [KW_INT] = create_const_string("int"),
    [KW_LONG] = create_const_string("long"),
    [KW_REGISTER] = create_const_string("register"),
    [KW_RESTRICT] = create_const_string("restrict"),
    [KW_RETURN] = create_const_string("return"),
    [KW_SHORT] = create_const_string("short"),
    [KW_SIGNED] = create_const_string("signed"),
    [KW_SIZEOF] = create_const_string("sizeof"),
    [KW_STATIC] = create_const_string("static"),
    [KW_STRUCT] = create_const_string("struct"),
    [KW_SWITCH] = create_const_string("switch"),
    [KW_TYPEDEF] = create_const_string("typedef"),
    [KW_UNION] = create_const_string("union"),
    [KW_UNSIGNED] = create_const_string("unsigned"),
    [KW_VOID] = create_const_string("void"),
    [KW_VOLATILE] = create_const_string("volatile"),
    [KW_WHILE] = create_const_string("while"),
    [KW__BOOL] = create_const_string("_Bool"),
    [KW__COMPLEX] = create_const_string("_Complex"),
    [KW__IMAGINARY] = create_const_string("_Imaginary"),

    // Punctuators
    [PUN_LEFT_SQUARE_BRACKET] = create_const_string("["),
    [PUN_RIGHT_SQUARE_BRACKET] = create_const_string("]"),
    [PUN_LEFT_PARENTHESIS] = create_const_string("("),
    [PUN_RIGHT_PARENTHESIS] = create_const_string(")"),
    [PUN_LEFT_BRACE] = create_const_string("{"),
    [PUN_RIGHT_BRACE] = create_const_string("}"),
    [PUN_DOT] = create_const_string("."),
    [PUN_ARROW] = create_const_string("->"),
    [PUN_INCREMENT] = create_const_string("++"),
    [PUN_DECREMENT] = create_const_string("--"),
    [PUN_AMPERSAND] = create_const_string("&"),
    [PUN_ASTERISK] = create_const_string("*"),
    [PUN_PLUS] = create_const_string("+"),
    [PUN_MINUS] = create_const_string("-"),
    [PUN_TILDE] = create_const_string("~"),
    [PUN_EXCLAMATION_MARK] = create_const_string("!"),
    [PUN_FWD_SLASH] = create_const_string("/"),
    [PUN_REMAINDER] = create_const_string("%"),
    [PUN_LEFT_BITSHIFT] = create_const_string("<<"),
    [PUN_RIGHT_BITSHIFT] = create_const_string(">>"),
    [PUN_LESS_THAN] = create_const_string("<"),
    [PUN_GREATER_THAN] = create_const_string(">"),
    [PUN_LESS_THAN_EQUAL] = create_const_string("<="),
    [PUN_GREATER_THAN_EQUAL] = create_const_string(">="),
    [PUN_EQUALITY] = create_const_string("=="),
    [PUN_INEQUALITY] = create_const_string("!="),
    [PUN_BITWISE_XOR] = create_const_string("^"),
    [PUN_BITWISE_OR] = create_const_string("|"),
    [PUN_LOGICAL_AND] = create_const_string("&&"),
    [PUN_LOGICAL_OR] = create_const_string("||"),
    [PUN_QUESTION_MARK] = create_const_string("?"),
    [PUN_COLON] = create_const_string(":"),
    [PUN_SEMICOLON] = create_const_string(";"),
    [PUN_ELLIPSIS] = create_const_string("..."),
    [PUN_ASSIGNMENT] = create_const_string("="),
    [PUN_MULTIPLY_ASSIGNMENT] = create_const_string("*="),
    [PUN_DIVIDE_ASSIGNMENT] = create_const_string("/="),
    [PUN_MOD_ASSIGNMENT] = create_const_string("%="),
    [PUN_PLUS_ASSIGNMENT] = create_const_string("+="),
    [PUN_MINUS_ASSIGNMENT] = create_const_string("-="),
    [PUN_LEFT_BITSHIFT_ASSIGNMENT] = create_const_string("<<="),
    [PUN_RIGHT_BITSHIFT_ASSIGNMENT] = create_const_string(">>="),
    [PUN_AND_ASSIGNMENT] = create_const_string("&="),
    [PUN_XOR_ASSIGNMENT] = create_const_string("^="),
    [PUN_OR_ASSIGNMENT] = create_const_string("|="),
    [PUN_COMMA] = create_const_string(","),
    [PUN_HASH] = create_const_string("#"),
    [PUN_DOUBLE_HASH] = create_const_string("##"),
    [PUN_NONE] = create_const_string(""),

    [CONST_INTEGER] = create_const_string(""),
    [CONST_UNSIGNED_INT] = create_const_string(""),
    [CONST_LONG] = create_const_string(""),
    [CONST_UNSIGNED_LONG] = create_const_string(""),
    [CONST_LONG_LONG] = create_const_string(""),
    [CONST_UNSIGNED_LONG_LONG] = create_const_string(""),
    [CONST_FLOAT] = create_const_string(""),
    [CONST_DOUBLE] = create_const_string(""),
    [CONST_LONG_DOUBLE] = create_const_string(""),
    [CONST_CHAR] = create_const_string(""),
    [CONST_WIDE_CHAR] = create_const_string(""),

    // Directives
    [DIRECTIVE_IF] = create_const_string("if"),
    [DIRECTIVE_IFDEF] = create_const_string("ifdef"),
    [DIRECTIVE_IFNDEF] = create_const_string("ifndef"),
    [DIRECTIVE_ELIF] = create_const_string("elif"),
    [DIRECTIVE_ELSE] = create_const_string("else"),
    [DIRECTIVE_ENDIF] = create_const_string("endif"),
    [DIRECTIVE_INCLUDE] = create_const_string("include"),
    [DIRECTIVE_DEFINE] = create_const_string("define"),
    [DIRECTIVE_UNDEF] = create_const_string("undef"),
    [DIRECTIVE_LINE] = create_const_string("line"),
    [DIRECTIVE_WARNING] = create_const_string("warning"),
    [DIRECTIVE_ERROR] = create_const_string("error"),
    [DIRECTIVE_PRAGMA] = create_const_string("pragma"),
    [DIRECTIVE_NULL] = create_const_string("")
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
        if (escaped) error(&FILES_TOP.filepath, FILES_TOP.current_line, "Lone \\");
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
            error(&FILES_TOP.filepath, FILES_TOP.current_line, "Unexpected escape character");
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
void build_lexme(bool compare_func(char), string *lexeme, bool allocate) {
    string *str = allocate ? &create_local_string("", MAX_LEXEME_LENGTH) : lexeme;
    char *str_data_ptr = &str->data[str->len];
    char overflow_char;

    while (compare_func(peek_next_char())) {
        *str_data_ptr = consume_next_char();

        if (escaped) {
            if (peek_next_char() == '\r') {consume_next_char();}
            if (peek_next_char() == '\n') {
                consume_next_char();
                continue;
            }

            char peeked_char = peek_next_char();

            // Skip \x, \0..\7, treat them as a normal character following a backslash
            if (peeked_char != 'x' && (peeked_char < '0' || peeked_char > '7')) {
                *str_data_ptr = (char) (consume_escaped_char() | 0x80);
            }
        }

        if (str->len == str->cap - 1) {
            str_data_ptr = &overflow_char;
        } else {
            str_data_ptr++;
            str->len++;
        }
    }
    *str_data_ptr = '\0';

    if (allocate) {
        *lexeme = create_heap_string(str->len+1, token_arena);
        string_copy(lexeme, str);
    }
}

void create_punctuator_token(token* new_token, const enum subtype punctuator) {
    *new_token = (token) {.type = PUNCTUATOR, .subtype = punctuator,
                       .lexeme = subtype_strings[punctuator], .line = FILES_TOP.current_line};
}

void create_identifier_or_keyword_token(token* new_token, char c) {
    *new_token = (token) {.lexeme = {0}, .line = FILES_TOP.current_line};

    string tmp_str = create_local_string(c, MAX_LEXEME_LENGTH);
    tmp_str.len++;

    build_lexme(is_alphanumeric, &tmp_str, false);

    // Check for keyword
    for (enum subtype enum_num = 0; enum_num < NUM_SUBTYPES; enum_num++) {
        if (string_cmp(&tmp_str, &subtype_strings[enum_num]) == 0) {
            new_token->type = KEYWORD;
            new_token->lexeme = subtype_strings[enum_num];
            new_token->subtype = enum_num;

            return;
        }
    }

    // If we're here, the lexeme is an identifier
    new_token->type = IDENTIFIER;
    new_token->lexeme = create_heap_string(tmp_str.len+1, token_arena);
    string_copy(&new_token->lexeme, &tmp_str);
}

void create_string_literal_token(token* new_token) {
    *new_token = (token) {.type = STRING_LITERAL, .line = FILES_TOP.current_line};

    build_lexme(not_end_of_string, &new_token->lexeme, true);

    if (peek_next_char() == EOF) {
        error(&FILES_TOP.filepath, FILES_TOP.current_line, "Expected end of string");
        return;
    }

    consume_next_char(); // Consume the ending "
}

#define SUFFIX_NONE 0
#define SUFFIX_U 1
#define SUFFIX_L 2
#define SUFFIX_LL 4
#define SUFFIX_F 8

void convert_to_base_10(const size_t base, string *in_str, string *out_str) {
    size_t result = 0;
    size_t digit = SIZE_MAX;
    size_t position_power = 1;

    for (size_t i = in_str->len - 1; i != SIZE_MAX; i--) {
        if (in_str->data[i] >= '0' && in_str->data[i] <= '9') digit = (size_t) in_str->data[i] - '0';
        if (in_str->data[i] >= 'a' && in_str->data[i] <= 'f') digit = (size_t) in_str->data[i] - 'a' + 10;
        if (in_str->data[i] >= 'A' && in_str->data[i] <= 'F') digit = (size_t) in_str->data[i] - 'A' + 10;

        if (digit == SIZE_MAX) {
            error(&FILES_TOP.filepath, FILES_TOP.current_line, "Error converting to base 10: Invalid digit");
        }

        result += digit * position_power;
        position_power *= base;
    }

    out_str->data[0] = '\0'; // In case in_str and out_str are the same
    out_str->len = (uint16_t) sprintf(out_str->data, "%ld", result);
}

void create_constant_token(token* new_token, const char c) {
    *new_token = (token) {.type = CONSTANT, .subtype = CONST_INTEGER,
                          .lexeme = {0}, .line = FILES_TOP.current_line};

    char next_char = peek_next_char();
    size_t base = 10;

    string tmp_str = create_local_string(c, MAX_LEXEME_LENGTH);
    tmp_str.len = 1;

    if (c == '0') {
        tmp_str.data[0] = 0;
        tmp_str.len = 0;

        if ((next_char & 95) == 'X') {
            base = 16;
            consume_next_char();

            build_lexme(is_hex, &tmp_str, false);
        }
        else if (is_numeric(next_char)) {
            base = 8;
            build_lexme(is_numeric, &tmp_str, false);
        }
        else {
            tmp_str.data[0] = '0';
            tmp_str.len = 1;
        }
    } else {
        build_lexme(is_numeric, &tmp_str, false);
    }

    if (base != 10) convert_to_base_10(base, &tmp_str, &tmp_str);

    next_char = peek_next_char();

    // Floating point constant handling
    if (next_char == '.') {
        new_token->subtype = CONST_DOUBLE;
        string_cat_c(&tmp_str, '.');
        consume_next_char();

        if (base == 10) {
            build_lexme(is_numeric, &tmp_str, false);
        } else if (base == 16) {
            build_lexme(is_hex, &tmp_str, false);
        } else {
            error(&FILES_TOP.filepath, new_token->line, "Invalid base for floating constant");
            return;
        }

        next_char = peek_next_char();

        if ((next_char & 95) == 'P') {
            if (base == 10) {
                error(&FILES_TOP.filepath, new_token->line, "Found binary exponent part in decimal floating constant");
                return;
            }

            consume_next_char();
            string_cat_c(&tmp_str, 'P');
            build_lexme(is_numeric, &tmp_str, false);
        }
    } else {
        new_token->subtype = CONST_INTEGER;
    }

    if ((next_char & 95) == 'E') {
        if (base == 16) {
            error(&FILES_TOP.filepath, new_token->line, "Found exponent part in hexadecimal floating constant");
            return;
        }

        consume_next_char();
        string_cat_c(&tmp_str, 'E');

        next_char = peek_next_char();

        if (next_char == '+' || next_char == '-') {
            consume_next_char();
            string_cat_c(&tmp_str, next_char);
        }

        build_lexme(is_numeric, &tmp_str, false);
    }

    next_char = peek_next_char();

    if (!is_alpha(next_char)) {
        new_token->lexeme = create_heap_string(tmp_str.len+1, token_arena);
        string_copy(&new_token->lexeme, &tmp_str);
        return; // No suffix
    }

    build_lexme(is_alpha, &tmp_str, false);

    char *suffix_ptr = &tmp_str.data[tmp_str.len-1];

    new_token->lexeme = create_heap_string(tmp_str.len+1, token_arena);
    string_copy(&new_token->lexeme, &tmp_str);

    if (strlen(suffix_ptr) > 3) {
        error(&FILES_TOP.filepath, new_token->line, "Unknown number suffix");
        return;
    }

    short suffix = 0;

    for (; *suffix_ptr != '\0'; suffix_ptr++) {
        // Convert to upper case
        *suffix_ptr &= 95;

        if (*suffix_ptr != 'U' && *suffix_ptr != 'L' && *suffix_ptr != 'F') {
            error(&FILES_TOP.filepath, new_token->line, "Unknown number suffix");
            return;
        }

        switch (*suffix_ptr) {
            case 'U':
                if (*(suffix_ptr+1) == 'U') {
                    error(&FILES_TOP.filepath, new_token->line, "Unknown number suffix");
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

            default: error(&FILES_TOP.filepath, new_token->line, "Unknown integer suffix"); break;
        }
    }
    else if (new_token->subtype == CONST_DOUBLE) {
        switch (suffix) {
            case SUFFIX_F: new_token->subtype = CONST_FLOAT; break;
            case SUFFIX_L: new_token->subtype = CONST_LONG_DOUBLE; break;
            default: error(&FILES_TOP.filepath, new_token->line, "Unknown integer suffix"); break;
        }
    }
}

void create_character_token(token* new_token, bool wide) {
    *new_token = (token) {.type = CONSTANT, .line = FILES_TOP.current_line};

    new_token->subtype = wide ? CONST_WIDE_CHAR : CONST_CHAR;

    if (wide) {consume_next_char();} // Consume the starting '

    build_lexme(not_end_of_char_const, &new_token->lexeme, true);

    if (peek_next_char() != '\'') {
        error(&FILES_TOP.filepath, new_token->line, "Expected \'");
    }

    consume_next_char(); // Consume the ending '
}

void create_directive_token(token* new_token) {
    consume_whitespace();

    *new_token = (token) {.type = DIRECTIVE, .lexeme = {0}, .line = FILES_TOP.current_line};

    build_lexme(is_alpha, &new_token->lexeme, true);

    // Check for directive
    for (enum subtype enum_num = DIRECTIVE_IF; enum_num < NUM_SUBTYPES; enum_num++) {
        if (string_cmp(&new_token->lexeme, &subtype_strings[enum_num]) == 0) {
            new_token->subtype = enum_num;

            in_include = (new_token->subtype == DIRECTIVE_INCLUDE);
            in_define = (new_token->subtype == DIRECTIVE_DEFINE);

            return;
        }
    }

    error(&FILES_TOP.filepath, FILES_TOP.current_line, "Unknown directive");
}

void create_header_token(token *new_token, header_type type) {
    consume_whitespace();

    *new_token = (token) {.type = HEADER_NAME, .lexeme = {0}, .line = FILES_TOP.current_line};

    switch (type) {
        case H_HEADER:
            build_lexme(is_h_header_char, &new_token->lexeme, true);
            consume_whitespace();
            consume_next_char(); // Consume the ending `>`

            new_token->subtype = HEADER_H;
            break;
        case Q_HEADER:
            build_lexme(is_q_header_char, &new_token->lexeme, true);
            consume_next_char(); // Consume the ending `"`

            new_token->subtype = HEADER_Q;
            break;
    }

    in_include = false;
}

void create_newline_token(token *new_token) {
    // FILES_TOP.current_line was incremented when the newline was read,
    // so -1 to get the actual line the newline is on
    *new_token = (token) {.type = NEWLINE, .lexeme = newline_string, .line = FILES_TOP.current_line-1};

    in_define = false;
}

void create_blank_token(token *new_token) {
    *new_token = (token) {.type = BLANK, .lexeme = empty_string, .line = FILES_TOP.current_line};
}

void create_end_token(token *new_token) {
    *new_token = (token) {.type = END, .lexeme = empty_string, .line = FILES_TOP.current_line};
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
                switch (peek_next_char()) {
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
                    case '=': create_punctuator_token(&new_token, PUN_DIVIDE_ASSIGNMENT); consume_next_char(); break;

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
                new_token.src_filepath = create_heap_string(MAX_FILEPATH_LENGTH, token_arena);

                string_copy(&new_token.src_filepath, &FILES_TOP.filepath);
                r_slash_ptr = string_rstr(&new_token.src_filepath, '/').data;
                if (r_slash_ptr != NULL) {
                    new_token.filename_index = (uint16_t) (r_slash_ptr - new_token.src_filepath.data + 1);
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
                    error(&FILES_TOP.filepath, FILES_TOP.current_line, "Unknown token");
                }
        }
    }

    new_token.follows_whitespace = follows_whitespace;
    new_token.src_filepath = create_heap_string(FILES_TOP.filepath.len+1, token_arena);

    string_copy(&new_token.src_filepath, &FILES_TOP.filepath);
    r_slash_ptr = string_rstr(&new_token.src_filepath, '/').data;
    if (r_slash_ptr != NULL) {
        new_token.filename_index = (uint16_t) (r_slash_ptr - new_token.src_filepath.data + 1);
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

bool add_file(const string *file_path) {
    char file_path_cstr[MAX_LEXEME_LENGTH];

    strcpy(file_path_cstr, file_path->data);
    file_path_cstr[file_path->len] = 0;

    FILE *file_stream = fopen(file_path_cstr, "rb");
    size_t file_size;

    if (file_stream == NULL) {
        return NULL;
    }

    fseek(file_stream, 0, SEEK_END);
    file_size = (size_t) ftell(file_stream);
    rewind(file_stream);

    files[++files_top] = (file_info) {.buffer = {.size = file_size, .pos = 0},
                                      .filepath = {.cap = file_path->cap, file_path->len},
                                      .current_pos = 0, .current_line = 1};

    files[files_top].buffer.data = malloc(file_size);
    files[files_top].filepath.data = malloc(file_path->cap);

    fread(FILES_TOP.buffer.data, 1, file_size, file_stream);
    string_copy(&files[files_top].filepath, file_path);

    fclose(file_stream);

    return true;
}
