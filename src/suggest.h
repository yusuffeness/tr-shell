/*
 * suggest.h - "did you mean ...?" for mistyped commands.
 */
#ifndef TRSH_SUGGEST_H
#define TRSH_SUGGEST_H

/*
 * Returns the tr-shell command closest to name (by edit distance), or NULL
 * if none is close enough to be a likely typo.
 */
const char *suggest_command(const char *name);

#endif
