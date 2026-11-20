/*
 * parser.c - splitting a command line into arguments.
 */
#include <string.h>

#include "parser.h"
#include "messages.h"
#include "ui.h"

#define DELIMS " \t\n"

int parse_line(char *line, char **argv, int max_args)
{
    int argc = 0;
    char *tok = strtok(line, DELIMS);

    while (tok != NULL) {
        if (argc >= max_args - 1) {
            ui_error(TRSH_NAME, MSG_TOO_MANY_ARGS, max_args - 1);
            return -1;
        }
        argv[argc++] = tok;
        tok = strtok(NULL, DELIMS);
    }
    argv[argc] = NULL;
    return argc;
}
