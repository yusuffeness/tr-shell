/*
 * executor.h - running parsed command lines.
 *
 * Pipelines are run with fork, pipe and dup2; redirections with open and
 * dup2. A builtin that is not part of a pipeline runs inside the shell
 * (so `gec` can change the shell's directory), with its redirections
 * applied around it and undone afterwards.
 */
#ifndef TRSH_EXECUTOR_H
#define TRSH_EXECUTOR_H

#include "parser.h"

/* Runs a whole parsed line. Returns the status of the last command. */
int exec_script(const struct script *s);

/*
 * Runs one already-expanded command in the foreground: a builtin directly,
 * anything else in a child. Used by `zamanla` and by commands that call
 * other programs (mv, rm).
 */
int exec_argv(int argc, char **argv);

/* Same as exec_argv, for a NULL-terminated argv. */
int executor_run(char *const argv[]);

#endif
