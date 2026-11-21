/*
 * lexer.c - splitting a command line into tokens.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "messages.h"
#include "shell.h"
#include "ui.h"

static int push(struct tokens *t, enum tok_type type, const char *start, size_t len)
{
    if (t->n == t->cap) {
        size_t cap = t->cap ? t->cap * 2 : 16;
        struct token *grown = realloc(t->v, cap * sizeof *grown);
        if (grown == NULL)
            return -1;
        t->v = grown;
        t->cap = cap;
    }
    char *text = malloc(len + 1);
    if (text == NULL)
        return -1;
    memcpy(text, start, len);
    text[len] = '\0';
    t->v[t->n].type = type;
    t->v[t->n].text = text;
    t->n++;
    return 0;
}

void tokens_free(struct tokens *t)
{
    for (size_t i = 0; i < t->n; i++)
        free(t->v[i].text);
    free(t->v);
    t->v = NULL;
    t->n = t->cap = 0;
}

static int is_operator_char(char c)
{
    return c == '|' || c == '&' || c == ';' || c == '<' || c == '>';
}

/* Recognizes an operator at s; returns its length (0 if none). */
static size_t match_operator(const char *s, enum tok_type *type)
{
    static const struct { const char *text; enum tok_type type; } ops[] = {
        /* longest first */
        { "2>&1", T_ERR_TO_OUT }, { "2>>", T_ERR_DGREAT }, { "2>", T_ERR_GREAT },
        { "||", T_OR }, { "&&", T_AND }, { ">>", T_DGREAT },
        { "|", T_PIPE }, { "&", T_AMP }, { ";", T_SEMI },
        { "<", T_LESS }, { ">", T_GREAT },
    };

    for (size_t i = 0; i < sizeof ops / sizeof ops[0]; i++) {
        size_t len = strlen(ops[i].text);
        if (strncmp(s, ops[i].text, len) == 0) {
            *type = ops[i].type;
            return len;
        }
    }
    return 0;
}

static const char *skip_subst(const char *s);

const char *lex_skip_subst(const char *s)
{
    return skip_subst(s);
}

/* Skips a "..." string starting at s (on the quote). NULL if unclosed. */
static const char *skip_dquote(const char *s)
{
    s++;
    while (*s && *s != '"') {
        if (*s == '\\' && s[1]) {
            s += 2;
        } else if (*s == '$' && s[1] == '(') {
            s = skip_subst(s);
            if (s == NULL)
                return NULL;
        } else {
            s++;
        }
    }
    return *s == '"' ? s + 1 : NULL;
}

/*
 * Skips a $(...) starting at s (on the $), allowing nested parentheses,
 * quotes and further $(...) inside. NULL if it is never closed.
 */
static const char *skip_subst(const char *s)
{
    int depth = 1;

    s += 2;
    while (*s) {
        if (*s == '\\' && s[1]) {
            s += 2;
        } else if (*s == '\'') {
            const char *close = strchr(s + 1, '\'');
            if (close == NULL)
                return NULL;
            s = close + 1;
        } else if (*s == '"') {
            s = skip_dquote(s);
            if (s == NULL)
                return NULL;
        } else if (*s == '$' && s[1] == '(') {
            s = skip_subst(s);
            if (s == NULL)
                return NULL;
        } else {
            if (*s == '(')
                depth++;
            else if (*s == ')' && --depth == 0)
                return s + 1;
            s++;
        }
    }
    return NULL;
}

/*
 * Finds the end of the word starting at s. Quotes, backslashes and $(...)
 * are skipped over (but kept in the word). Returns NULL if one of them is
 * never closed, with *unclosed set to the quote or to '('.
 */
static const char *word_end(const char *s, char *unclosed)
{
    while (*s && *s != ' ' && *s != '\t' && *s != '\n' && !is_operator_char(*s)) {
        const char *next;

        if (*s == '\\') {
            next = s[1] ? s + 2 : s + 1;
        } else if (*s == '\'') {
            const char *close = strchr(s + 1, '\'');
            next = close ? close + 1 : NULL;
            *unclosed = '\'';
        } else if (*s == '"') {
            next = skip_dquote(s);
            *unclosed = '"';
        } else if (*s == '$' && s[1] == '(') {
            next = skip_subst(s);
            *unclosed = '(';
        } else {
            next = s + 1;
        }
        if (next == NULL)
            return NULL;
        s = next;
    }
    return s;
}

int lex(const char *line, struct tokens *out)
{
    const char *s = line;
    enum tok_type type;

    out->v = NULL;
    out->n = out->cap = 0;

    for (;;) {
        while (*s == ' ' || *s == '\t')
            s++;
        if (*s == '#')              /* a comment runs to the end of the line */
            s += strcspn(s, "\n");
        if (*s == '\0')
            break;
        if (*s == '\n') {
            /* A new line separates commands like ';' does. */
            if (push(out, T_NEWLINE, ";", 1) != 0)
                goto oom;
            s++;
            continue;
        }

        /*
         * We are at the start of a token, so "2>" here is the stderr
         * operator; inside a word ("a2>f") the 2 stays part of the word.
         */
        size_t len = match_operator(s, &type);
        if (len > 0) {
            if (push(out, type, s, len) != 0)
                goto oom;
            s += len;
            continue;
        }

        char unclosed = 0;
        const char *end = word_end(s, &unclosed);
        if (end == NULL) {
            if (unclosed == '(')
                ui_error(TRSH_NAME, "%s", MSG_UNCLOSED_SUBST);
            else
                ui_error(TRSH_NAME, MSG_UNCLOSED_QUOTE, unclosed);
            tokens_free(out);
            return -1;
        }
        if (push(out, T_WORD, s, (size_t)(end - s)) != 0)
            goto oom;
        s = end;
    }

    if (push(out, T_END, "", 0) != 0)
        goto oom;
    return 0;

oom:
    ui_error(TRSH_NAME, "%s", MSG_NO_MEMORY);
    tokens_free(out);
    return -1;
}
