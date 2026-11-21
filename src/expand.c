/*
 * expand.c - turning a raw word into the arguments a program receives.
 *
 * One pass over the word builds two strings side by side:
 *   value    the word as the program will see it
 *   pattern  the same, but with quoted glob characters escaped by a
 *            backslash, so that glob() treats only unquoted * ? [ as special
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "expand.h"
#include "messages.h"
#include "shell.h"
#include "ui.h"

struct buf {
    char *s;
    size_t len, cap;
};

static int buf_add(struct buf *b, const char *s, size_t n)
{
    if (b->len + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 64;
        while (cap < b->len + n + 1)
            cap *= 2;
        char *grown = realloc(b->s, cap);
        if (grown == NULL)
            return -1;
        b->s = grown;
        b->cap = cap;
    }
    memcpy(b->s + b->len, s, n);
    b->len += n;
    b->s[b->len] = '\0';
    return 0;
}

static int is_glob_char(char c)
{
    return c == '*' || c == '?' || c == '[' || c == ']' || c == '\\';
}

struct expansion {
    struct buf value;
    struct buf pattern;
    int has_glob;       /* an unquoted * ? or [ was seen */
    int failed;
};

/* Adds text that came from quotes or a variable: never a glob pattern. */
static void add_literal(struct expansion *e, const char *s, size_t n)
{
    if (buf_add(&e->value, s, n) != 0)
        e->failed = 1;
    for (size_t i = 0; i < n; i++) {
        if (is_glob_char(s[i]) && buf_add(&e->pattern, "\\", 1) != 0)
            e->failed = 1;
        if (buf_add(&e->pattern, s + i, 1) != 0)
            e->failed = 1;
    }
}

/*
 * Adds one unquoted character: it goes into the pattern as is, so * ? [ ]
 * keep their meaning. (An unquoted backslash never gets here.)
 */
static void add_plain(struct expansion *e, char c)
{
    if (c == '*' || c == '?' || c == '[')
        e->has_glob = 1;
    if (buf_add(&e->value, &c, 1) != 0 || buf_add(&e->pattern, &c, 1) != 0)
        e->failed = 1;
}

static int is_name_start(char c)
{
    return isalpha((unsigned char)c) || c == '_';
}

static int is_name_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

/*
 * Expands the variable reference starting at s (just after '$').
 * Returns the number of characters consumed.
 */
static size_t expand_var(struct expansion *e, const char *s)
{
    char num[32];
    char name[256];
    size_t n = 0;

    if (*s == '?') {
        snprintf(num, sizeof num, "%d", g_shell.last_status);
        add_literal(e, num, strlen(num));
        return 1;
    }
    if (*s == '$') {
        snprintf(num, sizeof num, "%ld", (long)getpid());
        add_literal(e, num, strlen(num));
        return 1;
    }
    if (*s == '{') {
        const char *close = strchr(s, '}');
        if (close == NULL || close == s + 1 || (size_t)(close - s - 1) >= sizeof name) {
            add_literal(e, "$", 1);     /* not a valid ${...}: keep the $ */
            return 0;
        }
        n = (size_t)(close - s - 1);
        memcpy(name, s + 1, n);
        name[n] = '\0';
        const char *v = getenv(name);
        if (v)
            add_literal(e, v, strlen(v));
        return n + 2;
    }
    if (!is_name_start(*s)) {
        add_literal(e, "$", 1);         /* a lone $ is just a dollar sign */
        return 0;
    }
    while (is_name_char(s[n]) && n + 1 < sizeof name) {
        name[n] = s[n];
        n++;
    }
    name[n] = '\0';
    const char *v = getenv(name);
    if (v)
        add_literal(e, v, strlen(v));
    return n;
}

static void expand_into(const char *raw, struct expansion *e)
{
    const char *s = raw;

    memset(e, 0, sizeof *e);
    /* Make sure both strings exist even for an empty word (""). */
    if (buf_add(&e->value, "", 0) != 0 || buf_add(&e->pattern, "", 0) != 0)
        e->failed = 1;

    if (s[0] == '~' && (s[1] == '/' || s[1] == '\0')) {
        const char *home = getenv("HOME");
        if (home)
            add_literal(e, home, strlen(home));
        else
            add_literal(e, "~", 1);
        s++;
    }

    while (*s) {
        if (*s == '\\') {
            if (s[1]) {
                add_literal(e, s + 1, 1);
                s += 2;
            } else {
                s++;
            }
        } else if (*s == '\'') {
            const char *close = strchr(s + 1, '\'');
            size_t n = close ? (size_t)(close - s - 1) : strlen(s + 1);
            add_literal(e, s + 1, n);
            s += n + 1 + (close != NULL);
        } else if (*s == '"') {
            s++;
            while (*s && *s != '"') {
                if (*s == '\\' && (s[1] == '"' || s[1] == '\\' || s[1] == '$')) {
                    add_literal(e, s + 1, 1);
                    s += 2;
                } else if (*s == '$') {
                    s += 1 + expand_var(e, s + 1);
                } else {
                    add_literal(e, s, 1);
                    s++;
                }
            }
            if (*s == '"')
                s++;
        } else if (*s == '$') {
            s += 1 + expand_var(e, s + 1);
        } else {
            add_plain(e, *s);
            s++;
        }
    }
}

int expand_word(const char *raw, struct strvec *out)
{
    struct expansion e;
    int status = 0;

    expand_into(raw, &e);
    if (e.failed) {
        ui_error(TRSH_NAME, "%s", MSG_NO_MEMORY);
        status = -1;
    } else if (e.has_glob) {
        glob_t g;
        if (glob(e.pattern.s, 0, NULL, &g) == 0) {
            for (size_t i = 0; i < g.gl_pathc && status == 0; i++)
                status = strvec_push(out, g.gl_pathv[i]);
            globfree(&g);
        } else {
            /* No match: the pattern is passed on as typed, like bash. */
            status = strvec_push(out, e.value.s);
        }
    } else {
        status = strvec_push(out, e.value.s);
    }
    free(e.value.s);
    free(e.pattern.s);
    return status;
}

char *expand_single(const char *raw)
{
    struct expansion e;

    expand_into(raw, &e);
    free(e.pattern.s);
    if (e.failed) {
        free(e.value.s);
        ui_error(TRSH_NAME, "%s", MSG_NO_MEMORY);
        return NULL;
    }
    return e.value.s;
}

int is_assignment(const char *raw)
{
    if (!is_name_start(raw[0]))
        return 0;
    for (const char *s = raw + 1; *s; s++) {
        if (*s == '=')
            return 1;
        if (!is_name_char(*s))
            return 0;
    }
    return 0;
}
