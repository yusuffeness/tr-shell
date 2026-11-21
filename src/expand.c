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
#include "lexer.h"
#include "messages.h"
#include "run.h"
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
    int has_content;    /* the word exists even if empty: "" or '' */
    int empty_at;       /* "$@" with no arguments: drop the word if empty */
    int failed;
    struct strvec *out; /* finished words go here; NULL: one string only */
};

/* Adds text that came from quotes or a variable: never a glob pattern. */
static void add_literal(struct expansion *e, const char *s, size_t n)
{
    if (n > 0)
        e->has_content = 1;
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
    e->has_content = 1;
    if (buf_add(&e->value, &c, 1) != 0 || buf_add(&e->pattern, &c, 1) != 0)
        e->failed = 1;
}

/*
 * Ends the current word: pushes it (or the files its pattern matches) to
 * e->out and starts an empty one. A word made only of empty variables
 * disappears, as in bash.
 */
static void finish_word(struct expansion *e)
{
    if (e->out == NULL || e->failed)
        return;
    if (e->has_content && !(e->empty_at && e->value.len == 0)) {
        glob_t g;
        if (e->has_glob && glob(e->pattern.s, 0, NULL, &g) == 0) {
            for (size_t i = 0; i < g.gl_pathc; i++)
                if (strvec_push(e->out, g.gl_pathv[i]) != 0)
                    e->failed = 1;
            globfree(&g);
        } else if (strvec_push(e->out, e->value.s) != 0) {
            /* No match: the pattern is passed on as typed, like bash. */
            e->failed = 1;
        }
    }
    e->value.len = e->pattern.len = 0;
    e->value.s[0] = e->pattern.s[0] = '\0';
    e->has_glob = e->has_content = e->empty_at = 0;
}

static int is_name_start(char c)
{
    return isalpha((unsigned char)c) || c == '_';
}

static int is_name_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

/* Adds text split at spaces, tabs and new lines into separate words. */
static void add_split(struct expansion *e, const char *s)
{
    char *copy = strdup(s);
    int first = 1;

    if (copy == NULL) {
        e->failed = 1;
        return;
    }
    for (char *f = strtok(copy, " \t\n"); f; f = strtok(NULL, " \t\n")) {
        if (!first)
            finish_word(e);
        add_literal(e, f, strlen(f));
        first = 0;
    }
    free(copy);
}

/*
 * $(...): runs the text in a child shell and inserts what it printed,
 * without the trailing new lines. Unquoted, the output is split into
 * words at spaces, tabs and new lines; inside "..." it stays one piece.
 * s points at the '('. Returns the number of characters consumed.
 */
static size_t substitute(struct expansion *e, const char *s, int quoted)
{
    const char *end = lex_skip_subst(s - 1);
    if (end == NULL) {
        add_literal(e, "$", 1);
        return 0;
    }

    size_t len = (size_t)(end - s) - 2;     /* between the parentheses */
    char *text = malloc(len + 1);
    char *output = NULL;
    if (text == NULL) {
        e->failed = 1;
        return (size_t)(end - s);
    }
    memcpy(text, s + 1, len);
    text[len] = '\0';
    /* Like bash, $? is now the status of the substituted command. */
    g_shell.last_status = run_capture(text, &output);
    free(text);
    if (output == NULL) {
        e->failed = 1;
        return (size_t)(end - s);
    }

    size_t n = strlen(output);
    while (n > 0 && output[n - 1] == '\n')
        output[--n] = '\0';

    if (quoted || e->out == NULL) {
        add_literal(e, output, n);
    } else {
        add_split(e, output);
    }
    free(output);
    return (size_t)(end - s);
}

/* $0..$9, $# and $@ of a script. */
static size_t expand_param(struct expansion *e, const char *s, int quoted)
{
    char num[16];

    if (*s == '#') {
        snprintf(num, sizeof num, "%d", g_shell.nparams);
        add_literal(e, num, strlen(num));
    } else if (*s == '@') {
        if (g_shell.nparams == 0)
            e->empty_at = 1;
        /* One word per argument; unquoted, each is split further. */
        for (int i = 0; i < g_shell.nparams; i++) {
            if (i > 0) {
                if (e->out != NULL)
                    finish_word(e);
                else
                    add_literal(e, " ", 1);
            }
            if (quoted || e->out == NULL)
                add_literal(e, g_shell.params[i], strlen(g_shell.params[i]));
            else
                add_split(e, g_shell.params[i]);
        }
    } else if (*s == '0') {
        const char *p0 = g_shell.param0 ? g_shell.param0 : "trsh";
        add_literal(e, p0, strlen(p0));
    } else {
        int i = *s - '1';
        if (i < g_shell.nparams)
            add_literal(e, g_shell.params[i], strlen(g_shell.params[i]));
    }
    return 1;
}

/*
 * Expands the reference starting at s (just after '$').
 * Returns the number of characters consumed.
 */
static size_t expand_var(struct expansion *e, const char *s, int quoted)
{
    char num[32];
    char name[256];
    size_t n = 0;

    if (*s == '(')
        return substitute(e, s, quoted);
    if (*s == '#' || *s == '@' || isdigit((unsigned char)*s))
        return expand_param(e, s, quoted);
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

static void expand_into(const char *raw, struct expansion *e, struct strvec *out)
{
    const char *s = raw;

    memset(e, 0, sizeof *e);
    e->out = out;
    /* Make sure both strings exist even for an empty word (""). */
    if (buf_add(&e->value, "", 0) != 0 || buf_add(&e->pattern, "", 0) != 0) {
        e->failed = 1;
        return;
    }

    if (s[0] == '~' && (s[1] == '/' || s[1] == '\0')) {
        const char *home = getenv("HOME");
        add_literal(e, home ? home : "~", strlen(home ? home : "~"));
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
            e->has_content = 1;
            add_literal(e, s + 1, n);
            s += n + 1 + (close != NULL);
        } else if (*s == '"') {
            e->has_content = 1;
            s++;
            while (*s && *s != '"') {
                if (*s == '\\' && (s[1] == '"' || s[1] == '\\' || s[1] == '$')) {
                    add_literal(e, s + 1, 1);
                    s += 2;
                } else if (*s == '$') {
                    s += 1 + expand_var(e, s + 1, 1);
                } else {
                    add_literal(e, s, 1);
                    s++;
                }
            }
            if (*s == '"')
                s++;
        } else if (*s == '$') {
            s += 1 + expand_var(e, s + 1, 0);
        } else {
            add_plain(e, *s);
            s++;
        }
    }
}

int expand_word(const char *raw, struct strvec *out)
{
    struct expansion e;

    expand_into(raw, &e, out);
    finish_word(&e);
    free(e.value.s);
    free(e.pattern.s);
    if (e.failed) {
        ui_error(TRSH_NAME, "%s", MSG_NO_MEMORY);
        return -1;
    }
    return 0;
}

char *expand_single(const char *raw)
{
    struct expansion e;

    expand_into(raw, &e, NULL);
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
