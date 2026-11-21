/*
 * input.c - reading command lines.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#endif

#include "input.h"
#include "completion.h"
#include "messages.h"
#include "signals.h"
#include "ui.h"

int input_uses_readline(void)
{
#ifdef HAVE_READLINE
    return g_shell.interactive;
#else
    return 0;
#endif
}

void input_init(void)
{
#ifdef HAVE_READLINE
    if (input_uses_readline()) {
        rl_readline_name = "trsh";
        completion_init();
    }
#endif
}

static enum input_result too_long(size_t size)
{
    ui_error(TRSH_NAME, MSG_LINE_TOO_LONG, (int)size - 2);
    return INPUT_TOO_LONG;
}

static enum input_result read_fgets(char *buf, size_t size, const char *prompt)
{
    if (prompt != NULL && g_shell.interactive) {
        fputs(prompt, stdout);
        fflush(stdout);
    }

    errno = 0;
    if (fgets(buf, (int)size, stdin) == NULL) {
        if (ferror(stdin) && errno == EINTR && g_sigint) {
            clearerr(stdin);
            g_sigint = 0;
            putchar('\n');
            return INPUT_INTERRUPTED;
        }
        if (ferror(stdin))
            perror(TRSH_NAME);
        return INPUT_EOF;
    }

    /* The line did not fit: drop the remainder so it is not run next. */
    if (strchr(buf, '\n') == NULL && !feof(stdin)) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        return too_long(size);
    }
    buf[strcspn(buf, "\n")] = '\0';
    return INPUT_LINE;
}

#ifdef HAVE_READLINE
/*
 * readline's callback interface: we wait for input with select() ourselves,
 * so Ctrl+C simply interrupts select() and the line can be dropped cleanly.
 */
static char *rl_line;
static int rl_have_line;

static void on_line(char *line)
{
    rl_line = line;
    rl_have_line = 1;
    rl_callback_handler_remove();
}

static enum input_result read_readline(char *buf, size_t size, const char *prompt)
{
    int fd = fileno(rl_instream ? rl_instream : stdin);

    g_sigint = 0;
    rl_line = NULL;
    rl_have_line = 0;
    rl_callback_handler_install(prompt, on_line);

    while (!rl_have_line) {
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        if (select(fd + 1, &fds, NULL, NULL, NULL) < 0) {
            if (errno != EINTR) {
                rl_callback_handler_remove();
                return INPUT_EOF;
            }
            if (g_sigint) {
                g_sigint = 0;
                /* Show ^C after what was typed, then start a fresh line. */
                rl_point = rl_end;
                rl_redisplay();
                fputs("^C\n", rl_outstream ? rl_outstream : stdout);
                rl_callback_sigcleanup();
                rl_free_line_state();
                rl_callback_handler_remove();
                return INPUT_INTERRUPTED;
            }
            continue;   /* another signal, e.g. a window resize */
        }
        rl_callback_read_char();
    }

    if (rl_line == NULL)        /* Ctrl+D on an empty line */
        return INPUT_EOF;

    size_t len = strlen(rl_line);
    if (len >= size - 1) {
        free(rl_line);
        return too_long(size);
    }
    memcpy(buf, rl_line, len + 1);
    free(rl_line);
    return INPUT_LINE;
}
#endif

enum input_result input_read_line(char *buf, size_t size, const char *prompt)
{
#ifdef HAVE_READLINE
    if (input_uses_readline())
        return read_readline(buf, size, prompt);
#endif
    return read_fgets(buf, size, prompt);
}
