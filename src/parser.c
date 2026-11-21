/*
 * parser.c - building a syntax tree from tokens (recursive descent).
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "messages.h"
#include "shell.h"
#include "ui.h"

/* Keywords that end a list; a bit set says which ones the caller expects. */
enum {
    K_ISE     = 1 << 0,
    K_DEGILSE = 1 << 1,
    K_SON     = 1 << 2,
    K_YAP     = 1 << 3,
};

struct parser {
    const struct tokens *t;
    size_t pos;
    unsigned stop;      /* keywords that end a word list here (ise / yap) */
};

static int parse_list(struct parser *p, struct script *out, unsigned ends);

/* ------------------------------------------------------------- tokens */

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
    if (tok->type == T_END || tok->type == T_NEWLINE)
        ui_error(TRSH_NAME, "%s", MSG_SYNTAX_END);
    else
        ui_error(TRSH_NAME, MSG_SYNTAX, tok->text);
    return -1;
}

/* Running out of input inside a compound command means "read more". */
static int unexpected(const struct token *tok, int inside_compound)
{
    if (tok->type == T_END && inside_compound)
        return PARSE_INCOMPLETE;
    return syntax_error(tok);
}

/* Is tok the plain (unquoted) word w? */
static int is_word(const struct token *tok, const char *w)
{
    return tok->type == T_WORD && strcmp(tok->text, w) == 0;
}

static unsigned keyword_bit(const struct token *tok)
{
    if (tok->type != T_WORD)
        return 0;
    if (strcmp(tok->text, "ise") == 0)
        return K_ISE;
    if (strcmp(tok->text, "degilse") == 0)
        return K_DEGILSE;
    if (strcmp(tok->text, "son") == 0)
        return K_SON;
    if (strcmp(tok->text, "yap") == 0)
        return K_YAP;
    return 0;
}

static int is_redirection(enum tok_type t)
{
    return t == T_LESS || t == T_GREAT || t == T_DGREAT || t == T_ERR_GREAT ||
           t == T_ERR_DGREAT || t == T_ERR_TO_OUT;
}

static int is_separator(enum tok_type t)
{
    return t == T_SEMI || t == T_NEWLINE || t == T_AMP;
}

static void skip_newlines(struct parser *p)
{
    while (peek(p)->type == T_NEWLINE)
        next(p);
}

/* Grows an array of elements of size elem to hold n + 1. */
static void *grow(void *array, size_t n, size_t elem)
{
    return realloc(array, (n + 1) * elem);
}

/* ------------------------------------------------------------- freeing */

static void command_free(struct command *c);

void script_free(struct script *s);

static void script_destroy(struct script *s)
{
    if (s != NULL) {
        script_free(s);
        free(s);
    }
}

static void command_free(struct command *c)
{
    strvec_free(&c->words);
    for (size_t i = 0; i < c->nredirs; i++)
        free(c->redirs[i].target);
    free(c->redirs);
    script_destroy(c->cond);
    script_destroy(c->body);
    script_destroy(c->else_body);
    free(c->var);
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

/* -------------------------------------------------------- simple commands */

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

static int parse_simple(struct parser *p, struct command *c)
{
    for (;;) {
        const struct token *tok = peek(p);

        if (tok->type == T_WORD) {
            /* "eger test -f a ise": ise ends the condition without a ';'. */
            if (c->words.n > 0 && (keyword_bit(tok) & p->stop))
                break;
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

/* ------------------------------------------------------ compound commands */

/*
 * Parses a nested list into a newly allocated script. stop is the set of
 * keywords that also end a command's word list (only for conditions).
 */
static int sub_list(struct parser *p, struct script **out, unsigned ends,
                    unsigned stop)
{
    unsigned saved = p->stop;

    *out = calloc(1, sizeof **out);
    if (*out == NULL)
        return -1;
    p->stop = stop;
    int r = parse_list(p, *out, ends);
    p->stop = saved;
    return r;
}

/* Expects the keyword w; a missing one at the end of input means "read more". */
static int expect(struct parser *p, const char *w)
{
    if (is_word(peek(p), w)) {
        next(p);
        return 0;
    }
    return unexpected(peek(p), 1);
}

/* eger COND ise BODY [degilse ELSE] son */
static int parse_if(struct parser *p, struct command *c)
{
    int r;

    c->type = CT_IF;
    next(p);
    if ((r = sub_list(p, &c->cond, K_ISE, K_ISE)) != 0 || (r = expect(p, "ise")) != 0)
        return r;
    if (c->cond->nitems == 0)
        return syntax_error(&p->t->v[p->pos - 1]);
    if ((r = sub_list(p, &c->body, K_DEGILSE | K_SON, 0)) != 0)
        return r;
    if (is_word(peek(p), "degilse")) {
        next(p);
        if ((r = sub_list(p, &c->else_body, K_SON, 0)) != 0)
            return r;
    }
    return expect(p, "son");
}

/* dongu COND yap BODY son */
static int parse_while(struct parser *p, struct command *c)
{
    int r;

    c->type = CT_WHILE;
    next(p);
    if ((r = sub_list(p, &c->cond, K_YAP, K_YAP)) != 0 || (r = expect(p, "yap")) != 0)
        return r;
    if (c->cond->nitems == 0)
        return syntax_error(&p->t->v[p->pos - 1]);
    if ((r = sub_list(p, &c->body, K_SON, 0)) != 0)
        return r;
    return expect(p, "son");
}

static int valid_name(const char *s)
{
    if (!isalpha((unsigned char)*s) && *s != '_')
        return 0;
    for (; *s; s++)
        if (!isalnum((unsigned char)*s) && *s != '_')
            return 0;
    return 1;
}

/* her VAR icin WORD... yap BODY son */
static int parse_for(struct parser *p, struct command *c)
{
    int r;

    c->type = CT_FOR;
    next(p);

    const struct token *name = peek(p);
    if (name->type != T_WORD || !valid_name(name->text))
        return unexpected(name, 1);
    c->var = strdup(next(p)->text);
    if (c->var == NULL)
        return -1;
    if ((r = expect(p, "icin")) != 0)
        return r;

    /* The list runs up to "yap", which may follow a ; or a new line. */
    while (peek(p)->type == T_WORD && !is_word(peek(p), "yap"))
        if (strvec_push(&c->words, next(p)->text) != 0)
            return -1;
    if (peek(p)->type == T_SEMI || peek(p)->type == T_NEWLINE)
        next(p);
    skip_newlines(p);
    if ((r = expect(p, "yap")) != 0)
        return r;
    if ((r = sub_list(p, &c->body, K_SON, 0)) != 0)
        return r;
    return expect(p, "son");
}

static int parse_command(struct parser *p, struct command *c)
{
    const struct token *tok = peek(p);
    int r;

    memset(c, 0, sizeof *c);
    c->type = CT_SIMPLE;

    if (is_word(tok, "eger"))
        r = parse_if(p, c);
    else if (is_word(tok, "dongu"))
        r = parse_while(p, c);
    else if (is_word(tok, "her"))
        r = parse_for(p, c);
    else if (keyword_bit(tok) != 0)
        return syntax_error(tok);       /* "son" with no "eger" before it */
    else
        return parse_simple(p, c);

    if (r != 0)
        return r;
    /* A compound command may be followed by redirections only. */
    while (is_redirection(peek(p)->type))
        if (parse_redirection(p, c) != 0)
            return -1;
    /* "... son son": a closing keyword may follow without a ';'. */
    if (peek(p)->type == T_WORD && keyword_bit(peek(p)) == 0)
        return syntax_error(peek(p));
    return 0;
}

/* --------------------------------------------------------------- lists */

static int parse_pipeline(struct parser *p, struct pipeline *pl)
{
    memset(pl, 0, sizeof *pl);

    for (;;) {
        struct command *grown = grow(pl->cmds, pl->ncmds, sizeof *grown);
        if (grown == NULL)
            return -1;
        pl->cmds = grown;
        int r = parse_command(p, &pl->cmds[pl->ncmds]);
        if (r != 0) {
            command_free(&pl->cmds[pl->ncmds]);
            return r;
        }
        pl->ncmds++;
        if (peek(p)->type != T_PIPE)
            return 0;
        next(p);
        skip_newlines(p);       /* a | may end a line */
        if (peek(p)->type == T_END)
            return unexpected(peek(p), 1);  /* continued on the next line */
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
        int r = parse_pipeline(p, &ao->pipes[ao->npipes]);
        if (r != 0) {
            pipeline_free(&ao->pipes[ao->npipes]);
            return r;
        }
        ao->npipes++;

        enum tok_type t = peek(p)->type;
        if (t != T_AND && t != T_OR)
            return 0;
        next(p);
        skip_newlines(p);       /* && or || may end a line */
        if (peek(p)->type == T_END)
            return unexpected(peek(p), 1);  /* continued on the next line */

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
        if (t->v[i].type == T_NEWLINE && (i + 1 == to || i == from))
            continue;
        if (s[0] != '\0')
            strcat(s, " ");
        strcat(s, t->v[i].text);
    }
    return s;
}

/*
 * Parses items until the end of input or, inside a compound command, until
 * one of the keywords in ends stands where a command would start.
 */
static int parse_list(struct parser *p, struct script *out, unsigned ends)
{
    int nested = ends != 0;

    out->items = NULL;
    out->nitems = 0;
    skip_newlines(p);

    for (;;) {
        const struct token *tok = peek(p);

        if (tok->type == T_END)
            return nested ? PARSE_INCOMPLETE : 0;
        if (nested && (keyword_bit(tok) & ends))
            return 0;

        size_t start = p->pos;
        struct item *grown = grow(out->items, out->nitems, sizeof *grown);
        if (grown == NULL)
            return -1;
        out->items = grown;

        struct item *it = &out->items[out->nitems];
        memset(it, 0, sizeof *it);
        int r = parse_and_or(p, &it->ao);
        if (r != 0) {
            and_or_free(&it->ao);
            return r;
        }
        it->text = join_tokens(p->t, start, p->pos);
        out->nitems++;

        const struct token *sep = peek(p);
        if (is_separator(sep->type)) {
            it->background = sep->type == T_AMP;
            next(p);
            skip_newlines(p);
        } else if (sep->type != T_END && !(nested && (keyword_bit(sep) & ends))) {
            return syntax_error(sep);
        }
    }
}

int parse(const struct tokens *t, struct script *out)
{
    struct parser p = { t, 0, 0 };
    int r = parse_list(&p, out, 0);

    if (r != 0)
        script_free(out);
    return r;
}
