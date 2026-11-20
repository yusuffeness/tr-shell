/*
 * parser.c - komut satırını argümanlara ayırma.
 */
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "ui.h"

#define DELIMS " \t\n"

int parse_line(char *line, char **argv, int max_args)
{
    int argc = 0;
    char *tok = strtok(line, DELIMS);

    while (tok != NULL) {
        if (argc >= max_args - 1) {
            fprintf(stderr, "%s%s:%s cok fazla arguman (en fazla %d)\n",
                    RED, TRSH_NAME, RESET, max_args - 1);
            return -1;
        }
        argv[argc++] = tok;
        tok = strtok(NULL, DELIMS);
    }
    argv[argc] = NULL;
    return argc;
}
