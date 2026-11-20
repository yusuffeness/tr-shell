/*
 * executor.h - running external programs in a child process.
 */
#ifndef TRSH_EXECUTOR_H
#define TRSH_EXECUTOR_H

/*
 * Runs argv[0] with fork + execvp and waits for it to finish.
 * Returns the command's exit status (128 + signal if it was killed).
 */
int executor_run(char *const argv[]);

#endif
