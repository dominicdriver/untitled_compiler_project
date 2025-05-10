#include "enums.h"

#include <stdio.h>

int operator_precedence[] = {
    [PUN_ASTERISK] = 1, [PUN_FWD_SLASH] = 1,
    [PUN_EXCLAMATION_MARK] = 2,
    [PUN_LEFT_PARENTHESIS] = -10, [PUN_RIGHT_PARENTHESIS] = -10,
    [PUN_LOGICAL_AND] = -1, [PUN_LOGICAL_OR] = -2,
    [PUN_QUESTION_MARK] = -5,
    [PUN_REMAINDER] = 1,
    0
};

void error(const char *filename, int line, char *message) {
    fprintf(stderr, "\033[91mError in %s on line %d: %s\033[0m\n", filename, line, message);
}
