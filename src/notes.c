/*
 * notes.c - quick notes kept in ~/.trsh/notlar.
 *
 * One note per line: "<unix time>\t<text>".
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "notes.h"
#include "config.h"
#include "messages.h"
#include "shell.h"
#include "ui.h"
#include "util.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define NOTES_FILE "notlar"
#define MAX_NOTES  1000

struct note {
    long when;
    char text[MAX_LINE];
};

static int notes_path(char *buf, size_t size)
{
    return config_path(buf, size, NOTES_FILE);
}

/* Loads up to MAX_NOTES notes into a newly allocated array. */
static int load(struct note **out, size_t *count)
{
    char path[PATH_MAX];
    char line[MAX_LINE + 32];

    *out = NULL;
    *count = 0;
    if (!notes_path(path, sizeof path))
        return -1;

    FILE *f = fopen(path, "r");
    if (f == NULL)
        return errno == ENOENT ? 0 : -1;

    struct note *v = calloc(MAX_NOTES, sizeof *v);
    if (v == NULL) {
        fclose(f);
        return -1;
    }
    size_t n = 0;
    while (n < MAX_NOTES && fgets(line, sizeof line, f) != NULL) {
        char *tab = strchr(line, '\t');
        if (tab == NULL)
            continue;
        *tab = '\0';
        tab[1 + strcspn(tab + 1, "\n")] = '\0';
        v[n].when = atol(line);
        snprintf(v[n].text, sizeof v[n].text, "%s", tab + 1);
        n++;
    }
    fclose(f);
    *out = v;
    *count = n;
    return 0;
}

static int save(const struct note *v, size_t n)
{
    char path[PATH_MAX];

    if (!notes_path(path, sizeof path))
        return -1;
    FILE *f = fopen(path, "w");
    if (f == NULL)
        return -1;
    for (size_t i = 0; i < n; i++)
        fprintf(f, "%ld\t%s\n", v[i].when, v[i].text);
    return fclose(f);
}

int bi_not(int argc, char **argv)
{
    char path[PATH_MAX];
    char text[MAX_LINE];
    struct note *v;
    size_t n;

    if (argc < 2) {
        ui_error(argv[0], "%s %s", MSG_HELP_USAGE, "not METİN...");
        return 1;
    }
    if (!join_args(text, sizeof text, argc - 1, argv + 1)) {
        ui_error(argv[0], MSG_LINE_TOO_LONG, MAX_LINE - 2);
        return 1;
    }
    if (config_ensure_dir(argv[0]) != 0 || !notes_path(path, sizeof path))
        return 1;

    FILE *f = fopen(path, "a");
    if (f == NULL) {
        ui_error(argv[0], "%s: %s", path, msg_error(errno));
        return 1;
    }
    fprintf(f, "%ld\t%s\n", (long)time(NULL), text);
    if (fclose(f) != 0) {
        ui_error(argv[0], "%s: %s", path, msg_error(errno));
        return 1;
    }

    /* Tell the user which number the new note got. */
    n = 0;
    if (load(&v, &n) == 0)
        free(v);
    printf(MSG_NOTE_ADDED "\n", n);
    return 0;
}

static int list_notes(void)
{
    struct note *v;
    size_t n;
    char date[32];

    if (load(&v, &n) != 0) {
        ui_error("notlar", "%s", MSG_NOTES_READ_FAIL);
        return 1;
    }
    if (n == 0)
        printf("%s\n", MSG_NOTES_EMPTY);
    for (size_t i = 0; i < n; i++) {
        fmt_short_date(date, sizeof date, (time_t)v[i].when);
        printf("  %s%3zu%s  %s%s%s  %s\n", GRAY, i + 1, RESET, GRAY, date, RESET,
               v[i].text);
    }
    free(v);
    return 0;
}

static int delete_note(const char *arg)
{
    struct note *v;
    size_t n;
    char *end;
    long k = strtol(arg, &end, 10);

    if (*arg == '\0' || *end != '\0') {
        ui_error("notlar", MSG_BAD_NUMBER, arg);
        return 1;
    }
    if (load(&v, &n) != 0) {
        ui_error("notlar", "%s", MSG_NOTES_READ_FAIL);
        return 1;
    }
    if (k < 1 || (size_t)k > n) {
        ui_error("notlar", MSG_NOTE_NO_SUCH, k);
        free(v);
        return 1;
    }
    memmove(&v[k - 1], &v[k], (n - (size_t)k) * sizeof *v);
    int status = save(v, n - 1) == 0 ? 0 : 1;
    if (status == 0)
        printf(MSG_NOTE_DELETED "\n", k);
    free(v);
    return status;
}

int bi_notlar(int argc, char **argv)
{
    if (argc == 1)
        return list_notes();
    if (strcmp(argv[1], "sil") == 0 && argc == 3)
        return delete_note(argv[2]);
    if (strcmp(argv[1], "sil") == 0) {
        ui_error(argv[0], "%s %s", MSG_HELP_USAGE, "notlar sil NO");
        return 1;
    }
    ui_error(argv[0], MSG_UNKNOWN_SUBCMD, argv[1]);
    return 1;
}
