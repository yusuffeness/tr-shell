/*
 * input.c - reading lines from standard input.
 */
#include <stdio.h>
#include <string.h>

#include "input.h"
#include "messages.h"
#include "ui.h"

int input_read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        if (ferror(stdin))
            perror(TRSH_NAME);
        return 0;
    }

    /* The line did not fit: drop the remainder so it is not run next. */
    if (strchr(buf, '\n') == NULL && !feof(stdin)) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        ui_error(TRSH_NAME, MSG_LINE_TOO_LONG, (int)size - 2);
        return -1;
    }
    return 1;
}
