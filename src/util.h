/*
 * util.h - small helpers shared by the custom commands.
 */
#ifndef TRSH_UTIL_H
#define TRSH_UTIL_H

#include <stddef.h>
#include <time.h>

/*
 * Display width of a UTF-8 string, one column per code point. Good enough
 * for Turkish text and box drawing; double-width characters are not handled.
 */
int text_width(const char *s);

/* Human-readable size, Turkish style: "512 B", "1,2 KB", "34 MB". */
void fmt_size(char *buf, size_t size, long long bytes);

/* Short date: "21 Kas 07:54" this year, "21 Kas 2025" otherwise. */
void fmt_short_date(char *buf, size_t size, time_t t);

/*
 * Prints question, reads one line from stdin and returns 1 for a yes
 * ("e", "evet", "y", "yes"), 0 for anything else, end of input or Ctrl+C.
 */
int ask_yes_no(const char *question);

/* Joins argv[0..argc) with single spaces. Returns 0 if it does not fit. */
int join_args(char *buf, size_t size, int argc, char **argv);

#endif
