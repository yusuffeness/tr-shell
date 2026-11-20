/*
 * input.c - standart girdiden satır okuma.
 */
#include <stdio.h>
#include <string.h>

#include "input.h"
#include "ui.h"

int input_read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        if (ferror(stdin))
            perror("okuma hatasi");
        return 0;
    }

    /* Satır tampona sığmadıysa geri kalanını at. */
    if (strchr(buf, '\n') == NULL && !feof(stdin)) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        fprintf(stderr, "%s%s:%s satir cok uzun (en fazla %d karakter)\n",
                RED, TRSH_NAME, RESET, (int)size - 2);
        return -1;
    }
    return 1;
}
