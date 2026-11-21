/*
 * completion.c - Tab completion (readline builds only).
 */
#define _POSIX_C_SOURCE 200809L

#include "completion.h"

#ifdef HAVE_READLINE

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <readline/readline.h>

#include "commands.h"
#include "config.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Every name the first word can complete to; built on the first Tab. */
static char **names;
static size_t names_len, names_cap;

static void add_name(const char *name)
{
    if (names_len == names_cap) {
        size_t cap = names_cap ? names_cap * 2 : 256;
        char **grown = realloc(names, cap * sizeof *grown);
        if (grown == NULL)
            return;
        names = grown;
        names_cap = cap;
    }
    char *copy = strdup(name);
    if (copy != NULL)
        names[names_len++] = copy;
}

static void add_path_dir(const char *dir)
{
    char path[PATH_MAX];
    struct stat st;
    DIR *d = opendir(dir);

    if (d == NULL)
        return;
    for (struct dirent *e; (e = readdir(d)) != NULL;) {
        if (e->d_name[0] == '.')
            continue;
        int n = snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
        if (n < 0 || (size_t)n >= sizeof path)
            continue;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode) && access(path, X_OK) == 0)
            add_name(e->d_name);
    }
    closedir(d);
}

static void build_names(void)
{
    for (size_t i = 0; i < commands_count(); i++)
        add_name(commands_at(i)->name);

    const char *env = getenv("PATH");
    char *path = env ? strdup(env) : NULL;
    if (path == NULL)
        return;

    for (char *dir = strtok(path, ":"); dir != NULL; dir = strtok(NULL, ":")) {
        /*
         * WSL appends the Windows PATH (/mnt/c/...). Listing those folders
         * over the Windows file system is slow, so they are skipped.
         */
        if (strncmp(dir, "/mnt/", 5) == 0)
            continue;
        add_path_dir(dir);
    }
    free(path);
}

static char *command_generator(const char *text, int state)
{
    static size_t next, next_alias, len;

    if (state == 0) {
        if (names == NULL)
            build_names();
        next = next_alias = 0;
        len = strlen(text);
    }
    /* Aliases first; they can change while the shell runs, so not cached. */
    while (next_alias < g_settings.alias_names.n) {
        const char *name = g_settings.alias_names.v[next_alias++];
        if (strncmp(name, text, len) == 0)
            return strdup(name);
    }
    while (next < names_len) {
        const char *name = names[next++];
        if (strncmp(name, text, len) == 0)
            return strdup(name);
    }
    return NULL;
}

static char **attempt_completion(const char *text, int start, int end)
{
    (void)end;

    /* Only the first word is a command; after it, complete file names. */
    for (int i = 0; i < start; i++)
        if (rl_line_buffer[i] != ' ' && rl_line_buffer[i] != '\t')
            return NULL;
    /* "./script" or "/usr/bin/..." are paths, not command names. */
    if (strchr(text, '/') != NULL)
        return NULL;

    return rl_completion_matches(text, command_generator);
}

void completion_init(void)
{
    rl_attempted_completion_function = attempt_completion;
}

#else

void completion_init(void)
{
}

#endif
