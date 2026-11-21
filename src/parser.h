/*
 * parser.h - building a syntax tree from tokens.
 *
 *   script   := item*
 *   item     := and_or (';' | '&' | end)        '&' runs it in the background
 *   and_or   := pipeline (('&&' | '||') pipeline)*
 *   pipeline := command ('|' command)*
 *   command  := (WORD | redirection)+
 *   redirection := ('<' | '>' | '>>' | '2>' | '2>>') WORD  |  '2>&1'
 *
 * Words stay raw (quotes included); they are expanded when the command runs.
 */
#ifndef TRSH_PARSER_H
#define TRSH_PARSER_H

#include "lexer.h"
#include "strvec.h"

enum redir_kind {
    R_IN,           /* < file   */
    R_OUT,          /* > file   (fd 1 or 2) */
    R_APPEND,       /* >> file  (fd 1 or 2) */
    R_ERR_TO_OUT,   /* 2>&1     */
};

struct redir {
    enum redir_kind kind;
    int fd;             /* the descriptor being redirected */
    char *target;       /* raw word; NULL for 2>&1 */
};

struct command {
    struct strvec words;    /* raw words, assignments (NAME=x) first */
    struct redir *redirs;
    size_t nredirs;
};

struct pipeline {
    struct command *cmds;
    size_t ncmds;
};

enum andor_op { OP_AND, OP_OR };

struct and_or {
    struct pipeline *pipes;
    enum andor_op *ops;     /* ops[i] joins pipes[i] and pipes[i + 1] */
    size_t npipes;
};

struct item {
    struct and_or ao;
    int background;
    char *text;             /* the item as typed, for job listings */
};

struct script {
    struct item *items;
    size_t nitems;
};

/* Returns 0, or -1 after printing a syntax error. */
int parse(const struct tokens *t, struct script *out);

void script_free(struct script *s);

#endif
