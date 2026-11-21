/*
 * parser.h - building a syntax tree from tokens.
 *
 *   script   := sep* (item (sep item)*)? sep*        sep: ';' '&' or a new line
 *   item     := and_or ['&']                          '&' runs it in the background
 *   and_or   := pipeline (('&&' | '||') pipeline)*
 *   pipeline := command ('|' command)*
 *   command  := simple | compound redirection*
 *   simple   := (WORD | redirection)+
 *   compound := 'eger' script 'ise' script ['degilse' script] 'son'
 *             | 'dongu' script 'yap' script 'son'
 *             | 'her' NAME 'icin' WORD* sep? 'yap' script 'son'
 *   redirection := ('<' | '>' | '>>' | '2>' | '2>>') WORD  |  '2>&1'
 *
 * Keywords count only where a command starts and only without quotes, so
 * `echo son` prints "son". Words stay raw; they are expanded at run time.
 */
#ifndef TRSH_PARSER_H
#define TRSH_PARSER_H

#include "lexer.h"
#include "strvec.h"

#define PARSE_INCOMPLETE (-2)   /* input ended inside eger/dongu/her */

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

enum cmd_type {
    CT_SIMPLE,
    CT_IF,          /* eger COND ise BODY [degilse ELSE] son */
    CT_WHILE,       /* dongu COND yap BODY son                */
    CT_FOR,         /* her VAR icin WORDS yap BODY son        */
};

struct script;

struct command {
    enum cmd_type type;
    struct strvec words;    /* simple: raw words; for: the list to iterate */
    struct redir *redirs;
    size_t nredirs;
    struct script *cond;    /* if, while */
    struct script *body;    /* if (then), while, for */
    struct script *else_body;
    char *var;              /* for */
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

/*
 * Returns 0; -1 after printing a syntax error; or PARSE_INCOMPLETE (no
 * message) if the text ends inside eger / dongu / her, so that more lines
 * can be read.
 */
int parse(const struct tokens *t, struct script *out);

void script_free(struct script *s);

#endif
