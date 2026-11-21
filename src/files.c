/*
 * files.c - `listele` and `agac`, written in C instead of calling ls/tree.
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "files.h"
#include "messages.h"
#include "ui.h"
#include "util.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define TREE_DEFAULT_DEPTH 3

struct entry {
    char *name;
    struct stat st;     /* from lstat: links are not followed */
};

struct list {
    struct entry *items;
    size_t len, cap;
};

/* ---------------------------------------------------------------- helpers */

static void list_free(struct list *l)
{
    for (size_t i = 0; i < l->len; i++)
        free(l->items[i].name);
    free(l->items);
    l->items = NULL;
    l->len = l->cap = 0;
}

static int list_push(struct list *l, const char *name, const struct stat *st)
{
    if (l->len == l->cap) {
        size_t cap = l->cap ? l->cap * 2 : 32;
        struct entry *grown = realloc(l->items, cap * sizeof *grown);
        if (grown == NULL)
            return -1;
        l->items = grown;
        l->cap = cap;
    }
    char *copy = strdup(name);
    if (copy == NULL)
        return -1;
    l->items[l->len].name = copy;
    l->items[l->len].st = *st;
    l->len++;
    return 0;
}

static int is_dir(const struct entry *e)
{
    return S_ISDIR(e->st.st_mode);
}

/*
 * "Natural" order, ignoring ASCII case: runs of digits compare by value,
 * so dosya_2 comes before dosya_10.
 */
static int natural_compare(const char *a, const char *b)
{
    while (*a && *b) {
        if (isdigit((unsigned char)*a) && isdigit((unsigned char)*b)) {
            while (*a == '0')
                a++;
            while (*b == '0')
                b++;
            size_t la = strspn(a, "0123456789");
            size_t lb = strspn(b, "0123456789");
            if (la != lb)
                return la < lb ? -1 : 1;
            int c = strncmp(a, b, la);
            if (c != 0)
                return c;
            a += la;
            b += lb;
            continue;
        }
        int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
        if (ca != cb)
            return ca < cb ? -1 : 1;
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/* Directories first, then natural order by name. */
static int compare_entries(const void *a, const void *b)
{
    const struct entry *x = a, *y = b;

    if (is_dir(x) != is_dir(y))
        return is_dir(x) ? -1 : 1;
    int c = natural_compare(x->name, y->name);
    return c ? c : strcmp(x->name, y->name);
}

static int join_path(char *buf, size_t size, const char *dir, const char *name)
{
    size_t dl = strlen(dir);
    int n = snprintf(buf, size, "%s%s%s", dir,
                     dl > 0 && dir[dl - 1] == '/' ? "" : "/", name);
    return n >= 0 && (size_t)n < size;
}

/* Reads a directory into l, sorted. Returns 0, or -1 with errno set. */
static int read_dir(const char *path, int all, struct list *l)
{
    char full[PATH_MAX];
    struct stat st;
    DIR *d = opendir(path);

    if (d == NULL)
        return -1;
    for (struct dirent *e; (e = readdir(d)) != NULL;) {
        const char *n = e->d_name;
        if (strcmp(n, ".") == 0 || strcmp(n, "..") == 0)
            continue;
        if (n[0] == '.' && !all)
            continue;
        if (!join_path(full, sizeof full, path, n) || lstat(full, &st) != 0)
            continue;
        if (list_push(l, n, &st) != 0)
            break;
    }
    closedir(d);
    /* items is NULL for an empty folder, and qsort must not get NULL. */
    if (l->len > 1)
        qsort(l->items, l->len, sizeof l->items[0], compare_entries);
    return 0;
}

static const char *entry_color(const struct stat *st)
{
    if (S_ISDIR(st->st_mode))
        return DIR_COLOR;
    if (S_ISLNK(st->st_mode))
        return LINK_COLOR;
    if (S_ISREG(st->st_mode) && (st->st_mode & S_IXUSR))
        return EXEC_COLOR;
    return "";
}

/* Prints a name with its color and a trailing / for directories. */
static int print_name(const char *name, const struct stat *st)
{
    int dir = S_ISDIR(st->st_mode);

    printf("%s%s%s%s", entry_color(st), name, dir ? "/" : "", RESET);
    return text_width(name) + dir;
}

/* Parses -a / -d / -l style flags. Returns 0, or -1 for an unknown flag. */
static int parse_flags(const char *who, const char *arg, int *all, int *detail)
{
    for (const char *f = arg + 1; *f; f++) {
        if (*f == 'a')
            *all = 1;
        else if (*f == 'd' || *f == 'l')
            *detail = 1;
        else {
            ui_error(who, MSG_UNKNOWN_OPTION, *f);
            return -1;
        }
    }
    return 0;
}

static int is_flag(const char *arg)
{
    return arg[0] == '-' && arg[1] != '\0';
}

/* --------------------------------------------------------------- listele */

static void mode_string(char *buf, mode_t m)
{
    buf[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : '-';
    buf[1] = (m & S_IRUSR) ? 'r' : '-';
    buf[2] = (m & S_IWUSR) ? 'w' : '-';
    buf[3] = (m & S_IXUSR) ? 'x' : '-';
    buf[4] = (m & S_IRGRP) ? 'r' : '-';
    buf[5] = (m & S_IWGRP) ? 'w' : '-';
    buf[6] = (m & S_IXGRP) ? 'x' : '-';
    buf[7] = (m & S_IROTH) ? 'r' : '-';
    buf[8] = (m & S_IWOTH) ? 'w' : '-';
    buf[9] = (m & S_IXOTH) ? 'x' : '-';
    buf[10] = '\0';
}

static void print_detail(const char *dir, const struct entry *e,
                         long long *bytes, int *dirs, int *files)
{
    char mode[11], size[16], date[32];

    mode_string(mode, e->st.st_mode);
    fmt_short_date(date, sizeof date, e->st.st_mtime);
    if (is_dir(e)) {
        snprintf(size, sizeof size, "-");
        (*dirs)++;
    } else {
        fmt_size(size, sizeof size, (long long)e->st.st_size);
        *bytes += (long long)e->st.st_size;
        (*files)++;
    }

    printf("%s%s%s  %8s  %s%s%s  ", GRAY, mode, RESET, size, GRAY, date, RESET);
    print_name(e->name, &e->st);

    if (S_ISLNK(e->st.st_mode) && dir != NULL) {
        char path[PATH_MAX], target[PATH_MAX];
        ssize_t n;
        if (join_path(path, sizeof path, dir, e->name) &&
            (n = readlink(path, target, sizeof target - 1)) >= 0) {
            target[n] = '\0';
            printf(" %s→ %s%s", GRAY, target, RESET);
        }
    }
    putchar('\n');
}

static void list_detailed(const char *dir, const struct list *l)
{
    long long bytes = 0;
    int dirs = 0, files = 0;
    char total[16];

    for (size_t i = 0; i < l->len; i++)
        print_detail(dir, &l->items[i], &bytes, &dirs, &files);
    if (l->len > 0) {
        fmt_size(total, sizeof total, bytes);
        printf("%s" MSG_LIST_TOTAL "%s\n", GRAY, dirs, files, total, RESET);
    }
}

/* Names in columns (top to bottom, then left to right), like ls. */
static void list_columns(const struct list *l)
{
    int width = 0;

    for (size_t i = 0; i < l->len; i++) {
        int w = text_width(l->items[i].name) + is_dir(&l->items[i]);
        if (w > width)
            width = w;
    }
    int colw = width + 2;
    int cols = ui_term_cols() / colw;
    if (cols < 1)
        cols = 1;
    size_t rows = (l->len + (size_t)cols - 1) / (size_t)cols;

    for (size_t r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            size_t i = (size_t)c * rows + r;
            if (i >= l->len)
                break;
            int used = print_name(l->items[i].name, &l->items[i].st);
            if ((size_t)(c + 1) * rows + r < l->len)
                printf("%*s", colw - used, "");
        }
        putchar('\n');
    }
}

static void list_print(const char *dir, const struct list *l, int detail)
{
    if (detail) {
        list_detailed(dir, l);
    } else if (isatty(STDOUT_FILENO)) {
        list_columns(l);
    } else {
        /* One name per line when piped, so other programs can read it. */
        for (size_t i = 0; i < l->len; i++) {
            print_name(l->items[i].name, &l->items[i].st);
            putchar('\n');
        }
    }
}

static int list_one(const char *path, int all, int detail, int header)
{
    struct stat st, target;
    struct list l = { 0 };

    if (lstat(path, &st) != 0) {
        ui_error("listele", "%s: %s", path, msg_error(errno));
        return 1;
    }
    /* A link to a directory is listed like the directory itself. */
    int dir = S_ISDIR(st.st_mode) ||
              (S_ISLNK(st.st_mode) && stat(path, &target) == 0 && S_ISDIR(target.st_mode));

    if (!dir) {
        list_push(&l, path, &st);
        list_print(NULL, &l, detail);
        list_free(&l);
        return 0;
    }

    if (read_dir(path, all, &l) != 0) {
        ui_error("listele", "%s: %s", path, msg_error(errno));
        return 1;
    }
    if (header)
        printf("%s%s:%s\n", BOLD, path, RESET);
    list_print(path, &l, detail);
    list_free(&l);
    return 0;
}

int bi_listele(int argc, char **argv)
{
    int all = 0, detail = 0, paths = 0, status = 0;

    for (int i = 1; i < argc; i++) {
        if (is_flag(argv[i])) {
            if (parse_flags(argv[0], argv[i], &all, &detail) != 0)
                return 1;
        } else {
            paths++;
        }
    }
    if (paths == 0)
        return list_one(".", all, detail, 0);

    int shown = 0;
    for (int i = 1; i < argc; i++) {
        if (is_flag(argv[i]))
            continue;
        if (shown++ > 0 && paths > 1)
            putchar('\n');
        status |= list_one(argv[i], all, detail, paths > 1);
    }
    return status;
}

/* ------------------------------------------------------------------ agac */

struct tree {
    int all;
    int max_depth;
    int dirs, files;
};

static void tree_walk(struct tree *t, const char *path, char *prefix,
                      size_t prefix_len, int depth)
{
    struct list l = { 0 };
    char child[PATH_MAX];

    if (read_dir(path, t->all, &l) != 0)
        return;

    for (size_t i = 0; i < l.len; i++) {
        const struct entry *e = &l.items[i];
        int last = i + 1 == l.len;

        printf("%s%s%s%s", GRAY, prefix, last ? "└── " : "├── ", RESET);
        print_name(e->name, &e->st);
        putchar('\n');

        if (!is_dir(e)) {
            t->files++;
            continue;
        }
        t->dirs++;
        if (depth >= t->max_depth || !join_path(child, sizeof child, path, e->name))
            continue;

        /* Extend the prefix for the children, then cut it back. */
        const char *more = last ? "    " : "│   ";
        size_t more_len = strlen(more);
        if (prefix_len + more_len + 1 > PATH_MAX)
            continue;
        memcpy(prefix + prefix_len, more, more_len + 1);
        tree_walk(t, child, prefix, prefix_len + more_len, depth + 1);
        prefix[prefix_len] = '\0';
    }
    list_free(&l);
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

int bi_agac(int argc, char **argv)
{
    struct tree t = { 0, TREE_DEFAULT_DEPTH, 0, 0 };
    const char *path = NULL;
    int depth_given = 0;
    struct stat st;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (is_flag(a)) {
            if (strcmp(a, "-a") != 0) {
                ui_error(argv[0], MSG_UNKNOWN_OPTION, a[1]);
                return 1;
            }
            t.all = 1;
        } else if (all_digits(a) && !depth_given &&
                   (path != NULL || stat(a, &st) != 0)) {
            /* A number is the depth, unless it names an existing folder. */
            t.max_depth = atoi(a);
            depth_given = 1;
        } else if (path == NULL) {
            path = a;
        } else {
            ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
            return 1;
        }
    }
    if (path == NULL)
        path = ".";
    if (t.max_depth < 1)
        t.max_depth = 1;

    if (stat(path, &st) != 0) {
        ui_error(argv[0], "%s: %s", path, msg_error(errno));
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        ui_error(argv[0], "%s: %s", path, msg_error(ENOTDIR));
        return 1;
    }

    char prefix[PATH_MAX] = "";
    printf("%s%s%s\n", DIR_COLOR, path, RESET);
    tree_walk(&t, path, prefix, 0, 1);
    printf("\n%s" MSG_TREE_TOTAL "%s\n", GRAY, t.dirs, t.files, RESET);
    return 0;
}
