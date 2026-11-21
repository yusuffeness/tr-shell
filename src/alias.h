/*
 * alias.h - user-defined commands (takma adlar).
 *
 *   takmaad ll=listele -d          define (saved in ~/.trsh/config)
 *   takmaad guncelle='git pull && make'
 *   takmaad                        list
 *   takmaadsil ll                  remove
 *
 * An alias is replaced by its text when it is the first word of a command
 * and is written without quotes. The text may contain pipes and && too.
 */
#ifndef TRSH_ALIAS_H
#define TRSH_ALIAS_H

#include "lexer.h"

/* Replaces aliases in command position. Returns 0, or -1 (error printed). */
int alias_expand(struct tokens *t);

int bi_takmaad(int argc, char **argv);
int bi_takmaadsil(int argc, char **argv);

#endif
