/*
 * history.c - command history.
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_READLINE
#include <readline/history.h>
#endif

#include "history.h"
#include "config.h"
#include "messages.h"
#include "shell.h"
#include "ui.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define HISTORY_FILE "history"

static char **entries;  /* capacity entries, set from the config */
static int capacity;
static int count;
static int persist;     /* append new lines to the history file */

/* Allocates the list once; the size comes from `gecmis_boyutu`. */
static int ensure_list(void)
{
    if (entries != NULL)
        return 0;
    capacity = g_settings.history_size > 0 ? g_settings.history_size : 1000;
    entries = calloc((size_t)capacity, sizeof *entries);
    return entries ? 0 : -1;
}

/* Adds a line to memory (and to readline), dropping the oldest if full. */
static void push(const char *line)
{
    char *copy = strdup(line);

    if (copy == NULL || ensure_list() != 0) {
        free(copy);
        return;
    }
    if (count == capacity) {
        free(entries[0]);
        memmove(entries, entries + 1, (size_t)(capacity - 1) * sizeof entries[0]);
        count--;
    }
    entries[count++] = copy;
#ifdef HAVE_READLINE
    add_history(line);
#endif
}

/* Rewrites the file with only what is in memory, so it does not grow forever. */
static void rewrite_file(const char *path)
{
    FILE *f = fopen(path, "w");

    if (f == NULL)
        return;
    for (int i = 0; i < count; i++)
        fprintf(f, "%s\n", entries[i]);
    fclose(f);
}

void hist_init(void)
{
    char path[PATH_MAX];
    char line[MAX_LINE];
    int lines = 0;

    if (ensure_list() != 0 || !g_shell.interactive)
        return;
    persist = 1;
#ifdef HAVE_READLINE
    stifle_history(capacity);
#endif
    if (!config_path(path, sizeof path, HISTORY_FILE))
        return;

    FILE *f = fopen(path, "r");
    if (f == NULL)
        return;
    while (fgets(line, sizeof line, f) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        if (line[0] != '\0') {
            push(line);
            lines++;
        }
    }
    fclose(f);

    if (lines > 2 * capacity)
        rewrite_file(path);
}

static int is_blank(const char *s)
{
    for (; *s; s++)
        if (!isspace((unsigned char)*s))
            return 0;
    return 1;
}

static void append_to_file(const char *line)
{
    char path[PATH_MAX];

    if (config_ensure_dir(TRSH_NAME) != 0 ||
        !config_path(path, sizeof path, HISTORY_FILE))
        return;

    FILE *f = fopen(path, "a");
    if (f == NULL)
        return;
    fprintf(f, "%s\n", line);
    fclose(f);
}

void hist_add(const char *line)
{
    if (is_blank(line))
        return;
    if (count > 0 && strcmp(entries[count - 1], line) == 0)
        return;

    push(line);
    if (persist)
        append_to_file(line);
}

static int all_digits(const char *s)
{
    if (*s == '\0')
        return 0;
    for (; *s; s++)
        if (!isdigit((unsigned char)*s))
            return 0;
    return 1;
}

/* Finds the entry a reference such as "!!", "!12", "!-2" or "!ec" names. */
static const char *lookup(const char *ref)
{
    const char *arg = ref + 1;

    if (strcmp(ref, "!!") == 0)
        return count > 0 ? entries[count - 1] : NULL;

    if (all_digits(arg)) {
        long n = strtol(arg, NULL, 10);
        return n >= 1 && n <= count ? entries[n - 1] : NULL;
    }

    if (arg[0] == '-' && all_digits(arg + 1)) {
        long n = strtol(arg + 1, NULL, 10);
        return n >= 1 && n <= count ? entries[count - n] : NULL;
    }

    size_t len = strlen(arg);
    for (int i = count - 1; i >= 0; i--)
        if (strncmp(entries[i], arg, len) == 0)
            return entries[i];
    return NULL;
}

int hist_expand(const char *line, char *out, size_t size)
{
    char ref[64];
    const char *p = line;

    while (*p == ' ' || *p == '\t')
        p++;
    /* A lone "!" (or "! cmd") is left alone, as in bash. */
    if (p[0] != '!' || p[1] == '\0' || p[1] == ' ' || p[1] == '\t')
        return 0;

    size_t len = strcspn(p, " \t");
    if (len >= sizeof ref) {
        ui_error(TRSH_NAME, MSG_HIST_NOT_FOUND, p);
        return -1;
    }
    memcpy(ref, p, len);
    ref[len] = '\0';

    const char *entry = lookup(ref);
    if (entry == NULL) {
        ui_error(ref, "%s", MSG_HIST_NO_MATCH);
        return -1;
    }

    int n = snprintf(out, size, "%s%s", entry, p + len);
    if (n < 0 || (size_t)n >= size) {
        ui_error(TRSH_NAME, MSG_LINE_TOO_LONG, (int)size - 2);
        return -1;
    }
    return 1;
}

int bi_gecmis(int argc, char **argv)
{
    int first = 0;

    if (argc > 2) {
        ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
        return 1;
    }
    if (argc == 2) {
        if (!all_digits(argv[1])) {
            ui_error(argv[0], MSG_BAD_NUMBER, argv[1]);
            return 1;
        }
        long n = strtol(argv[1], NULL, 10);
        if (n < count)
            first = count - (int)n;
    }

    for (int i = first; i < count; i++)
        printf("%s%4d%s  %s\n", GRAY, i + 1, RESET, entries[i]);
    return 0;
}
