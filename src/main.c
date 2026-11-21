/*
 * main.c - tr-shell main loop.
 *
 * Each iteration: show the prompt, read a line, expand history references,
 * split it into arguments and run it.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "shell.h"
#include "ui.h"
#include "input.h"
#include "parser.h"
#include "commands.h"
#include "history.h"
#include "signals.h"

struct shell g_shell = {
    .last_status      = 0,
    .last_duration_ms = 0,
    .has_run          = 0,
    .interactive      = 0,
    .use_color        = 0,
    .running          = 1,
};

static long elapsed_ms(const struct timespec *a, const struct timespec *b)
{
    return (b->tv_sec - a->tv_sec) * 1000L + (b->tv_nsec - a->tv_nsec) / 1000000L;
}

static void finish(int status, long ms)
{
    g_shell.last_status = status;
    g_shell.last_duration_ms = ms;
    g_shell.has_run = 1;
}

static void run(int argc, char **argv)
{
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    int status = commands_exec(argc, argv);
    clock_gettime(CLOCK_MONOTONIC, &end);
    finish(status, elapsed_ms(&start, &end));
}

/*
 * Replaces a leading !!, !N or !PREFIX with the line from history and shows
 * the result, as bash does. Returns 0 if the line should not be run.
 */
static int expand_history(char *line, size_t size)
{
    char expanded[MAX_LINE];
    int r = hist_expand(line, expanded, sizeof expanded);

    if (r < 0)
        return 0;
    if (r > 0) {
        snprintf(line, size, "%s", expanded);
        puts(line);
    }
    return 1;
}

int main(void)
{
    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    ui_init();
    signals_init();
    input_init();
    hist_init();
    if (g_shell.interactive)
        ui_banner();

    while (g_shell.running) {
        const char *prompt = NULL;

        if (g_shell.interactive) {
            ui_prompt();
            prompt = ui_prompt_tail(input_uses_readline());
        }

        enum input_result r = input_read_line(line, sizeof line, prompt);
        if (r == INPUT_EOF) {           /* Ctrl+D or end of file */
            if (g_shell.interactive)
                putchar('\n');
            break;
        }
        if (r == INPUT_INTERRUPTED) {   /* Ctrl+C at the prompt */
            finish(130, 0);
            continue;
        }
        if (r == INPUT_TOO_LONG) {
            finish(1, 0);
            continue;
        }

        if (!expand_history(line, sizeof line)) {
            finish(1, 0);
            continue;
        }
        hist_add(line);

        int argc = parse_line(line, argv, MAX_ARGS);
        if (argc < 0) {
            finish(1, 0);
            continue;
        }
        if (argc == 0)                  /* empty line */
            continue;

        run(argc, argv);

        /* Builtin output must not lag behind the next command's output. */
        fflush(stdout);
    }

    /* Back to the normal screen first, so the goodbye stays visible. */
    ui_shutdown();
    if (g_shell.interactive)
        ui_goodbye();
    return g_shell.last_status;
}
