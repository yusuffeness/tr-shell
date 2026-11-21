/*
 * signals.h - keyboard signals in an interactive shell.
 *
 *   Ctrl+C  (SIGINT)   at the prompt: drops the current line.
 *                      while a command runs: stops the command, not the shell.
 *   Ctrl+\  (SIGQUIT)  ignored by the shell; still stops a running command.
 *   Ctrl+Z  (SIGTSTP)  ignored until job control arrives.
 */
#ifndef TRSH_SIGNALS_H
#define TRSH_SIGNALS_H

#include <signal.h>

/* Set by the SIGINT handler; cleared by whoever handles it. */
extern volatile sig_atomic_t g_sigint;

/* Installs the handlers above (interactive shells only). */
void signals_init(void);

/* In a forked child, before exec: give SIGINT and SIGQUIT their defaults back. */
void signals_child(void);

#endif
