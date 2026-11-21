/*
 * expand.h - turning a raw word into the arguments a program receives.
 *
 *   ~ at the start          -> $HOME
 *   $NAME ${NAME}           -> environment variable (empty if unset)
 *   $?  $$                  -> last exit status, shell's process id
 *   '...'                   -> literal, nothing expanded
 *   "..."                   -> variables expanded; \" \\ \$ escaped
 *   \c                      -> the character c, literally
 *   $(komut)                -> what the command printed (unquoted: split
 *                              into words at white space)
 *   $1 .. $9  $#  $@  $0    -> script arguments
 *   * ? [...] (unquoted)    -> matching file names, sorted; the pattern
 *                              itself if nothing matches
 */
#ifndef TRSH_EXPAND_H
#define TRSH_EXPAND_H

#include "strvec.h"

/*
 * Expands raw into zero or more words appended to out (zero when it was
 * only an empty variable). Returns 0 or -1.
 */
int expand_word(const char *raw, struct strvec *out);

/* Expands raw into exactly one string (no file-name matching). Caller frees. */
char *expand_single(const char *raw);

/* 1 if raw has the form NAME=... with an unquoted, valid NAME. */
int is_assignment(const char *raw);

#endif
