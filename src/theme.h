/*
 * theme.h - color themes.
 *
 * A theme only decides the accent colors (box, logo, prompt frame,
 * directory, git branch). Success / error colors stay green / red.
 * The chosen theme is saved in ~/.trsh/theme.
 */
#ifndef TRSH_THEME_H
#define TRSH_THEME_H

struct theme {
    const char *name;
    const char *desc;
    const char *accent;     /* frame, logo, directory */
    const char *accent2;    /* git branch, secondary details */
};

/* The active theme (never NULL). */
const struct theme *theme_current(void);

/* Loads the saved theme, if any. Unknown or missing values keep the default. */
void theme_load(void);

/* `tema` builtin: lists themes, or switches to and saves the named one. */
int bi_tema(int argc, char **argv);

#endif
