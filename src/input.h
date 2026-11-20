/*
 * input.h - reading lines from standard input.
 */
#ifndef TRSH_INPUT_H
#define TRSH_INPUT_H

#include <stddef.h>

/*
 * Reads one line into buf.
 * Returns 1 on success, 0 on end of input (EOF) or read error,
 * and -1 if the line was too long (the rest of it is discarded).
 */
int input_read_line(char *buf, size_t size);

#endif
