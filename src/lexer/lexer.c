// Making a few assumption to get a proof of concept:
// - Only a single source file exists
// - The lexer is responsible for opening and reading from the file

#include "lexer.h"

#include <stdio.h>
#include <string.h>

token tokens[128]; // TODO: Fixed size for now
int num_tokens = 0;

FILE *source_file;
int current_pos;
int current_line;

// Maps keyword enums to their string representation
char *keywords_strings[] = {
    [AUTO] = "auto",
    [BREAK] = "break",
    [CASE] = "case",
    [CHAR] = "char",
    [CONST] = "const",
    [CONTINUE] = "continue",
    [DEFAULT] = "default",
    [DO] = "do",
    [DOUBLE] = "double",
    [ELSE] = "else",
    [ENUM] = "enum",
    [EXTERN] = "extern",
    [FLOAT] = "float",
    [FOR] = "for",
    [GOTO] = "goto",
    [IF] = "if",
    [INLINE] = "inline",
    [INT] = "int",
    [LONG] = "long",
    [REGISTER] = "register",
    [RESTRICT] = "restrict",
    [RETURN] = "return",
    [SHORT] = "short",
    [SIGNED] = "signed",
    [SIZEOF] = "sizeof",
    [STATIC] = "static",
    [STRUCT] = "struct",
    [SWITCH] = "switch",
    [TYPEDEF] = "typedef",
    [UNION] = "union",
    [UNSIGNED] = "unsigned",
    [VOID] = "void",
    [VOLATILE] = "volatile",
    [WHILE] = "while",
    [_BOOL] = "_Bool",
    [_COMPLEX] = "_Complex",
    [_IMAGINARY] = "_Imaginary"
};

// Maps punctuator enums to their string representations
char *punctuator_strings[] = {
    [LEFT_SQUARE_BRACKET] = "[",
    [RIGHT_SQUARE_BRACKET] = "]",
    [LEFT_PARENTHESIS] = "(",
    [RIGHT_PARENTHESIS] = ")",
    [LEFT_BRACE] = "{",
    [RIGHT_BRACE] = "}",
    [DOT] = ".",
    [ARROW] = "->",
    [INCREMENT] = "++",
    [DECREMENT] = "--",
    [AMPERSAND] = "&",
    [ASTERISK] = "*",
    [PLUS] = "+",
    [MINUS] = "-",
    [TILDE] = "~",
    [EXCLIMATION_MARK] = "!",
    [FWD_SLASH] = "/",
    [REMAINDER] = "%",
    [LEFT_BITSHIFT] = "<<",
    [RIGHT_BITSHIFT] = ">>",
    [LESS_THAN] = "<",
    [GREATER_THAN] = ">",
    [LESS_THAN_EQUAL] = "<=",
    [GREATER_THAN_EQUAL] = ">=",
    [EQUALITY] = "==",
    [INEQUALITY] = "!=",
    [BITWISE_XOR] = "^",
    [BITWISE_OR] = "|",
    [LOGICAL_AND] = "&&",
    [LOGICAL_OR] = "||",
    [QUESTION_MARK] = "?",
    [COLON] = ":",
    [SEMICOLON] = ";",
    [ELLIPSIS] = "...",
    [ASSIGNMENT] = "=",
    [MULTIPLY_ASSIGNMENT] = "*=",
    [DIVIDE_ASSIGNMENT] = "/=",
    [MOD_ASSIGNMENT] = "%=",
    [PLUS_ASSIGNMENT] = "+=",
    [MINUS_ASSIGNMENT] = "-=",
    [LEFT_BITSHIFT_ASSIGNMENT] = "<<=",
    [RIGHT_BITSHIFT_ASSIGNMENT] = ">>=",
    [AND_ASSIGNMENT] = "&=",
    [XOR_ASSIGNMENT] = "^=",
    [OR_ASSIGNMENT] = "|=",
    [COMMA] = ",",
    [HASH] = "#",
    [DOUBLE_HASH] = "##",
};



#define NUM_KEYWORDS sizeof(keywords_strings) / sizeof(keywords_strings[0])

char consume_next_char() {
    return fgetc(source_file);
}


char peek_next_char() {
    char next_char = fgetc(source_file);
    fseek(source_file, -1, SEEK_CUR);
    return next_char;
}

char peek_ahead_char(int characters_ahead) {
    int chars_read = 0;
    char ahead_char = 0;

    for (int i = 0; i < characters_ahead; i++) {
        ahead_char = fgetc(source_file);
        if (ahead_char == EOF) {
            break;
        }
        chars_read++;
    }

    fseek(source_file, -chars_read, SEEK_CUR);

    return ahead_char;
}

_Bool match(const char expected) {
    return peek_next_char() == expected;
}

_Bool is_numeric (const char c) {
    return c >= '0' && c <= '9';
}

_Bool is_alphanumeric(const char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
            c == '_';
}

_Bool not_end_of_string(const char c) {
    return c != '"' && c != EOF;
}

/// Builds up a lexeme by consuming characters until
/// a character does not satisfy the compare function
char build_lexme(_Bool compare_func(char), char *lexeme) {
    char current_char;
    // TODO: Handle lexeme overflow
    while (compare_func(peek_next_char())) {
        current_char = consume_next_char();
        strncat(lexeme, &current_char, 1);
    }

    return current_char;
}

void error(int line_num, char *message) {
    fprintf(stderr, "Error on line %d: %s\n", line_num, message);
}

void add_punctuator_token(const enum punctuator punctuator) {
    token new_token = {.type = PUNCTUATOR, .punctuator = punctuator,
                       .lexeme[0] = '\0', .line = current_line};

    strcpy(new_token.lexeme, punctuator_strings[punctuator]);

    tokens[num_tokens++] = new_token;
}

void add_identifier_or_keyword_token(char c) {
    token new_token = {.lexeme[0] = c, .lexeme[1] = '\0', .line = current_line};

    build_lexme(is_alphanumeric, new_token.lexeme);

    // Check for keyword
    for (size_t enum_num = 0; enum_num < NUM_KEYWORDS; enum_num++) {
        if (strcmp(new_token.lexeme, keywords_strings[enum_num]) == 0) {
            new_token.type = KEYWORD;
            new_token.keyword = enum_num;

            tokens[num_tokens++] = new_token;
            return;
        }
    }

    // If we're here, the lexeme is an identifier
    new_token.type = IDENTIFER;
    tokens[num_tokens++] = new_token;
}

void add_string_literal_token() {
    token new_token = {.type = STRING_LITERAL, .line = current_line};

    build_lexme(not_end_of_string, new_token.lexeme);

    if (peek_next_char() == EOF) {
        error(current_line, "Expected end of string");
        return;
    }

    consume_next_char(); // Consume the ending "

    tokens[num_tokens++] = new_token;
}

void add_constant_token(const char c) {
    token new_token = {.type = CONSTANT, .lexeme[0] = c, .lexeme[1] = '\0', .line = current_line};
    // TODO: Differentiate between constants (only integers are handled for now)

    build_lexme(is_numeric, new_token.lexeme);

    new_token.constant = INTEGER;
    tokens[num_tokens++] = new_token;
}

int scan_token() {
    char c = consume_next_char();

    switch (c) {
        // Punctuator
        case '[': add_punctuator_token(LEFT_SQUARE_BRACKET); break;
        case ']': add_punctuator_token(RIGHT_SQUARE_BRACKET); break;
        case '(': add_punctuator_token(LEFT_PARENTHESIS); break;
        case ')': add_punctuator_token(RIGHT_PARENTHESIS); break;
        case '{': add_punctuator_token(LEFT_BRACE); break;
        case '}': add_punctuator_token(RIGHT_BRACE); break;
        case '.':
            switch (peek_next_char()) {
                case '.': if (peek_ahead_char(2) == '.') {add_punctuator_token(ELLIPSIS); consume_next_char();}
                consume_next_char(); break;

                default:  add_punctuator_token(DOT); break;
            }
            break;

        case '-':
            switch (peek_next_char()) {
                case '>': add_punctuator_token(ARROW); consume_next_char(); break;
                case '-': add_punctuator_token(DECREMENT); consume_next_char(); break;
                case '=': add_punctuator_token(MINUS_ASSIGNMENT); consume_next_char(); break;

                default:  add_punctuator_token(MINUS); break;
            }
            break;

        case '&':
            switch(peek_next_char()) {
                case '&': add_punctuator_token(LOGICAL_AND); consume_next_char(); break;
                case '=': add_punctuator_token(AND_ASSIGNMENT); consume_next_char(); break;

                default:  add_punctuator_token(AMPERSAND); break;
            }
            break;

        case '*':
            switch (peek_next_char()) {
                case '=': add_punctuator_token(MULTIPLY_ASSIGNMENT);
                consume_next_char(); break;

                default:  add_punctuator_token(ASTERISK); break;
            }
            break;

        case '+':
            switch (peek_next_char()) {
                case '+': add_punctuator_token(INCREMENT); consume_next_char(); break;
                case '=': add_punctuator_token(PLUS_ASSIGNMENT); consume_next_char(); break;

                default:  add_punctuator_token(PLUS); break;
            }
            break;

        case '~': add_punctuator_token(TILDE); break;
        case '!':
            switch(peek_next_char()) {
                case '=': add_punctuator_token(INEQUALITY);
                consume_next_char(); break;

                default: add_punctuator_token(EXCLIMATION_MARK); break;
            }
            break;

        case '/':
            switch (peek_next_char()) {
                // If we're in a comment, consume characters until the next line
                case '/': while (consume_next_char() != '\n'); current_line++; break;
                case '=': add_punctuator_token(DIVIDE_ASSIGNMENT);
                consume_next_char(); break;

                default: add_punctuator_token(FWD_SLASH); break;
            }
            break;

        case '%':
            switch (peek_next_char()) {
                case '=': add_punctuator_token(MOD_ASSIGNMENT);
                consume_next_char(); break;

                default:  add_punctuator_token(REMAINDER); break;
            }
            break;

        case '<':
            switch (peek_next_char()) {
                case '<':
                    if (peek_ahead_char(2) == '=') {
                        add_punctuator_token(LEFT_BITSHIFT_ASSIGNMENT);
                        consume_next_char();
                    }
                    else {
                        add_punctuator_token(LEFT_BITSHIFT);
                    }
                    consume_next_char();
                    break;

                case '=': add_punctuator_token(LESS_THAN_EQUAL); consume_next_char(); break;

                default:  add_punctuator_token(LESS_THAN); break;
            }
            break;

        case '>':
            switch (peek_next_char()) {
                case '>':
                    if (peek_ahead_char(2) == '=') {
                        add_punctuator_token(RIGHT_BITSHIFT_ASSIGNMENT);
                        consume_next_char();
                    }
                    else {
                        add_punctuator_token(RIGHT_BITSHIFT);
                    }
                    consume_next_char();
                    break;

                case '=': add_punctuator_token(GREATER_THAN_EQUAL); consume_next_char(); break;

                default:  add_punctuator_token(GREATER_THAN); break;
            }
            break;

        case '^':
            switch(peek_next_char()) {
                case '=': add_punctuator_token(XOR_ASSIGNMENT);
                consume_next_char(); break;

                default:  add_punctuator_token(BITWISE_XOR); break;
            }
            break;

        case '|':
            switch(peek_next_char()) {
                case '|': add_punctuator_token(LOGICAL_OR); consume_next_char(); break;
                case '=': add_punctuator_token(XOR_ASSIGNMENT); consume_next_char(); break;

                default: add_punctuator_token(BITWISE_OR); break;
            }
            break;

        case '?': add_punctuator_token(QUESTION_MARK); break;
        case ':': add_punctuator_token(COLON); break;
        case ';': add_punctuator_token(SEMICOLON); break;
        case '=':
            switch (peek_next_char()) {
                case '=': add_punctuator_token(EQUALITY);
                consume_next_char(); break;

                default: add_punctuator_token(ASSIGNMENT); break;
            }
            break;

        case ',': add_punctuator_token(COMMA); break;
        case '#':
            switch (peek_next_char()) {
                case '#': add_punctuator_token(DOUBLE_HASH);
                consume_next_char(); break;

                default: add_punctuator_token(HASH); break;
            }
            break;


        // White space characters
        case '\n': current_line++; break;
        case '\r': break; // '\r' should be followed by '\n'
        case '\t': break; // Ignore tabs
        case ' ': break; // Ignore spaces

        // String literal
        case '"': add_string_literal_token(); break;

        // End of file
        case EOF: return EOF;

        default:
            // Number
            if (is_numeric(c)) {
                add_constant_token(c);
            }
            // Keyword/Identifier
            else if (is_alphanumeric(c)) {
                add_identifier_or_keyword_token(c);
            }
            else {
                error(current_line, "Unknown token");
            }
    }

    return c;
}

int scan_tokens() {
    while (scan_token() != EOF);
    return 0;
}

int init_lexer(const char *file_path) {
    current_pos = 0;
    current_line = 1;

    source_file = fopen(file_path, "rb");

    if (source_file == NULL) {
        return 1;
    }

    return 0;
}

void debug_print_tokens() {
    printf("Tokens:\n");
    for (int i = 0; i < num_tokens; i++) {
        char token_type[16];
        switch (tokens[i].type) {
            case KEYWORD: strcpy(token_type, "Keyword"); break;
            case IDENTIFER: strcpy(token_type, "Identifier"); break;
            case PUNCTUATOR: strcpy(token_type, "Punctuation"); break;
            case CONSTANT: strcpy(token_type, "Constant"); break;
            case STRING_LITERAL: strcpy(token_type, "String"); break;
        }

        printf("%s: %s\n", token_type, tokens[i].lexeme);
    }
}

// TODO: Decide how the Lexer will be called
int main(void) {
    init_lexer("tests/hello_world.c");
    scan_tokens();
}
