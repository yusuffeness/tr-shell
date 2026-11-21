/*
 * strvec.h - a growable, NULL-terminated array of owned strings.
 */
#ifndef TRSH_STRVEC_H
#define TRSH_STRVEC_H

#include <stddef.h>

struct strvec {
    char **v;       /* always NULL-terminated once anything was pushed */
    size_t n, cap;
};

/* Appends a copy of s. Returns 0, or -1 if out of memory. */
int strvec_push(struct strvec *sv, const char *s);

/* Appends s itself (takes ownership). */
int strvec_push_owned(struct strvec *sv, char *s);

void strvec_free(struct strvec *sv);

#endif
