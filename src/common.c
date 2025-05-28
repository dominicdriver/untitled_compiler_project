#include "enums.h"
#include "common.h"

#include <stdio.h>

#define RED_TEXT "\x1B[1;31m"
#define RESET_TEXT "\x1B[0m"

#define COLOUR_TEXT(colour, text) colour##_TEXT text RESET_TEXT

int operator_precedence[] = {
    [PUN_ASTERISK] = 1, [PUN_FWD_SLASH] = 1,
    [PUN_EXCLAMATION_MARK] = 2,
    [PUN_LEFT_PARENTHESIS] = -10, [PUN_RIGHT_PARENTHESIS] = -10,
    [PUN_LOGICAL_AND] = -1, [PUN_LOGICAL_OR] = -2,
    [PUN_QUESTION_MARK] = -5,
    [PUN_REMAINDER] = 1,
    0
};

void error(const string *filename, int line, char *message) {
    fprintf(stderr, COLOUR_TEXT(RED, "Error in %.*s on line %d: %s\n"), filename->len, filename->data, line, message);
}
