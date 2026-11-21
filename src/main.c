/*
 * main.c - tr-shell entry point and main loop.
 *
 *   trsh                          interactive shell (or commands from a pipe)
 *   trsh DOSYA [ARG...]           run a script; $0 is DOSYA, $1... the arguments
 *   trsh -c 'KOMUT' [AD ARG...]   run one command string ($0 = AD)
 *   trsh --surum | --yardim
 *
 * Each loop iteration: report finished background jobs, show the prompt,
 * read a line (and more lines while an eger / dongu / her block is open),
 * then parse and run it.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "shell.h"
#include "ui.h"
#include "input.h"
#include "config.h"
#include "history.h"
#include "jobs.h"
#include "messages.h"
#include "run.h"
#include "signals.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define STARTUP_FILE "baslangic"    /* ~/.trsh/baslangic, like ~/.bashrc */

struct shell g_shell = {
    .running = 1,
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

/* ------------------------------------------------------ the main loop */

struct block {
    char text[MAX_LINE * 4];    /* all lines of an open eger / dongu / her */
    size_t len;
};

static int block_add(struct block *b, const char *line)
{
    size_t n = strlen(line);

    if (b->len + n + 2 > sizeof b->text) {
        ui_error(TRSH_NAME, MSG_LINE_TOO_LONG, (int)sizeof b->text - 2);
        return -1;
    }
    if (b->len > 0)
        b->text[b->len++] = '\n';
    memcpy(b->text + b->len, line, n + 1);
    b->len += n;
    return 0;
}

static int is_opening_word(const char *one, size_t len)
{
    static const char *const words[] = { "ise", "yap", "degilse" };

    for (size_t i = 0; i < sizeof words / sizeof words[0]; i++) {
        size_t w = strlen(words[i]);
        if (len >= w && memcmp(one + len - w, words[i], w) == 0
            && (len == w || one[len - w - 1] == ' ' || one[len - w - 1] == ';'))
            return 1;
    }
    return 0;
}

/* The block as one history line: new lines become "; " where needed. */
static void block_to_history(const struct block *b)
{
    char one[MAX_LINE * 4];
    size_t len = 0;
    const char *s = b->text;

    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
    while (*s && len + 3 < sizeof one) {
        if (*s != '\n') {
            one[len++] = *s++;
            continue;
        }
        while (len > 0 && (one[len - 1] == ' ' || one[len - 1] == '\t'))
            len--;
        /* After | && || ; or ise / yap / degilse a space is enough. */
        int joins = len > 0 && (strchr("|&;", one[len - 1]) != NULL
                                || is_opening_word(one, len));
        if (len > 0 && !joins)
            one[len++] = ';';
        if (len > 0)
            one[len++] = ' ';
        while (*s == '\n' || *s == ' ' || *s == '\t')
            s++;
    }
    while (len > 0 && (one[len - 1] == ' ' || one[len - 1] == ';'))
        len--;
    one[len] = '\0';
    if (len > 0)
        hist_add(one);
}

static void interactive_loop(void)
{
    static char line[MAX_LINE];
    static struct block block;

    while (g_shell.running) {
        const char *prompt = NULL;

        /* "[1]+ Bitti  sleep 10" for background jobs that have finished. */
        if (block.len == 0)
            jobs_poll(1);

        if (g_shell.interactive) {
            if (block.len == 0) {
                ui_prompt();
                prompt = ui_prompt_tail(input_uses_readline());
            } else {
                prompt = ui_prompt_more(input_uses_readline());
            }
        }

        enum input_result r = input_read_line(line, sizeof line, prompt);
        if (r == INPUT_EOF) {               /* Ctrl+D or end of file */
            if (block.len > 0) {
                if (g_shell.interactive)
                    putchar('\n');
                run_text(block.text, 0);    /* reports the missing "son" */
                block.len = 0;
                finish(EXIT_SYNTAX, 0);
                if (g_shell.interactive) {
                    clearerr(stdin);        /* the terminal can be read again */
                    continue;
                }
                break;
            }
            if (g_shell.interactive)
                putchar('\n');
            break;
        }
        if (r == INPUT_INTERRUPTED || r == INPUT_TOO_LONG) {
            block.len = 0;
            finish(r == INPUT_INTERRUPTED ? 130 : 1, 0);
            continue;
        }

        if (block.len == 0 && !expand_history(line, sizeof line)) {
            finish(1, 0);
            continue;
        }
        if (block_add(&block, line) != 0) {
            block.len = 0;
            finish(1, 0);
            continue;
        }

        struct script script;
        int parsed = run_parse(block.text, &script, 1);
        if (parsed == RUN_INCOMPLETE)
            continue;                       /* show "┆" and read more */
        block_to_history(&block);
        block.len = 0;
        if (parsed != 0) {
            finish(EXIT_SYNTAX, 0);
            continue;
        }
        if (script.nitems == 0) {           /* blank line or comment */
            script_free(&script);
            continue;
        }

        struct timespec start, end;
        g_shell.interrupted = 0;
        g_sigint = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
        int status = run_script(&script);
        clock_gettime(CLOCK_MONOTONIC, &end);
        finish(status, elapsed_ms(&start, &end));

        /* Builtin output must not lag behind the next command's output. */
        fflush(stdout);
    }
}

/* Runs ~/.trsh/baslangic if it exists. */
static void run_startup_file(void)
{
    char path[PATH_MAX];

    if (!config_path(path, sizeof path, STARTUP_FILE))
        return;
    FILE *f = fopen(path, "r");
    if (f == NULL)
        return;
    run_file(f);
    fclose(f);
    g_shell.interrupted = 0;
    g_shell.running = 1;
}

/* ------------------------------------------------------------ options */

enum mode { MODE_INTERACTIVE, MODE_FILE, MODE_STRING };

/* Reads the command line. Returns the mode, or -1 to exit with *code. */
static int parse_options(int argc, char **argv, const char **what, int *code)
{
    g_shell.param0 = "trsh";
    if (argc < 2)
        return MODE_INTERACTIVE;

    const char *a = argv[1];
    if (strcmp(a, "--surum") == 0 || strcmp(a, "-v") == 0) {
        printf("%s %s\n", TRSH_NAME, TRSH_VERSION);
        *code = 0;
        return -1;
    }
    if (strcmp(a, "--yardim") == 0 || strcmp(a, "-h") == 0) {
        printf(MSG_USAGE, TRSH_NAME, TRSH_VERSION);
        *code = 0;
        return -1;
    }
    if (strcmp(a, "-c") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%s: %s\n", TRSH_NAME, MSG_OPT_C_NEEDS_ARG);
            *code = EXIT_SYNTAX;
            return -1;
        }
        *what = argv[2];
        /* As in bash: trsh -c 'yaz $0 $1' ad birinci */
        if (argc > 3)
            g_shell.param0 = argv[3];
        g_shell.params = argc > 4 ? argv + 4 : NULL;
        g_shell.nparams = argc > 4 ? argc - 4 : 0;
        return MODE_STRING;
    }
    if (a[0] == '-' && a[1] != '\0') {
        fprintf(stderr, "%s: " MSG_UNKNOWN_OPTION_STR "\n", TRSH_NAME, a);
        fprintf(stderr, MSG_USAGE, TRSH_NAME, TRSH_VERSION);
        *code = EXIT_SYNTAX;
        return -1;
    }
    *what = a;
    g_shell.param0 = a;
    g_shell.params = argv + 2;
    g_shell.nparams = argc - 2;
    return MODE_FILE;
}

int main(int argc, char **argv)
{
    const char *what = NULL;
    int code = 0;
    int mode = parse_options(argc, argv, &what, &code);

    if (mode < 0)
        return code;
    g_shell.script_mode = mode != MODE_INTERACTIVE;

    ui_init();
    config_load();
    signals_init();
    jobs_init();

    if (mode == MODE_STRING) {
        code = run_text(what, 0);
    } else if (mode == MODE_FILE) {
        FILE *f = fopen(what, "r");
        if (f == NULL) {
            int err = errno;
            fprintf(stderr, "%s: %s: %s\n", TRSH_NAME, what, msg_error(err));
            return err == ENOENT ? 127 : 126;
        }
        code = run_file(f);
        fclose(f);
    } else {
        input_init();
        hist_init();
        if (g_shell.interactive)
            run_startup_file();
        if (g_shell.interactive)
            ui_banner();
        interactive_loop();
        code = g_shell.last_status;
    }

    fflush(stdout);
    jobs_cleanup();
    /* Back to the normal screen first, so the goodbye stays visible. */
    ui_shutdown();
    if (g_shell.interactive)
        ui_goodbye();
    return code;
}
