/*
 * util.c - small helpers shared by the custom commands.
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "util.h"
#include "shell.h"

int text_width(const char *s)
{
    int n = 0;

    for (; *s; s++)
        if (((unsigned char)*s & 0xC0) != 0x80)
            n++;
    return n;
}

void fmt_size(char *buf, size_t size, long long bytes)
{
    static const char *const units[] = { "B", "KB", "MB", "GB", "TB" };
    double v = (double)bytes;
    size_t u = 0;

    while (v >= 1024 && u + 1 < sizeof units / sizeof units[0]) {
        v /= 1024;
        u++;
    }
    if (u == 0) {
        snprintf(buf, size, "%lld B", bytes);
    } else if (v < 10) {
        /* One decimal for small values, with a Turkish decimal comma. */
        long tenths = (long)(v * 10 + 0.5);
        snprintf(buf, size, "%ld,%ld %s", tenths / 10, tenths % 10, units[u]);
    } else {
        snprintf(buf, size, "%.0f %s", v, units[u]);
    }
}

void fmt_short_date(char *buf, size_t size, time_t t)
{
    static const char *const months[] = {
        "Oca", "Şub", "Mar", "Nis", "May", "Haz",
        "Tem", "Ağu", "Eyl", "Eki", "Kas", "Ara",
    };
    struct tm tm, now_tm;
    time_t now = time(NULL);

    if (localtime_r(&t, &tm) == NULL || localtime_r(&now, &now_tm) == NULL) {
        snprintf(buf, size, "?");
        return;
    }
    if (tm.tm_year == now_tm.tm_year)
        snprintf(buf, size, "%2d %s %02d:%02d", tm.tm_mday, months[tm.tm_mon],
                 tm.tm_hour, tm.tm_min);
    else
        snprintf(buf, size, "%2d %s  %d", tm.tm_mday, months[tm.tm_mon],
                 tm.tm_year + 1900);
}

int ask_yes_no(const char *question)
{
    char answer[32];

    fputs(question, stdout);
    fflush(stdout);
    if (fgets(answer, sizeof answer, stdin) == NULL) {
        clearerr(stdin);
        putchar('\n');
        return 0;
    }
    /* A terminal echoes the Enter key; a script does not. */
    if (!g_shell.interactive)
        putchar('\n');

    answer[strcspn(answer, "\r\n")] = '\0';
    char *a = answer;
    while (isspace((unsigned char)*a))
        a++;
    return strcasecmp(a, "e") == 0 || strcasecmp(a, "evet") == 0 ||
           strcasecmp(a, "y") == 0 || strcasecmp(a, "yes") == 0;
}

int join_args(char *buf, size_t size, int argc, char **argv)
{
    size_t used = 0;

    buf[0] = '\0';
    for (int i = 0; i < argc; i++) {
        int n = snprintf(buf + used, size - used, "%s%s", i ? " " : "", argv[i]);
        if (n < 0 || (size_t)n >= size - used)
            return 0;
        used += (size_t)n;
    }
    return 1;
}
