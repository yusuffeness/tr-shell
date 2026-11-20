/*
 * ui.h - colors, banner, prompt and error reporting.
 */
#ifndef TRSH_UI_H
#define TRSH_UI_H

#include "shell.h"

/* Colors are emitted only when g_shell.use_color is set. */
#define C(code) (g_shell.use_color ? (code) : "")
#define RESET   C("\033[0m")
#define BOLD    C("\033[1m")
#define RED     C("\033[1;31m")
#define GREEN   C("\033[1;32m")
#define BLUE    C("\033[1;34m")
#define GRAY    C("\033[90m")

void ui_init(void);
void ui_banner(void);
void ui_prompt(void);
void ui_goodbye(void);

/* Prints "who: message" to stderr, with "who" in red. printf-style. */
void ui_error(const char *who, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#endif
