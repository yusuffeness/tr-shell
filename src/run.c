/*
 * run.c - running shell text: a line, a file, or a $(...) capture.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "run.h"
#include "alias.h"
#include "executor.h"
#include "jobs.h"
#include "lexer.h"
#include "messages.h"
#include "signals.h"
#include "ui.h"

#define MAX_SOURCE_DEPTH 32

int run_parse(const char *text, struct script *out, int allow_incomplete)
{
    struct tokens tokens;

    out->items = NULL;
    out->nitems = 0;
    if (lex(text, &tokens) != 0)
        return -1;
    if (alias_expand(&tokens) != 0) {
        tokens_free(&tokens);
        return -1;
    }
    int r = parse(&tokens, out);
    tokens_free(&tokens);

    if (r == PARSE_INCOMPLETE) {
        if (allow_incomplete)
            return RUN_INCOMPLETE;
        ui_error(TRSH_NAME, "%s", MSG_SYNTAX_MISSING_END);
        return -1;
    }
    return r;
}

int run_script(struct script *s)
{
    int status = s->nitems > 0 ? exec_script(s) : g_shell.last_status;
    script_free(s);
    return status;
}

int run_text(const char *text, int allow_incomplete)
{
    struct script s;
    int r = run_parse(text, &s, allow_incomplete);

    if (r == RUN_INCOMPLETE)
        return r;
    if (r != 0)
        return EXIT_SYNTAX;
    return run_script(&s);
}

/* ---------------------------------------------------------------- files */

struct text {
    char *s;
    size_t len, cap;
};

static int text_add(struct text *t, const char *s)
{
    size_t n = strlen(s);

    if (t->len + n + 2 > t->cap) {
        size_t cap = t->cap ? t->cap : 256;
        while (cap < t->len + n + 2)
            cap *= 2;
        char *grown = realloc(t->s, cap);
        if (grown == NULL)
            return -1;
        t->s = grown;
        t->cap = cap;
    }
    if (t->len > 0)
        t->s[t->len++] = '\n';
    memcpy(t->s + t->len, s, n + 1);
    t->len += n;
    return 0;
}

int run_file(FILE *f)
{
    struct text acc = { NULL, 0, 0 };
    char line[MAX_LINE];

    while (g_shell.running && !g_shell.interrupted && fgets(line, sizeof line, f) != NULL) {
        size_t n = strcspn(line, "\n");
        if (line[n] != '\n' && !feof(f)) {
            int c;
            while ((c = fgetc(f)) != '\n' && c != EOF)
                ;
            ui_error(TRSH_NAME, MSG_LINE_TOO_LONG, MAX_LINE - 2);
            g_shell.last_status = 1;
            continue;
        }
        line[n] = '\0';
        if (text_add(&acc, line) != 0)
            break;

        int r = run_text(acc.s, 1);
        if (r == RUN_INCOMPLETE)
            continue;           /* inside eger / dongu / her: read on */
        g_shell.last_status = r;
        acc.len = 0;
        acc.s[0] = '\0';
    }

    /* The file ended in the middle of a block. */
    if (acc.len > 0 && g_shell.running && !g_shell.interrupted)
        g_shell.last_status = run_text(acc.s, 0);
    free(acc.s);
    return g_shell.last_status;
}

/* --------------------------------------------------------------- $(...) */

int run_capture(const char *text, char **out)
{
    struct text buf = { NULL, 0, 0 };
    char chunk[4096];
    int p[2];
    int status;

    *out = NULL;
    if (pipe(p) < 0) {
        perror("pipe");
        *out = strdup("");
        return 1;
    }
    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(p[0]);
        close(p[1]);
        *out = strdup("");
        return 1;
    }
    if (pid == 0) {
        close(p[0]);
        dup2(p[1], STDOUT_FILENO);
        close(p[1]);
        signals_child();
        jobs_disable();
        g_shell.interactive = 0;
        status = run_text(text, 0);
        fflush(stdout);
        fflush(stderr);
        _exit(status == RUN_INCOMPLETE ? EXIT_SYNTAX : status);
    }

    close(p[1]);
    buf.s = malloc(1);
    if (buf.s != NULL) {
        buf.s[0] = '\0';
        buf.cap = 1;
    }
    for (;;) {
        ssize_t n = read(p[0], chunk, sizeof chunk);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        if (buf.s == NULL)
            continue;
        if (buf.len + (size_t)n + 1 > buf.cap) {
            size_t cap = buf.cap * 2;
            while (cap < buf.len + (size_t)n + 1)
                cap *= 2;
            char *grown = realloc(buf.s, cap);
            if (grown == NULL) {
                free(buf.s);
                buf.s = NULL;
                continue;
            }
            buf.s = grown;
            buf.cap = cap;
        }
        memcpy(buf.s + buf.len, chunk, (size_t)n);
        buf.len += (size_t)n;
        buf.s[buf.len] = '\0';
    }
    close(p[0]);

    while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
        ;
    *out = buf.s;
    if (WIFSIGNALED(status)) {
        if (WTERMSIG(status) == SIGINT)
            g_shell.interrupted = 1;
        return 128 + WTERMSIG(status);
    }
    return WEXITSTATUS(status);
}

/* --------------------------------------------------------------- kaynak */

int bi_kaynak(int argc, char **argv)
{
    static int depth;

    if (argc != 2) {
        ui_error(argv[0], "%s %s", MSG_HELP_USAGE, "kaynak DOSYA");
        return 1;
    }
    if (depth >= MAX_SOURCE_DEPTH) {
        ui_error(argv[0], "%s", MSG_SOURCE_TOO_DEEP);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        ui_error(argv[0], "%s: %s", argv[1], msg_error(errno));
        return 1;
    }
    depth++;
    int status = run_file(f);
    depth--;
    fclose(f);
    return status;
}
