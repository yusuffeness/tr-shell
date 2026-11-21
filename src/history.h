/*
 * history.h - command history.
 *
 * The last `gecmis_boyutu` lines (see config.h) are kept in memory,
 * numbered from 1 (oldest).
 * In an interactive shell they are also appended to ~/.trsh/history and
 * loaded again on the next start; with readline they are reachable with
 * the arrow keys and Ctrl+R.
 */
#ifndef TRSH_HISTORY_H
#define TRSH_HISTORY_H

#include <stddef.h>

/* Loads the history file (interactive shells only). */
void hist_init(void);

/* Records a line. Blank lines and repeats of the previous line are skipped. */
void hist_add(const char *line);

/*
 * Expands a leading history reference:
 *   !!       the previous line
 *   !N       line number N
 *   !-N      the Nth line back
 *   !PREFIX  the most recent line starting with PREFIX
 * Anything after the reference is kept (`!! -la`).
 * Returns 1 if line was expanded into out, 0 if there was nothing to expand,
 * and -1 (after printing an error) if the reference matched nothing.
 */
int hist_expand(const char *line, char *out, size_t size);

/* `gecmis [N]` builtin: prints the whole history, or the last N lines. */
int bi_gecmis(int argc, char **argv);

#endif
