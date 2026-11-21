/*
 * theme.h - color themes.
 *
 * A theme only decides the accent colors (box, logo, prompt frame,
 * directory, git branch). Success / error colors stay green / red.
 * The chosen theme is saved in the config file (~/.trsh/config).
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

/* Switches to the named theme. Returns 0, or -1 if there is no such theme. */
int theme_set(const char *name);

/* `tema` builtin: lists themes, or switches to and saves the named one. */
int bi_tema(int argc, char **argv);

#endif
