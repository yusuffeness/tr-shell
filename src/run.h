/*
 * run.h - running shell text: a line, a file, or a $(...) capture.
 */
#ifndef TRSH_RUN_H
#define TRSH_RUN_H

#include <stdio.h>

#include "parser.h"

#define RUN_INCOMPLETE (-2)     /* the text ends inside eger / dongu / her */
#define EXIT_SYNTAX    2        /* same as bash for syntax errors */

/*
 * Tokenizes, expands aliases and parses text. Returns 0, -1 after a
 * syntax error, or RUN_INCOMPLETE (without a message) if allow_incomplete
 * is set and more lines are needed.
 */
int run_parse(const char *text, struct script *out, int allow_incomplete);

/* Runs and frees a parsed script. Returns the last exit status. */
int run_script(struct script *s);

/*
 * Parses and runs text. Returns the exit status, EXIT_SYNTAX after a
 * syntax error, or RUN_INCOMPLETE as above.
 */
int run_text(const char *text, int allow_incomplete);

/* Runs a whole file, joining lines of eger / dongu / her blocks. */
int run_file(FILE *f);

/*
 * $(...): runs text in a child shell and stores everything it printed in
 * *out (allocated; caller frees). Returns the child's exit status.
 */
int run_capture(const char *text, char **out);

/* `kaynak DOSYA`: runs a file in the current shell (like `source`). */
int bi_kaynak(int argc, char **argv);

#endif
