/*
 * parser.c - building a syntax tree from tokens (recursive descent).
 */
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "messages.h"
#include "shell.h"
#include "ui.h"

struct parser {
    const struct tokens *t;
    size_t pos;
};

static const struct token *peek(const struct parser *p)
{
    return &p->t->v[p->pos];
}

static const struct token *next(struct parser *p)
{
    const struct token *tok = &p->t->v[p->pos];
    if (tok->type != T_END)
        p->pos++;
    return tok;
}

static int syntax_error(const struct token *tok)
{
    if (tok->type == T_END)
        ui_error(TRSH_NAME, "%s", MSG_SYNTAX_END);
    else
        ui_error(TRSH_NAME, MSG_SYNTAX, tok->text);
    return -1;
}

static int is_redirection(enum tok_type t)
{
    return t == T_LESS || t == T_GREAT || t == T_DGREAT || t == T_ERR_GREAT ||
           t == T_ERR_DGREAT || t == T_ERR_TO_OUT;
}

/* Grows an array of elements of size elem to hold n + 1. */
static void *grow(void *array, size_t n, size_t elem)
{
    return realloc(array, (n + 1) * elem);
}

static void command_free(struct command *c)
{
    strvec_free(&c->words);
    for (size_t i = 0; i < c->nredirs; i++)
        free(c->redirs[i].target);
    free(c->redirs);
}

static void pipeline_free(struct pipeline *p)
{
    for (size_t i = 0; i < p->ncmds; i++)
        command_free(&p->cmds[i]);
    free(p->cmds);
}

static void and_or_free(struct and_or *ao)
{
    for (size_t i = 0; i < ao->npipes; i++)
        pipeline_free(&ao->pipes[i]);
    free(ao->pipes);
    free(ao->ops);
}

void script_free(struct script *s)
{
    for (size_t i = 0; i < s->nitems; i++) {
        and_or_free(&s->items[i].ao);
        free(s->items[i].text);
    }
    free(s->items);
    s->items = NULL;
    s->nitems = 0;
}

static int parse_redirection(struct parser *p, struct command *c)
{
    const struct token *op = next(p);
    struct redir r = { R_OUT, 1, NULL };

    switch (op->type) {
    case T_LESS:       r.kind = R_IN;     r.fd = 0; break;
    case T_GREAT:      r.kind = R_OUT;    r.fd = 1; break;
    case T_DGREAT:     r.kind = R_APPEND; r.fd = 1; break;
    case T_ERR_GREAT:  r.kind = R_OUT;    r.fd = 2; break;
    case T_ERR_DGREAT: r.kind = R_APPEND; r.fd = 2; break;
    default:           r.kind = R_ERR_TO_OUT; r.fd = 2; break;
    }

    if (r.kind != R_ERR_TO_OUT) {
        const struct token *target = peek(p);
        if (target->type != T_WORD)
            return syntax_error(target);
        r.target = strdup(next(p)->text);
        if (r.target == NULL)
            return -1;
    }

    struct redir *grown = grow(c->redirs, c->nredirs, sizeof *grown);
    if (grown == NULL) {
        free(r.target);
        return -1;
    }
    c->redirs = grown;
    c->redirs[c->nredirs++] = r;
    return 0;
}

static int parse_command(struct parser *p, struct command *c)
{
    memset(c, 0, sizeof *c);

    for (;;) {
        const struct token *tok = peek(p);

        if (tok->type == T_WORD) {
            if (c->words.n >= MAX_ARGS - 1) {
                ui_error(TRSH_NAME, MSG_TOO_MANY_ARGS, MAX_ARGS - 1);
                return -1;
            }
            if (strvec_push(&c->words, next(p)->text) != 0)
                return -1;
        } else if (is_redirection(tok->type)) {
            if (parse_redirection(p, c) != 0)
                return -1;
        } else {
            break;
        }
    }
    if (c->words.n == 0 && c->nredirs == 0)
        return syntax_error(peek(p));
    return 0;
}

static int parse_pipeline(struct parser *p, struct pipeline *pl)
{
    memset(pl, 0, sizeof *pl);

    for (;;) {
        struct command *grown = grow(pl->cmds, pl->ncmds, sizeof *grown);
        if (grown == NULL)
            return -1;
        pl->cmds = grown;
        if (parse_command(p, &pl->cmds[pl->ncmds]) != 0) {
            command_free(&pl->cmds[pl->ncmds]);
            return -1;
        }
        pl->ncmds++;
        if (peek(p)->type != T_PIPE)
            return 0;
        next(p);
    }
}

static int parse_and_or(struct parser *p, struct and_or *ao)
{
    memset(ao, 0, sizeof *ao);

    for (;;) {
        struct pipeline *grown = grow(ao->pipes, ao->npipes, sizeof *grown);
        if (grown == NULL)
            return -1;
        ao->pipes = grown;
        if (parse_pipeline(p, &ao->pipes[ao->npipes]) != 0) {
            pipeline_free(&ao->pipes[ao->npipes]);
            return -1;
        }
        ao->npipes++;

        enum tok_type t = peek(p)->type;
        if (t != T_AND && t != T_OR)
            return 0;
        next(p);

        enum andor_op *ops = grow(ao->ops, ao->npipes - 1, sizeof *ops);
        if (ops == NULL)
            return -1;
        ao->ops = ops;
        ao->ops[ao->npipes - 1] = t == T_AND ? OP_AND : OP_OR;
    }
}

/* The tokens of an item joined by spaces, as a readable job name. */
static char *join_tokens(const struct tokens *t, size_t from, size_t to)
{
    size_t len = 1;
    for (size_t i = from; i < to; i++)
        len += strlen(t->v[i].text) + 1;

    char *s = malloc(len);
    if (s == NULL)
        return NULL;
    s[0] = '\0';
    for (size_t i = from; i < to; i++) {
        if (i > from)
            strcat(s, " ");
        strcat(s, t->v[i].text);
    }
    return s;
}

int parse(const struct tokens *t, struct script *out)
{
    struct parser p = { t, 0 };

    out->items = NULL;
    out->nitems = 0;

    while (peek(&p)->type != T_END) {
        size_t start = p.pos;
        struct item *grown = grow(out->items, out->nitems, sizeof *grown);
        if (grown == NULL)
            goto fail;
        out->items = grown;

        struct item *it = &out->items[out->nitems];
        memset(it, 0, sizeof *it);
        if (parse_and_or(&p, &it->ao) != 0) {
            and_or_free(&it->ao);
            goto fail;
        }
        it->text = join_tokens(t, start, p.pos);
        out->nitems++;

        const struct token *sep = peek(&p);
        if (sep->type == T_SEMI || sep->type == T_AMP) {
            it->background = sep->type == T_AMP;
            next(&p);
        } else if (sep->type != T_END) {
            syntax_error(sep);
            goto fail;
        }
    }
    return 0;

fail:
    script_free(out);
    return -1;
}
