/*
 * ui.h - screen handling, colors, banner, prompt and error output.
 */
#ifndef TRSH_UI_H
#define TRSH_UI_H

#include <stddef.h>

#include "shell.h"
#include "theme.h"

/* Colors are emitted only when g_shell.use_color is set. */
#define C(code) (g_shell.use_color ? (code) : "")
#define RESET   C("\033[0m")
#define BOLD    C("\033[1m")
#define RED     C("\033[1;31m")
#define GREEN   C("\033[1;32m")
#define GRAY    C("\033[90m")
#define ACCENT  C(theme_current()->accent)
#define ACCENT2 C(theme_current()->accent2)

/*
 * Detects whether the shell is interactive, loads the theme and, on a
 * terminal, switches to the alternate screen so the previous terminal
 * contents are hidden until the shell exits.
 */
void ui_init(void);

/* Leaves the alternate screen. Safe to call more than once. */
void ui_shutdown(void);

/* Width of the terminal in columns (80 if unknown). */
int ui_term_cols(void);

/* Colors for file types in listele and agac. */
#define DIR_COLOR  ACCENT
#define EXEC_COLOR GREEN
#define LINK_COLOR C("\033[36m")

void ui_banner(void);

/* Prints the first prompt line (user, directory, branch, time, status). */
void ui_prompt(void);

/* The second prompt line ("╰─❯ "); escape codes wrapped for readline if asked. */
const char *ui_prompt_tail(int for_readline);
void ui_goodbye(void);

/* Clears the screen and draws the banner again (`temizle`). */
void ui_clear(void);

/*
 * Writes the date in Turkish: "Cuma, 20 Kasım 2026 · 12:57" with the
 * weekday, "20 Kasım 2026 · 12:57" without it.
 */
void ui_format_date(char *buf, size_t size, int with_weekday);

/* Prints "who: message" to stderr, with "who" in red. printf-style. */
void ui_error(const char *who, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#endif
