/*
 * alias.c - user-defined commands (takma adlar).
 *
 * Expansion works on tokens: whenever a plain word stands where a command
 * starts (the beginning of the line, or after | || && ; &), and an alias
 * has that name, the word is replaced by the tokens of the alias text.
 * Those tokens are expanded again, except for aliases already being
 * expanded, so `takmaad ls=ls -F` does not loop forever.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alias.h"
#include "config.h"
#include "messages.h"
#include "ui.h"
#include "util.h"

#define MAX_DEPTH 16

static int starts_command(enum tok_type t)
{
    return t == T_PIPE || t == T_OR || t == T_AND || t == T_AMP || t == T_SEMI;
}

static int append(struct tokens *out, const struct token *tok)
{
    if (out->n == out->cap) {
        size_t cap = out->cap ? out->cap * 2 : 16;
        struct token *grown = realloc(out->v, cap * sizeof *grown);
        if (grown == NULL)
            return -1;
        out->v = grown;
        out->cap = cap;
    }
    char *text = strdup(tok->text);
    if (text == NULL)
        return -1;
    out->v[out->n].type = tok->type;
    out->v[out->n].text = text;
    out->n++;
    return 0;
}

/* A word may name an alias only if it has no quotes or backslashes. */
static int plain_word(const char *w)
{
    return strpbrk(w, "'\"\\") == NULL;
}

/*
 * Copies tokens [0, n) of in to out, expanding aliases. active holds the
 * names being expanded right now (depth entries). Returns 0 or -1.
 */
static int expand_into(const struct token *in, size_t n, struct tokens *out,
                       const char **active, int depth)
{
    int at_start = 1;

    for (size_t i = 0; i < n; i++) {
        const struct token *tok = &in[i];
        const char *value = NULL;

        if (at_start && tok->type == T_WORD && plain_word(tok->text))
            value = alias_get(tok->text);
        for (int d = 0; value != NULL && d < depth; d++)
            if (strcmp(active[d], tok->text) == 0)
                value = NULL;   /* already inside this alias */

        if (value == NULL || depth >= MAX_DEPTH) {
            if (append(out, tok) != 0)
                return -1;
            at_start = starts_command(tok->type);
            continue;
        }

        struct tokens sub;
        if (lex(value, &sub) != 0)
            return -1;
        active[depth] = tok->text;
        size_t before = out->n;
        int r = expand_into(sub.v, sub.n - 1, out, active, depth + 1);  /* skip T_END */
        tokens_free(&sub);
        if (r != 0)
            return -1;
        /* An alias that ends in "|" or "&&" leaves us at a command start. */
        at_start = out->n == before || starts_command(out->v[out->n - 1].type);
    }
    return 0;
}

int alias_expand(struct tokens *t)
{
    const char *active[MAX_DEPTH];
    struct tokens out = { NULL, 0, 0 };

    if (g_settings.alias_names.n == 0)
        return 0;
    /* Expand everything but the final T_END, then put it back. */
    if (expand_into(t->v, t->n - 1, &out, active, 0) != 0 ||
        append(&out, &t->v[t->n - 1]) != 0) {
        tokens_free(&out);
        ui_error(TRSH_NAME, "%s", MSG_NO_MEMORY);
        return -1;
    }
    tokens_free(t);
    *t = out;
    return 0;
}

/* -------------------------------------------------------------- builtins */

static void list_aliases(void)
{
    if (g_settings.alias_names.n == 0) {
        printf("%s\n", MSG_ALIAS_NONE);
        return;
    }
    for (size_t i = 0; i < g_settings.alias_names.n; i++)
        printf("  %s%s%s = %s\n", GREEN, g_settings.alias_names.v[i], RESET,
               g_settings.alias_values.v[i]);
}

int bi_takmaad(int argc, char **argv)
{
    char def[MAX_LINE];

    if (argc == 1) {
        list_aliases();
        return 0;
    }
    /* takmaad ll=listele -d   and   takmaad ll='listele -d'   both work. */
    if (!join_args(def, sizeof def, argc - 1, argv + 1)) {
        ui_error(argv[0], MSG_LINE_TOO_LONG, MAX_LINE - 2);
        return 1;
    }
    char *eq = strchr(def, '=');
    if (eq == NULL) {
        const char *v = alias_get(def);
        if (v == NULL) {
            ui_error(argv[0], MSG_ALIAS_NO_SUCH, def);
            return 1;
        }
        printf("  %s%s%s = %s\n", GREEN, def, RESET, v);
        return 0;
    }

    *eq = '\0';
    const char *name = def, *value = eq + 1;
    if (!alias_name_ok(name)) {
        ui_error(argv[0], MSG_ALIAS_BAD_NAME, name);
        return 1;
    }
    if (*value == '\0') {
        ui_error(argv[0], MSG_ALIAS_EMPTY, name);
        return 1;
    }
    if (alias_define(name, value) != 0) {
        ui_error(argv[0], "%s", MSG_NO_MEMORY);
        return 1;
    }
    return config_save(argv[0]);
}

int bi_takmaadsil(int argc, char **argv)
{
    int status = 0, changed = 0;

    if (argc < 2) {
        ui_error(argv[0], "%s %s", MSG_HELP_USAGE, "takmaadsil AD...");
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        if (alias_remove(argv[i]) != 0) {
            ui_error(argv[0], MSG_ALIAS_NO_SUCH, argv[i]);
            status = 1;
        } else {
            changed = 1;
        }
    }
    if (changed && config_save(argv[0]) != 0)
        status = 1;
    return status;
}
