/*
 * strvec.c - a growable, NULL-terminated array of owned strings.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>

#include "strvec.h"

int strvec_push_owned(struct strvec *sv, char *s)
{
    if (sv->n + 2 > sv->cap) {
        size_t cap = sv->cap ? sv->cap * 2 : 8;
        char **grown = realloc(sv->v, cap * sizeof *grown);
        if (grown == NULL)
            return -1;
        sv->v = grown;
        sv->cap = cap;
    }
    sv->v[sv->n++] = s;
    sv->v[sv->n] = NULL;
    return 0;
}

int strvec_push(struct strvec *sv, const char *s)
{
    char *copy = strdup(s);

    if (copy == NULL)
        return -1;
    if (strvec_push_owned(sv, copy) != 0) {
        free(copy);
        return -1;
    }
    return 0;
}

void strvec_free(struct strvec *sv)
{
    for (size_t i = 0; i < sv->n; i++)
        free(sv->v[i]);
    free(sv->v);
    sv->v = NULL;
    sv->n = sv->cap = 0;
}
