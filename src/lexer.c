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

/*
 * Finds the end of the word starting at s. Quotes and backslashes are
 * skipped over (but kept in the word). Returns NULL on an unclosed quote.
 */
static const char *word_end(const char *s, char *unclosed)
{
    while (*s && *s != ' ' && *s != '\t' && *s != '\n' && !is_operator_char(*s)) {
        if (*s == '\\') {
            s++;
            if (*s)
                s++;
        } else if (*s == '\'') {
            const char *close = strchr(s + 1, '\'');
            if (close == NULL) {
                *unclosed = '\'';
                return NULL;
            }
            s = close + 1;
        } else if (*s == '"') {
            s++;
            while (*s && *s != '"') {
                if (*s == '\\' && s[1])
                    s++;
                s++;
            }
            if (*s != '"') {
                *unclosed = '"';
                return NULL;
            }
            s++;
        } else {
            s++;
        }
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
        while (*s == ' ' || *s == '\t' || *s == '\n')
            s++;
        if (*s == '\0' || *s == '#')
            break;

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
