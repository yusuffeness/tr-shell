/*
 * main.c - tr-shell main loop.
 *
 * Each iteration: report finished background jobs, show the prompt, read
 * a line, expand history references, then tokenize, parse and run it.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "shell.h"
#include "ui.h"
#include "input.h"
#include "lexer.h"
#include "parser.h"
#include "executor.h"
#include "history.h"
#include "jobs.h"
#include "signals.h"

#define EXIT_SYNTAX 2       /* same as bash for syntax errors */

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

/* Tokenizes, parses and runs one line. Blank lines and comments do nothing. */
static void run_line(const char *line)
{
    struct tokens tokens;
    struct script script;
    struct timespec start, end;

    if (lex(line, &tokens) != 0) {
        finish(EXIT_SYNTAX, 0);
        return;
    }
    if (tokens.n == 1) {            /* only T_END */
        tokens_free(&tokens);
        return;
    }
    int parsed = parse(&tokens, &script);
    tokens_free(&tokens);
    if (parsed != 0) {
        finish(EXIT_SYNTAX, 0);
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    int status = exec_script(&script);
    clock_gettime(CLOCK_MONOTONIC, &end);
    script_free(&script);
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
    static char line[MAX_LINE];

    ui_init();
    signals_init();
    jobs_init();
    input_init();
    hist_init();
    if (g_shell.interactive)
        ui_banner();

    while (g_shell.running) {
        const char *prompt = NULL;

        /* "[1]+ Bitti  sleep 10" for background jobs that have finished. */
        jobs_poll(1);

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
        run_line(line);

        /* Builtin output must not lag behind the next command's output. */
        fflush(stdout);
    }

    jobs_cleanup();
    /* Back to the normal screen first, so the goodbye stays visible. */
    ui_shutdown();
    if (g_shell.interactive)
        ui_goodbye();
    return g_shell.last_status;
}
