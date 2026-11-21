/*
 * input.h - reading command lines.
 *
 * An interactive shell built with readline gets line editing, history on
 * the arrow keys and Tab completion. Otherwise (or when reading from a file
 * or pipe) lines are read with fgets.
 */
#ifndef TRSH_INPUT_H
#define TRSH_INPUT_H

#include <stddef.h>

enum input_result {
    INPUT_LINE        =  1,  /* a line was read into buf (without '\n') */
    INPUT_EOF         =  0,  /* end of input (Ctrl+D) or read error */
    INPUT_TOO_LONG    = -1,  /* the line did not fit and was discarded */
    INPUT_INTERRUPTED = -2,  /* Ctrl+C at the prompt */
};

/* Sets up readline (completion, history) if it is available. */
void input_init(void);

/* 1 if lines are read with readline. */
int input_uses_readline(void);

/* Reads one line, showing prompt first when interactive. */
enum input_result input_read_line(char *buf, size_t size, const char *prompt);

#endif
