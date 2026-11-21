/*
 * suggest.c - "did you mean ...?" for mistyped commands.
 */
#include <string.h>

#include "suggest.h"
#include "commands.h"

#define MAX_NAME 32

/*
 * Levenshtein distance: the fewest single-character insertions, deletions
 * and substitutions that turn a into b. Two rows of the usual table suffice.
 */
static size_t edit_distance(const char *a, const char *b)
{
    size_t la = strlen(a), lb = strlen(b);
    size_t prev[MAX_NAME + 1], cur[MAX_NAME + 1];

    if (la > MAX_NAME || lb > MAX_NAME)
        return (size_t)-1;

    for (size_t j = 0; j <= lb; j++)
        prev[j] = j;
    for (size_t i = 1; i <= la; i++) {
        cur[0] = i;
        for (size_t j = 1; j <= lb; j++) {
            size_t del = prev[j] + 1;
            size_t ins = cur[j - 1] + 1;
            size_t sub = prev[j - 1] + (a[i - 1] != b[j - 1]);
            size_t best = del < ins ? del : ins;
            cur[j] = best < sub ? best : sub;
        }
        memcpy(prev, cur, (lb + 1) * sizeof prev[0]);
    }
    return prev[lb];
}

const char *suggest_command(const char *name)
{
    size_t len = strlen(name);
    /* Short words tolerate one mistake, longer ones two. */
    size_t limit = len <= 4 ? 1 : 2;
    const char *best = NULL;
    size_t best_dist = limit + 1;

    if (len < 2 || strchr(name, '/') != NULL)
        return NULL;

    for (size_t i = 0; i < commands_count(); i++) {
        const char *candidate = commands_at(i)->name;
        size_t d = edit_distance(name, candidate);

        if (d > 0 && d < best_dist) {
            best = candidate;
            best_dist = d;
        }
    }
    return best;
}
