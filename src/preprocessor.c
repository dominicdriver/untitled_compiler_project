#include "enums.h"
#include "common.h"
#include "preprocessor.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "memory.h"
#include "helper_functions.h"

bool in_define = 0;
bool in_include = 0;

macro macros[MAX_NUM_MACROS];
size_t num_macros = 0;
size_t max_macros = 0;

int operator_precedence[DIRECTIVE_NULL] = {
    [PUN_ASTERISK] = 1, [PUN_FWD_SLASH] = 1,
    [PUN_EXCLAMATION_MARK] = 2,
    [PUN_LEFT_PARENTHESIS] = -10, [PUN_RIGHT_PARENTHESIS] = -10,
    [PUN_LOGICAL_AND] = -1, [PUN_LOGICAL_OR] = -2,
    [PUN_QUESTION_MARK] = -5,
    0
};

const char *current_src_file;
int current_line;

void initialise_macros(void) {
    macros[num_macros++] = (macro) {.name = "__x86_64__", .num_params = 0, .hash = hash("__x86_64__")};
    macros[num_macros++] = (macro) {.name = "__LP64__", .num_params = 0, .hash = hash("__LP64__")};
}

bool macros_equal(const macro *macro_one, const macro *macro_two) {
    bool macros_equal = true;

    const size_t replacement_length = sizeof(macro_one->replacement)/sizeof(macro_one->replacement[0]);
    const size_t parameter_length = sizeof(macro_one->parameters)/sizeof(macro_one->parameters[0]);

    macros_equal &= (strcmp(macro_one->name, macro_two->name) == 0);
    macros_equal &= (macro_one->is_function_like == macro_two->is_function_like);

    for (size_t i = 0; i < replacement_length; i++) {
        macros_equal &= (strcmp(macro_one->replacement[i].lexeme, macro_two->replacement[i].lexeme) == 0);
        macros_equal &= (macro_one->replacement[i].type == macro_two->replacement[i].type);
        macros_equal &= (macro_one->replacement[i].subtype == macro_two->replacement[i].subtype);
    }

    macros_equal &= (macro_one->num_params == macro_two->num_params);

    if (macro_one->is_function_like) {
        for (size_t i = 0; i < parameter_length; i++) {
            macros_equal &= (strcmp(macro_one->parameters[i], macro_two->parameters[i]) == 0);
        }
    }

    return macros_equal;
}

void add_macro(const macro new_macro) {
    for (size_t i = 0; i < num_macros; i++) {
        if (new_macro.hash == macros[i].hash && strcmp(new_macro.name, macros[i].name) == 0) {
            if (!macros_equal(&new_macro, &macros[i])) {
                error(current_src_file, current_line, "Duplicate macro");
            }
            return;
        }
    }
    macros[num_macros++] = new_macro;

    if (num_macros > max_macros)
    {
        max_macros = num_macros;
    }
}

void remove_macro(const char* macro_name) {
    uint64_t macro_name_hash = hash(macro_name);
    for (size_t i = 0; i < num_macros; i++) {
        if (macros[i].hash == macro_name_hash && strcmp(macros[i].name, macro_name) == 0) {
            macros[i] = (macro) {0};

            // Shift all elements left to remove the gap
            memmove(&macros[i], &macros[i+1], (num_macros-i-1) * sizeof(macro));

            num_macros--;
            break;
        }
    }
}

macro *macro_exists(tk_node *token_node) {
    char *identifier = token_node->token.lexeme;
    tk_node *next_tk_node = token_node->next;

    uint64_t identifier_hash = hash(identifier);
    bool function_like = (next_tk_node != NULL && next_tk_node->token.subtype == PUN_LEFT_PARENTHESIS);

    // func_like | macro_func_like | c
    // 0         | 0               | 1
    // 0         | 1               | 0
    // 1         | 0               | 1
    // 1         | 1               | 1

    for (size_t i = 0; i < num_macros; i++) {
        if (identifier_hash == macros[i].hash) {
        if (strcmp(identifier, macros[i].name) == 0 && (function_like | !macros[i].is_function_like)) {
            return &macros[i];
        }
        }
    }

    return NULL;
}

void handle_include_directive(tk_node *token_node) {
    // token_node points to the include token

    // TODO: Use own standard library
    char *include_dirs[] = {"/usr/include/", "/usr/local/include/"};

    char header_path[128] = {0};

    bool found_header = false;

    token_node = token_node->next;

    token *header_token = &token_node->token;

    strcpy(header_path, header_token->src_filepath);
    header_path[header_token->filename_index] = '\0';
    strcat(header_path + header_token->filename_index, header_token->lexeme);

    found_header = add_file(header_path);

    // Try to open the header file in different include directories until successful
    for (size_t i = 0; !found_header && i < sizeof(include_dirs)/sizeof(include_dirs[0]); i++) {

        strcpy(header_path, include_dirs[i]);
        strcat(header_path, token_node->token.lexeme);

        found_header = add_file(header_path);
    }

    if (!found_header) {
        char error_msg[64] = "Cannot find ";
        strcat(error_msg, token_node->token.lexeme);

        error(token_node->token.src_filepath, token_node->token.line, error_msg);
        return;
    }

    token_node = token_node->next;

    debugf("Including: %s\n", header_path);
    scan_and_insert_tokens(token_node);
}

void handle_define_directive(tk_node *token_node) {
    in_define = 1;

    macro new_macro = {0};

    token *identifier_token = &token_node->token;

    strcpy(new_macro.defined_file, identifier_token->src_filepath);
    new_macro.defined_line = identifier_token->line;

    token_node = token_node->next;

    if (identifier_token->type != IDENTIFIER) {
        error(identifier_token->src_filepath, identifier_token->line, "Expected identifier after #define");
    }

    strcpy(new_macro.name, identifier_token->lexeme);
    new_macro.hash = hash(new_macro.name);

    // If the token is a left parenthesis, and if there was no whitespace before it, it's function-like
    if (token_node->token.subtype == PUN_LEFT_PARENTHESIS &&
        token_node->token.follows_whitespace == false) {
        token_node = token_node->next;

        new_macro.is_function_like = true;

        while (token_node->token.subtype != PUN_RIGHT_PARENTHESIS) {

            if (token_node->token.type != IDENTIFIER && token_node->token.subtype != PUN_ELLIPSIS) {
                error(token_node->token.src_filepath, token_node->token.line,
                     "Expected identifier or ... in macro parameter list");
                return;
            }

            if (token_node->token.subtype == PUN_ELLIPSIS &&
                token_node->next->token.subtype != PUN_RIGHT_PARENTHESIS) {
                error(token_node->token.src_filepath, token_node->token.line,
                 "Expected ... to be the last argument");
                return;
            }

            strcpy(new_macro.parameters[new_macro.num_params++], token_node->token.lexeme);

            token_node = token_node->next;

            if (token_node->token.subtype == PUN_COMMA) {
                token_node = token_node->next;
            }

        }
        token_node = token_node->next;
    }

    // Build up the replacement list
    token *replacement_ptr = new_macro.replacement;

    while (token_node->token.type != NEWLINE) {
        *replacement_ptr = token_node->token;

        for (short i = 0; i < new_macro.num_params; i++) {
            if (strcmp(token_node->token.lexeme, new_macro.parameters[i]) == 0) {
                replacement_ptr->type = ARGUMENT;
            }
        }
        replacement_ptr++;
        token_node = token_node->next;
    }

    add_macro(new_macro);

    in_define = 0;
}

void handle_undef_directive(tk_node *token_node) {
    if (token_node->token.type != IDENTIFIER) {
        error(token_node->token.src_filepath, token_node->token.line, "Expected identifier after #undef");
        return;
    }

    remove_macro(token_node->token.lexeme);
}

void handle_defined(tk_node *token_node) {
    // token_node should point to token before "defined" token

    tk_node *before_defined = token_node;
    bool remove_right_parenthesis = false;

    token_node = token_node->next->next;

    if (token_node->token.subtype == PUN_LEFT_PARENTHESIS) {
        token_node = token_node->next;
        before_defined->next->next = token_node;
        remove_right_parenthesis = true;
    }

    token_node->token.type = CONSTANT;
    token_node->token.subtype = CONST_INTEGER;

    if (macro_exists(token_node)) {
        strcpy(token_node->token.lexeme, "1");
    }
    else {
        strcpy(token_node->token.lexeme, "0");
    }

    before_defined->next = token_node;

    if (remove_right_parenthesis && token_node->next->token.subtype == PUN_RIGHT_PARENTHESIS) {
        token_node->next = token_node->next->next;
    }
}

bool evaluate_if(tk_node *token_node) {
    // token_node should point to #if token
    tk_node *before_token_ptr = token_node;

    token RPN_tokens[64];
    token *RPN_pointer = RPN_tokens;

    token op_stack[64];
    token *op_stack_pointer = op_stack;

    int number_stack[64];
    int *number_stack_pointer = number_stack;

    token_node = token_node->next;

    // Shunting Yard Algorithm
    while (token_node->token.type != NEWLINE) {
        if (token_node->token.type == IDENTIFIER) {
            if (strcmp(token_node->token.lexeme, "defined") == 0) {
                handle_defined(before_token_ptr);
            }
            else {
                token_node->token.type = CONSTANT;
                token_node->token.subtype = CONST_INTEGER;
                strcpy(token_node->token.lexeme, "0");

                *RPN_pointer++ = token_node->token;
            }
        }
        else if (token_node->token.subtype >= CONST_INTEGER &&
                 token_node->token.subtype <= CONST_WIDE_CHAR) {
            *RPN_pointer++ = token_node->token;
        }
        else if (token_node->token.subtype == PUN_LEFT_PARENTHESIS) {
            *op_stack_pointer++ = token_node->token;
        }
        else if (token_node->token.subtype == PUN_RIGHT_PARENTHESIS) {
            while (op_stack_pointer != op_stack && op_stack_pointer[-1].subtype != PUN_LEFT_PARENTHESIS) {
                *RPN_pointer++ = *--op_stack_pointer;
            }
            op_stack_pointer--; // Discard the left parenthesis
        }
        // For the ternary operator, the '?' is kept, it's treated like a normal operator
        else if (token_node->token.subtype == PUN_COLON) {
            while (op_stack_pointer != op_stack && op_stack_pointer[-1].subtype != PUN_QUESTION_MARK) {
                *RPN_pointer++ = *--op_stack_pointer;
            }
        }
        else if (token_node->token.type == BLANK) {
            // Ignore any blank tokens
        }
        else if (token_node->token.type != PUNCTUATOR) {
            error(token_node->token.src_filepath, token_node->token.line, "Expected punctuator");
            return false;
        }
        else {
            while (op_stack_pointer != op_stack &&
                   operator_precedence[token_node->token.subtype]
                   <= operator_precedence[op_stack_pointer[-1].subtype]) {
                *RPN_pointer++ = *--op_stack_pointer;
            }
            *op_stack_pointer++ = token_node->token;
        }
        before_token_ptr = token_node;
        token_node = token_node->next;
    }

    while (op_stack_pointer != op_stack) {
        *RPN_pointer++ = *--op_stack_pointer;
    }

    for (token *ptr = RPN_tokens; ptr < RPN_pointer; ptr++) {
        if (ptr->subtype >= CONST_INTEGER && ptr->subtype <= CONST_UNSIGNED_LONG_LONG) {
            *number_stack_pointer++ = atoi(ptr->lexeme); // TODO: Implement this conversion
        }
        else if (ptr->subtype == CONST_CHAR || ptr->subtype == CONST_WIDE_CHAR) {
            *number_stack_pointer++ = ptr->lexeme[0]; // TODO: Might need to handle wide char differently
        }
        else if (ptr->type == PUNCTUATOR) {
            int r_operand = *--number_stack_pointer;
            int m_operand; // Only used for ternary operator (a ? b : c)
            int l_operand;
            if (ptr->subtype == PUN_QUESTION_MARK) m_operand = *--number_stack_pointer;;
            if (ptr->subtype != PUN_EXCLAMATION_MARK) l_operand = *--number_stack_pointer;

            switch (ptr->subtype) {
                case PUN_PLUS: *number_stack_pointer++ = l_operand + r_operand; break;
                case PUN_MINUS: *number_stack_pointer++ = l_operand - r_operand; break;
                case PUN_ASTERISK: *number_stack_pointer++ = l_operand * r_operand; break;
                case PUN_FWD_SLASH: *number_stack_pointer++ = l_operand / r_operand; break;
                case PUN_LOGICAL_AND: *number_stack_pointer++ = l_operand && r_operand; break;
                case PUN_LOGICAL_OR: *number_stack_pointer++ = l_operand || r_operand; break;
                case PUN_GREATER_THAN: *number_stack_pointer++ = l_operand > r_operand; break;
                case PUN_GREATER_THAN_EQUAL: *number_stack_pointer++ = l_operand >= r_operand; break;
                case PUN_LESS_THAN: *number_stack_pointer++ = l_operand < r_operand; break;
                case PUN_LESS_THAN_EQUAL: *number_stack_pointer++ = l_operand <= r_operand; break;
                case PUN_EQUALITY: *number_stack_pointer++ = l_operand == r_operand; break;
                case PUN_INEQUALITY: *number_stack_pointer++ = l_operand != r_operand; break;
                case PUN_EXCLAMATION_MARK: *number_stack_pointer++ = !r_operand; break;
                case PUN_QUESTION_MARK: *number_stack_pointer++ = l_operand ? m_operand : r_operand; break;
                default: error(ptr->src_filepath, ptr->line, "#if: Unknown operator"); return false;
            }
        }
    }

    return number_stack[0];
}

void handle_if_directives(tk_node *token_node, enum subtype if_type) {
    tk_node *if_directive = token_node;
    short current_if_level = 0;
    bool cond;
    bool if_cond;
    bool any_cond_true = false;

    if (if_type == DIRECTIVE_IFDEF || if_type == DIRECTIVE_IFNDEF) {
        tk_node *new_list_entry = allocate_from_arena(token_arena, sizeof(tk_node));

        new_list_entry->token = (token) {.type = IDENTIFIER, .line = token_node->token.line};
        strcpy(new_list_entry->token.lexeme, "defined");

        strcpy(new_list_entry->token.src_filepath, token_node->token.src_filepath);
        new_list_entry->token.filename_index = token_node->token.filename_index;

        new_list_entry->next = token_node->next;
        token_node->next = new_list_entry;

        // Feels like the wrong way round (it's not)
        if (if_type == DIRECTIVE_IFNDEF) {
            new_list_entry = allocate_from_arena(token_arena, sizeof(tk_node));

            new_list_entry->token = (token) {.type = PUNCTUATOR, .subtype = PUN_EXCLAMATION_MARK,
                                             .line = token_node->token.line};

            strcpy(new_list_entry->token.lexeme, "!");

            strcpy(new_list_entry->token.src_filepath, token_node->token.src_filepath);
            new_list_entry->token.filename_index = token_node->token.filename_index;

            new_list_entry->next = token_node->next;
            token_node->next = new_list_entry;
        }
    }

    cond = evaluate_if(token_node);


    if_cond = cond;
    any_cond_true |= cond;

    while (token_node->token.subtype != DIRECTIVE_ENDIF) {
        // Move to the end of the directive
        while (token_node->token.type != NEWLINE) {
            token_node = advance_list(token_node, 1);
        }

        tk_node *before_remove = token_node;

        token_node = advance_list(token_node, 1);

        tk_node *remove_start = token_node;

        if (if_directive->token.subtype == DIRECTIVE_ELIF) {

            // Basically a copy
            tk_node *ptr = if_directive;
            while (ptr->next->token.type != NEWLINE) {
                if (strcmp(ptr->token.lexeme, "defined") != 0 && macro_exists(ptr->next)) {
                    tk_list_segment expanded_macro_segment = expand_macro(ptr->next);

                    ptr->next = advance_list(ptr, 2); // Remove the macro identifier

                    insert_list_segment(ptr, expanded_macro_segment);

                    ptr = advance_list(ptr, expanded_macro_segment.len); // Move to the end of the expanded macro
                } else {
                    ptr = advance_list(ptr, 1);
                }
            }

            // Only want the elif condition to be true if
            // the original condition and the previous condition was false
            cond = !if_cond && !cond && evaluate_if(if_directive);
            any_cond_true |= cond;
        }

        while (current_if_level > 0 || (token_node->token.subtype != DIRECTIVE_ELIF &&
                                        token_node->token.subtype != DIRECTIVE_ELSE &&
                                        token_node->token.subtype != DIRECTIVE_ENDIF)) {

            if (token_node->token.subtype == DIRECTIVE_IF) current_if_level++;
            if (token_node->token.subtype == DIRECTIVE_IFDEF) current_if_level++;
            if (token_node->token.subtype == DIRECTIVE_IFNDEF) current_if_level++;
            if (token_node->token.subtype == DIRECTIVE_ENDIF) current_if_level--;
            token_node = token_node->next;
        }

        tk_node *remove_end = token_node;

        if (!cond && (if_directive->token.subtype == DIRECTIVE_IF || if_directive->token.subtype == DIRECTIVE_ELIF ||
                      if_directive->token.subtype == DIRECTIVE_IFDEF || if_directive->token.subtype == DIRECTIVE_IFNDEF)) {
            remove_from_list(before_remove, remove_start, remove_end);
        }

        if (any_cond_true && if_directive->token.subtype == DIRECTIVE_ELSE) {
            remove_from_list(before_remove, remove_start, remove_end);
        }

        if (token_node->token.subtype != DIRECTIVE_ENDIF) {
            if_directive = token_node;
            token_node = token_node->next;
        }
    }
}

short find_parameter_index(const char *parameter_name, const macro *replacement_macro) {
    for (short param_num = 0; param_num < replacement_macro->num_params; param_num++) {
        if (strcmp(parameter_name, replacement_macro->parameters[param_num]) == 0) {
            return param_num;
        }
    }

    error(current_src_file, current_line, "Parameter not found");
    return -1;
}

tk_list_segment substitute_argument(tk_node *arg_tk_ptr, const macro *replacement_macro, token arguments[8][32]) {
    tk_list_segment arg_sub_segment = {NULL, NULL, 0};
    tk_node *seg_ptr = NULL;

    const int token_line = arg_tk_ptr->token.line;
    short param_index = -1;

    char token_source_file[128];
    ptrdiff_t token_filename_index;

    strcpy(token_source_file, arg_tk_ptr->token.src_filepath);
    token_filename_index = arg_tk_ptr->token.filename_index;

    param_index = find_parameter_index(arg_tk_ptr->token.lexeme, replacement_macro);

    // Parameter not found
    if (param_index == -1) {
        return arg_sub_segment;
    }

    arg_tk_ptr->token = arguments[param_index][0];
    arg_tk_ptr->token.line = token_line;

    strcpy(arg_tk_ptr->token.src_filepath, token_source_file);
    arg_tk_ptr->token.filename_index = token_filename_index;

    // While still more argument tokens, add them to the token list

    tk_node *new_entry;
    for (token *arg_token_ptr = &arguments[param_index][1]; arg_token_ptr->line != 0; arg_token_ptr++) {
        if (arg_sub_segment.start == NULL) {
            arg_sub_segment.start = allocate_from_arena(token_arena, sizeof(tk_node));
            new_entry = seg_ptr = arg_sub_segment.start;
        } else {
            new_entry = allocate_from_arena(token_arena, sizeof(tk_node));
        }

        arg_sub_segment.len++;

        new_entry->token = *arg_token_ptr;
        new_entry->token.line = token_line;

        strcpy(new_entry->token.src_filepath, arg_tk_ptr->token.src_filepath);
        new_entry->token.filename_index = arg_tk_ptr->token.filename_index;

        if (seg_ptr != new_entry) {
            new_entry->next = seg_ptr->next;
            seg_ptr->next = new_entry;
            seg_ptr = new_entry;
        }
    }

    arg_sub_segment.end = seg_ptr;
    return arg_sub_segment;
}

tk_node *consume_argument(tk_node *token_node, token *argument_tokens) {
    // Idea is once here, the tokens from token_node should be in the form:
    // ARG0, ARG1, ARG2 )

    size_t parenthesis_level = 0;
    size_t argument_counter = 0;

    if (token_node->token.subtype == PUN_COMMA ||
        token_node->token.subtype == PUN_RIGHT_PARENTHESIS) { // Empty argument
        *argument_tokens = (token) {.type = BLANK, .lexeme = "", .line = token_node->token.line,
                                    .filename_index = token_node->token.filename_index};

        strcpy(argument_tokens->src_filepath, token_node->token.src_filepath);

        return token_node;
    }

    while (parenthesis_level > 0 || (token_node->token.subtype != PUN_COMMA &&
                                     token_node->token.subtype != PUN_RIGHT_PARENTHESIS)) {

        if (token_node->token.subtype == PUN_LEFT_PARENTHESIS) parenthesis_level++;
        if (token_node->token.subtype == PUN_RIGHT_PARENTHESIS) parenthesis_level--;

        *argument_tokens++ = token_node->token;
        argument_counter++;

        token_node = advance_list(token_node, 1);
    }

    // Ignore whitespace before the first argument token.
    // Used for stringification
    (argument_tokens-argument_counter)->follows_whitespace = false;

    return token_node;
}

// stringify_argument will find the correct parameter, combine the lexemes of all tokens
// in the argument, taking into account the whitespace between them.
token stringify_argument(const char *parameter_name, macro *replacement_macro, token arguments[8][32]) {
    token stringified_token = {.type = STRING_LITERAL, .lexeme[0] = '\0', .line = current_line};
    token *argument_ptr = NULL;
    short param_index = -1;

    param_index = find_parameter_index(parameter_name, replacement_macro);

    // Parameter not found
    if (param_index == -1) {
        return (token) {0};
    }

    argument_ptr = arguments[param_index];

    // Ignore any whitespace before the argument's first token
    for (; (argument_ptr+1)->line != 0; argument_ptr++) {
        strcat(stringified_token.lexeme, argument_ptr->lexeme);

        if ((argument_ptr+1)->follows_whitespace) {
            strcat(stringified_token.lexeme, " ");
        }
    }

    // Ignore any whitespace after the argument's last token
    strcat(stringified_token.lexeme, argument_ptr->lexeme);

    return stringified_token;
}

tk_list_segment expand_macro(tk_node *tk_list_ptr) {
    macro *replacement_macro = NULL;
    token arguments[8][32] = {0};

    tk_list_segment macro_expanded_segment = {NULL, NULL, 0};

    if ((replacement_macro = macro_exists(tk_list_ptr)) == NULL) {
        error(tk_list_ptr->token.src_filepath, tk_list_ptr->token.line, "Expected macro");
        return (tk_list_segment) {NULL, NULL, 0};
    }

    if (replacement_macro->is_function_like) {
        tk_node *arg_ptr = advance_list(tk_list_ptr, 2); // Set to after the opening parenthesis

        for (short arg_num = 0; arg_num < replacement_macro->num_params; arg_num++) {
            arg_ptr = consume_argument(arg_ptr, arguments[arg_num]);
            arg_ptr = advance_list(arg_ptr, 1); // Move past the comma or closing parenthesis
        }

        // If no parameters, still need to move past the closing parenthesis
        if (replacement_macro->num_params == 0) {
            arg_ptr = advance_list(arg_ptr, 1);
        }

        remove_from_list(tk_list_ptr, advance_list(tk_list_ptr, 1), arg_ptr);
    }

    const token *replacement_tk_ptr = replacement_macro->replacement;

    tk_node *new_entry = NULL;

    while (replacement_tk_ptr->line != 0) {

        if (macro_expanded_segment.start == NULL) {
            macro_expanded_segment.start = allocate_from_arena(token_arena, sizeof(tk_node));
            new_entry = macro_expanded_segment.start;
        } else if (new_entry->token.line != 0) { // If new_entry is "something", reuse it as it isn't included
            new_entry->next = allocate_from_arena(token_arena, sizeof(tk_node));
            *new_entry->next = (tk_node) {0};

            new_entry = advance_list(new_entry, 1);
        }

        macro_expanded_segment.len++;

        new_entry->token = *replacement_tk_ptr++;
        new_entry->token.irreplaceable = !strcmp(new_entry->token.lexeme, replacement_macro->name);
        new_entry->token.line = tk_list_ptr->token.line;
        new_entry->token.filename_index = tk_list_ptr->token.filename_index;
        strcpy(new_entry->token.src_filepath, tk_list_ptr->token.src_filepath);


        // Stringification #
        // -----------------
        if (new_entry->token.subtype == PUN_HASH) {
            const token * parameter_token = replacement_tk_ptr++;

            if (parameter_token->type != ARGUMENT) {
                error(tk_list_ptr->token.src_filepath, tk_list_ptr->token.line, "# not followed by parameter");
                return (tk_list_segment) {0};
            }

            new_entry->token = stringify_argument(parameter_token->lexeme, replacement_macro, arguments);
        }
        // -----------------

        // Argument substitution
        // ---------------------
        tk_list_segment arg_sub_segment = {NULL, NULL, 0};
        tk_node *first_arg_sub = NULL;
        if (new_entry->token.type == ARGUMENT) {
            arg_sub_segment = substitute_argument(new_entry, replacement_macro, arguments);
            first_arg_sub = new_entry;

            if (arg_sub_segment.len > 0) {
                arg_sub_segment.end->next = new_entry->next;
                new_entry->next = arg_sub_segment.start;

                new_entry = arg_sub_segment.end;

                macro_expanded_segment.len += arg_sub_segment.len;
            }
        }
        // ---------------------

        // Token concatination ##
        // ----------------------
        if (new_entry->token.subtype == PUN_DOUBLE_HASH) {
            // If the ## is the first replacement token
            error(tk_list_ptr->token.src_filepath, tk_list_ptr->token.line, "Found ## at start of replacement list");
            return (tk_list_segment) {0};
        }

        // Checks if the next replacement token is a ##
        // no +1 since the ptr is already incremented
        while (replacement_tk_ptr->subtype == PUN_DOUBLE_HASH) {
            // If the ## is the last replacement token (i.e. the next token is invalid), throw an error
            if ((replacement_tk_ptr+1)->line == 0) {
                error(tk_list_ptr->token.src_filepath, tk_list_ptr->token.line, "Found ## at end of replacement list");
                return (tk_list_segment) {0};
            }

            // new_entry contains the token to the left of the ##

            // concat_tk_list contains the token to concatenate as well as any
            // argument tokens if the token to the right of the ## is an argument
            tk_node concat_tk_list = {*(replacement_tk_ptr+1), NULL};

            if (concat_tk_list.token.type == ARGUMENT) {
                tk_list_segment arg_sub_segment = substitute_argument(&concat_tk_list, replacement_macro, arguments);

                if (arg_sub_segment.len > 0) {
                    arg_sub_segment.end->next = new_entry->next;
                    new_entry->next = arg_sub_segment.start;

                    new_entry = arg_sub_segment.end;
                }
            }

            // If tokens are not the same type, set the type to an identifier
            if (new_entry->token.type != concat_tk_list.token.type || new_entry->token.subtype != concat_tk_list.token.subtype) {
                new_entry->token.type = IDENTIFIER;
            }

            strcat(new_entry->token.lexeme, concat_tk_list.token.lexeme);

            replacement_tk_ptr += 2; // Skip the ## token and the token to the right of it
        }
        // ----------------------

        // Expand substituted tokens
        // -------------------------
        if (first_arg_sub != NULL) {
            for (tk_node *ptr = first_arg_sub; ptr != NULL; ptr = advance_list(ptr, 1)) {
                macro *potential_macro = macro_exists(ptr);

                if (potential_macro && !ptr->token.irreplaceable) {
                    // Expand any remaining argument tokens
                    tk_list_segment arg_macro_segment = expand_macro(ptr);

                    // If function-like, need to re-evaluate length,
                    // as argument tokens will have been removed
                    if (potential_macro->is_function_like) {
                        size_t old_len = arg_sub_segment.len;
                        tk_node *len_ptr = first_arg_sub->next;

                        arg_sub_segment.start = first_arg_sub->next;
                        arg_sub_segment.len = 0;

                        // Only need to loop through if the new length is > 0
                        if (len_ptr != NULL) {
                            for (;len_ptr->next != NULL; len_ptr = advance_list(len_ptr, 1)) {
                                arg_sub_segment.len++;
                            }

                            arg_sub_segment.len++;
                            arg_sub_segment.end = len_ptr;
                        } else {
                            arg_sub_segment.end = arg_sub_segment.start;
                        }

                        if (arg_sub_segment.len > 0) {
                            // Sanity check that arg_sub_segment.len is correct
                            assert(advance_list(arg_sub_segment.start, arg_sub_segment.len - 1) == arg_sub_segment.end);
                            assert(advance_list(arg_sub_segment.end, 1) == NULL);
                        }

                        macro_expanded_segment.len -= (old_len - arg_sub_segment.len);
                    }

                    if (arg_macro_segment.len > 0) {

                        ptr->token = arg_macro_segment.start->token;

                        arg_macro_segment.end->next = ptr->next;
                        ptr->next = arg_macro_segment.start->next;
                        ptr = arg_macro_segment.end;

                        macro_expanded_segment.len += arg_macro_segment.len - 1;
                        arg_sub_segment.len += arg_macro_segment.len - 1;
                    }
                    else {
                        ptr->token.type = BLANK;
                        ptr->token.lexeme[0] = '\0';
                    }
                }
            }

            new_entry = advance_list(first_arg_sub, arg_sub_segment.len);

            // Sanity check that new_entry is at the end of the list
            assert(new_entry != NULL);
            assert(new_entry->next == NULL);
        }
        // -------------------------
    }

    // Rescan and expansion
    // --------------------
    for (tk_node * sub_tk_ptr = macro_expanded_segment.start; sub_tk_ptr != NULL;
         sub_tk_ptr = advance_list(sub_tk_ptr, 1)) {

        macro *potential_macro = macro_exists(sub_tk_ptr);
        if (potential_macro != NULL && !sub_tk_ptr->token.irreplaceable) {
            tk_list_segment sub_macro_segment = expand_macro(sub_tk_ptr);

            // If function-like, need to re-evaluate length,
            // as argument tokens will have been removed
            if (potential_macro->is_function_like) {
                macro_expanded_segment.len = 0;
                for (tk_node *len_ptr = macro_expanded_segment.start; len_ptr != NULL;
                     len_ptr = advance_list(len_ptr, 1)) {
                        macro_expanded_segment.len++;
                }
            }

            if (sub_macro_segment.len > 0) {
                sub_tk_ptr->token = sub_macro_segment.start->token;

                sub_macro_segment.end->next = sub_tk_ptr->next;
                sub_tk_ptr->next = sub_macro_segment.start->next;
                sub_tk_ptr = sub_macro_segment.end;

                macro_expanded_segment.len += sub_macro_segment.len - 1;
            } else {
                sub_tk_ptr->token.type = BLANK;
                sub_tk_ptr->token.lexeme[0] = '\0';
            }
        }
    }
    // --------------------

    // Only set the segment end if the segment isn't empty.
    // If the segment is empty, the end will remain NULL,
    // otherwise set the end to "start + length - 1"
    if (macro_expanded_segment.len > 0) {
        macro_expanded_segment.end = advance_list(macro_expanded_segment.start, macro_expanded_segment.len - 1);
    }
    return macro_expanded_segment;
}

void process_preprocessing_tokens(tk_node *token_node) {
    tk_node* ptr = token_node;
    tk_node* before_directive;

    while(ptr->next != NULL) {
        current_src_file = ptr->token.src_filepath;
        current_line = ptr->token.line;

         if (ptr->token.type != DIRECTIVE) {
            before_directive = ptr;

            if (macro_exists(ptr->next)) {
                tk_list_segment expanded_macro_segment = expand_macro(ptr->next);

                // Remove the macro identifier
                ptr->next = advance_list(ptr, 2);

                insert_list_segment(ptr, expanded_macro_segment);

                // Move to end of expanded macro
                ptr = advance_list(ptr, expanded_macro_segment.len);
            } else {
                ptr = advance_list(ptr, 1);
            }
            continue;
        }

        tk_node *directive = ptr;

        // Expand any macros within the directive
        while (ptr->next->token.type != NEWLINE) {

            // Skip expansion for `defined` operator
            if (strcmp(ptr->token.lexeme, "defined") == 0) {

                // If a ( is found, make sure there's a matching )
                if (ptr->next->token.subtype == PUN_LEFT_PARENTHESIS) {
                    ptr = advance_list(ptr, 3);

                    if (ptr->token.subtype != PUN_RIGHT_PARENTHESIS) {
                        error(ptr->token.src_filepath, ptr->token.line, "Expected ) after defined");
                    }

                    continue;
                }

                ptr = advance_list(ptr, 1);
                continue;
            }

            if (macro_exists(ptr->next) &&
                directive->token.subtype != DIRECTIVE_UNDEF && directive->token.subtype != DIRECTIVE_DEFINE &&
                directive->token.subtype != DIRECTIVE_IFDEF && directive->token.subtype != DIRECTIVE_IFNDEF) {
                tk_list_segment expanded_macro_segment = expand_macro(ptr->next);

                // Remove the macro identifier
                ptr->next = advance_list(ptr, 2);

                insert_list_segment(ptr, expanded_macro_segment);

                // Move to the end of the expanded macro
                ptr = advance_list(ptr, expanded_macro_segment.len);
            } else {
                ptr = advance_list(ptr, 1);
            }
        }

        ptr = directive;

        switch (directive->token.subtype) {
            case DIRECTIVE_DEFINE:
                handle_define_directive(directive->next);
                break;
            case DIRECTIVE_UNDEF:
                handle_undef_directive(directive->next);
                break;
            case DIRECTIVE_IFDEF:
                handle_if_directives(directive, directive->token.subtype);
                break;
            case DIRECTIVE_IFNDEF:
                handle_if_directives(directive, directive->token.subtype);
            break;
            case DIRECTIVE_IF:
                handle_if_directives(directive, directive->token.subtype);
                break;
            case DIRECTIVE_INCLUDE:
                handle_include_directive(directive);
                break;
            case DIRECTIVE_PRAGMA:
                // Leave in Pragma directives for now
                while (ptr->next->token.type != NEWLINE) {
                    ptr = advance_list(ptr, 1);
                }
                continue;
            default: break;
        }

        // Go to end of directive line
        while (ptr->next->token.type != NEWLINE) {
            ptr = advance_list(ptr, 1);
        }

        // Remove the directive
        remove_from_list(before_directive, directive, ptr->next->next);
        ptr = before_directive;
    }

}
