/*
 * ui.c - colors, banner, prompt and error reporting.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "ui.h"
#include "messages.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void ui_init(void)
{
    /*
     * When input comes from a file or a pipe (e.g. the test suite) the
     * shell runs non-interactively: no banner, prompt or goodbye.
     */
    g_shell.interactive = isatty(STDIN_FILENO);
    g_shell.use_color = isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;
}

void ui_banner(void)
{
    printf("%s%s%s %s - %s\n", BOLD, TRSH_NAME, RESET, TRSH_VERSION, TRSH_USER);
    printf("%s%s%s\n\n", GRAY, MSG_BANNER_HINT, RESET);
}

void ui_goodbye(void)
{
    printf("%s%s%s\n", GRAY, MSG_GOODBYE, RESET);
}

void ui_error(const char *who, const char *fmt, ...)
{
    va_list ap;

    /* Keep stdout and stderr in order when both go to the same pipe. */
    fflush(stdout);
    fprintf(stderr, "%s%s:%s ", RED, who, RESET);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

/*
 * Returns the current directory with the home directory shortened to ~.
 * The result points to a static buffer.
 */
static const char *shown_cwd(void)
{
    static char cwd[PATH_MAX];
    static char buf[PATH_MAX + 1];
    const char *home = getenv("HOME");

    if (getcwd(cwd, sizeof cwd) == NULL)
        return "?";

    size_t hl = home ? strlen(home) : 0;
    if (hl > 1 && strncmp(cwd, home, hl) == 0 &&
        (cwd[hl] == '/' || cwd[hl] == '\0')) {
        snprintf(buf, sizeof buf, "~%s", cwd + hl);
        return buf;
    }
    return cwd;
}

void ui_prompt(void)
{
    printf("%s%s%s %s%s%s", GREEN, TRSH_USER, RESET, BLUE, shown_cwd(), RESET);
    if (g_shell.last_status != 0)
        printf(" %s[%d]%s", RED, g_shell.last_status, RESET);
    printf("%s>%s ", BOLD, RESET);
    fflush(stdout);
}
