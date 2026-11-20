/*
 * theme.c - color themes.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "theme.h"
#include "messages.h"
#include "ui.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define CONFIG_DIR  ".trsh"
#define THEME_FILE  "theme"

/* 256-color foregrounds; supported by Windows Terminal, iTerm2, GNOME, etc. */
static const struct theme themes[] = {
    { "mavi",    "Mavi (varsayılan)",     "\033[38;5;39m",  "\033[38;5;111m" },
    { "yesil",   "Yeşil",                 "\033[38;5;42m",  "\033[38;5;114m" },
    { "mor",     "Mor",                   "\033[38;5;135m", "\033[38;5;183m" },
    { "turuncu", "Turuncu",               "\033[38;5;208m", "\033[38;5;222m" },
    { "sade",    "Sade (vurgu rengi yok)", "",              ""               },
};

#define THEMES_LEN (sizeof themes / sizeof themes[0])

static const struct theme *current = &themes[0];

const struct theme *theme_current(void)
{
    return current;
}

static const struct theme *theme_find(const char *name)
{
    for (size_t i = 0; i < THEMES_LEN; i++)
        if (strcmp(themes[i].name, name) == 0)
            return &themes[i];
    return NULL;
}

/* Builds ~/.trsh or ~/.trsh/theme into buf. Returns 0 if HOME is unset. */
static int config_path(char *buf, size_t size, const char *file)
{
    const char *home = getenv("HOME");

    if (home == NULL || *home == '\0')
        return 0;
    if (file)
        snprintf(buf, size, "%s/%s/%s", home, CONFIG_DIR, file);
    else
        snprintf(buf, size, "%s/%s", home, CONFIG_DIR);
    return 1;
}

void theme_load(void)
{
    char path[PATH_MAX];
    char name[32];

    if (!config_path(path, sizeof path, THEME_FILE))
        return;

    FILE *f = fopen(path, "r");
    if (f == NULL)
        return;
    if (fgets(name, sizeof name, f) != NULL) {
        name[strcspn(name, "\r\n")] = '\0';
        const struct theme *t = theme_find(name);
        if (t != NULL)
            current = t;
    }
    fclose(f);
}

static int theme_save(const char *who)
{
    char dir[PATH_MAX];
    char path[PATH_MAX];

    if (!config_path(dir, sizeof dir, NULL) ||
        !config_path(path, sizeof path, THEME_FILE)) {
        ui_error(who, "%s", MSG_NO_HOME);
        return 1;
    }
    if (mkdir(dir, 0755) == -1 && errno != EEXIST) {
        ui_error(who, "%s: %s", dir, msg_error(errno));
        return 1;
    }

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ui_error(who, "%s: %s", path, msg_error(errno));
        return 1;
    }
    fprintf(f, "%s\n", current->name);
    if (fclose(f) != 0) {
        ui_error(who, "%s: %s", path, msg_error(errno));
        return 1;
    }
    return 0;
}

static void theme_list(void)
{
    for (size_t i = 0; i < THEMES_LEN; i++) {
        const struct theme *t = &themes[i];
        int active = t == current;

        printf("  %s%s%s %s%-8s%s %s\n",
               C(t->accent), active ? "●" : "○", RESET,
               active ? BOLD : "", t->name, RESET, t->desc);
    }
}

int bi_tema(int argc, char **argv)
{
    if (argc > 2) {
        ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
        return 1;
    }
    if (argc == 1) {
        theme_list();
        return 0;
    }

    const struct theme *t = theme_find(argv[1]);
    if (t == NULL) {
        ui_error(argv[0], MSG_UNKNOWN_THEME, argv[1]);
        return 1;
    }
    current = t;
    if (theme_save(argv[0]) != 0)
        return 1;
    printf(MSG_THEME_SET "\n", t->name);
    return 0;
}
