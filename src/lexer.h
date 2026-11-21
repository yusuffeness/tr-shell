/*
 * lexer.h - splitting a command line into tokens.
 *
 * Words are kept exactly as typed, quotes included; quote removal and
 * variable expansion happen later (expand.c), right before a command runs,
 * so that `X=1; yaz $X` sees the new value.
 */
#ifndef TRSH_LEXER_H
#define TRSH_LEXER_H

#include <stddef.h>

enum tok_type {
    T_WORD,
    T_PIPE,         /* |    */
    T_OR,           /* ||   */
    T_AMP,          /* &    */
    T_AND,          /* &&   */
    T_SEMI,         /* ;    */
    T_LESS,         /* <    */
    T_GREAT,        /* >    */
    T_DGREAT,       /* >>   */
    T_ERR_GREAT,    /* 2>   */
    T_ERR_DGREAT,   /* 2>>  */
    T_ERR_TO_OUT,   /* 2>&1 */
    T_END,
};

struct token {
    enum tok_type type;
    char *text;     /* raw word, or the operator as written */
};

struct tokens {
    struct token *v;
    size_t n, cap;
};

/*
 * Tokenizes line. Text after an unquoted # at the start of a word is a
 * comment. Returns 0, or -1 after printing an error (unclosed quote).
 * The list always ends with a T_END token.
 */
int lex(const char *line, struct tokens *out);

void tokens_free(struct tokens *t);

#endif
