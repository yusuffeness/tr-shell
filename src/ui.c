/*
 * ui.c - renkler, açılış/kapanış mesajları ve istem.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "ui.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void ui_init(void)
{
    g_shell.use_color = isatty(STDOUT_FILENO);
}

void ui_banner(void)
{
    printf("%s%s%s %s - %s\n", BOLD, TRSH_NAME, RESET, TRSH_VERSION, TRSH_USER);
    printf("%sCikmak icin 'exit' yazin ya da Ctrl+D'ye basin.%s\n\n", GRAY, RESET);
}

void ui_goodbye(void)
{
    printf("%sGorusmek uzere!%s\n", GRAY, RESET);
}

/*
 * Bulunulan dizini döndürür; ev dizini ~ ile kısaltılır.
 * Dönen işaretçi statik bir tampona işaret eder.
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
