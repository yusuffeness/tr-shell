/*
 * ui.h - renkler, açılış/kapanış mesajları ve istem.
 */
#ifndef TRSH_UI_H
#define TRSH_UI_H

#include "shell.h"

/* Renkler yalnızca çıktı bir terminale gidiyorsa kullanılır. */
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

#endif
