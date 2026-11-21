/*
 * executor.c - running parsed command lines.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "executor.h"
#include "commands.h"
#include "expand.h"
#include "jobs.h"
#include "messages.h"
#include "signals.h"
#include "ui.h"
#include "util.h"

/* A command after expansion: what is actually run. */
struct xredir {
    enum redir_kind kind;
    int fd;
    char *path;
};

struct xcmd {
    const struct command *compound;     /* eger / dongu / her, or NULL */
    struct strvec argv;
    struct strvec assigns;      /* NAME=value pairs before the command */
    struct xredir *redirs;
    size_t nredirs;
};

/* ------------------------------------------------------------ expansion */

static void xcmd_free(struct xcmd *x)
{
    strvec_free(&x->argv);
    strvec_free(&x->assigns);
    for (size_t i = 0; i < x->nredirs; i++)
        free(x->redirs[i].path);
    free(x->redirs);
}

static int expand_command(const struct command *c, struct xcmd *x)
{
    size_t i = 0;

    memset(x, 0, sizeof *x);

    /* A compound command expands its own words when it runs. */
    if (c->type != CT_SIMPLE)
        x->compound = c;

    /* Leading NAME=value words are assignments, not the command. */
    for (; !x->compound && i < c->words.n && is_assignment(c->words.v[i]); i++) {
        char *v = expand_single(c->words.v[i]);
        if (v == NULL || strvec_push_owned(&x->assigns, v) != 0) {
            free(v);
            return -1;
        }
    }
    for (; !x->compound && i < c->words.n; i++)
        if (expand_word(c->words.v[i], &x->argv) != 0)
            return -1;

    if (c->nredirs > 0) {
        x->redirs = calloc(c->nredirs, sizeof *x->redirs);
        if (x->redirs == NULL)
            return -1;
        x->nredirs = c->nredirs;
        for (size_t r = 0; r < c->nredirs; r++) {
            x->redirs[r].kind = c->redirs[r].kind;
            x->redirs[r].fd = c->redirs[r].fd;
            if (c->redirs[r].target != NULL) {
                x->redirs[r].path = expand_single(c->redirs[r].target);
                if (x->redirs[r].path == NULL)
                    return -1;
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------- redirections */

/* Applies x's redirections to this process. Returns 0, or -1 (reported). */
static int apply_redirs(const struct xcmd *x)
{
    for (size_t i = 0; i < x->nredirs; i++) {
        const struct xredir *r = &x->redirs[i];
        int fd;

        if (r->kind == R_ERR_TO_OUT) {
            if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
                return -1;
            continue;
        }
        if (r->kind == R_IN)
            fd = open(r->path, O_RDONLY);
        else if (r->kind == R_OUT)
            fd = open(r->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        else
            fd = open(r->path, O_WRONLY | O_CREAT | O_APPEND, 0644);

        if (fd < 0) {
            ui_error(r->path, "%s", msg_error(errno));
            return -1;
        }
        if (fd != r->fd) {
            dup2(fd, r->fd);
            close(fd);
        }
    }
    return 0;
}

static void apply_assigns(const struct xcmd *x)
{
    for (size_t i = 0; i < x->assigns.n; i++) {
        char *eq = strchr(x->assigns.v[i], '=');
        *eq = '\0';
        setenv(x->assigns.v[i], eq + 1, 1);
        *eq = '=';
    }
}

/*
 * Runs fn (a builtin, or nothing) inside the shell with x's redirections,
 * then puts stdin, stdout and stderr back as they were.
 */
static int run_here(int (*fn)(int, char **), struct xcmd *x)
{
    int saved[3] = { -1, -1, -1 };
    int status;

    fflush(stdout);
    fflush(stderr);
    if (x->nredirs > 0)
        for (int fd = 0; fd < 3; fd++)
            saved[fd] = fcntl(fd, F_DUPFD_CLOEXEC, 10);

    if (apply_redirs(x) != 0)
        status = 1;
    else if (x->compound != NULL)
        status = exec_compound(x->compound);
    else if (fn != NULL)
        status = fn((int)x->argv.n, x->argv.v);
    else
        status = 0;

    fflush(stdout);
    fflush(stderr);
    for (int fd = 0; fd < 3; fd++) {
        if (saved[fd] >= 0) {
            dup2(saved[fd], fd);
            close(saved[fd]);
        }
    }
    return status;
}

/* ------------------------------------------------------------ processes */

/* In the child: wire up stdin/stdout, redirect, then run. Never returns. */
static void child_run(struct xcmd *x, int in_fd, int out_fd, int background)
{
    signals_child();

    if (in_fd >= 0) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    } else if (background && !jobs_control()) {
        /* Without job control a background job must not read the terminal. */
        int null = open("/dev/null", O_RDONLY);
        if (null >= 0) {
            dup2(null, STDIN_FILENO);
            close(null);
        }
    }
    if (out_fd >= 0) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }
    if (apply_redirs(x) != 0)
        _exit(1);
    if (x->compound != NULL) {
        /* A loop in a pipeline runs in this child, like a subshell. */
        jobs_disable();
        g_shell.interactive = 0;
        int status = exec_compound(x->compound);
        fflush(stdout);
        fflush(stderr);
        _exit(status);
    }
    apply_assigns(x);
    if (x->argv.n == 0)
        _exit(0);
    commands_exec_child((int)x->argv.n, x->argv.v);
}

/* Puts a new child in the job's process group and, in front, on the terminal. */
static void join_group(pid_t pid, pid_t *pgid, int background)
{
    if (!jobs_control())
        return;
    if (*pgid == 0)
        *pgid = pid;
    setpgid(pid, *pgid);
    if (!background && pid == getpid())
        tcsetpgrp(STDIN_FILENO, *pgid);
}

/* Starts every command of a pipeline, connected by pipes. */
static int launch(struct xcmd *xs, size_t n, int background, const char *text)
{
    pid_t *pids = calloc(n, sizeof *pids);
    pid_t pgid = 0;
    size_t started = 0;
    int in_fd = -1;

    if (pids == NULL)
        return 1;
    fflush(stdout);
    fflush(stderr);

    for (size_t i = 0; i < n; i++) {
        int p[2] = { -1, -1 };

        if (i + 1 < n && pipe(p) < 0) {
            perror("pipe");
            break;
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (p[0] >= 0) {
                close(p[0]);
                close(p[1]);
            }
            break;
        }
        if (pid == 0) {
            /* Both sides call setpgid: whichever runs first wins the race. */
            pid_t self = getpid();
            join_group(self, &pgid, background);
            if (p[0] >= 0)
                close(p[0]);
            child_run(&xs[i], in_fd, p[1], background);
        }

        join_group(pid, &pgid, background);
        pids[started++] = pid;
        if (in_fd >= 0)
            close(in_fd);
        if (p[1] >= 0)
            close(p[1]);
        in_fd = p[0];
    }
    if (in_fd >= 0)
        close(in_fd);

    int status = 1;
    if (started > 0) {
        struct job *j = jobs_add(pgid, pids, started, text);
        if (j == NULL) {
            for (size_t i = 0; i < started; i++)
                waitpid(pids[i], NULL, 0);
        } else if (background) {
            jobs_announce_background(j);
            status = 0;
        } else {
            status = jobs_wait_foreground(j);
        }
    }
    free(pids);
    return status;
}

/* --------------------------------------------------------- the tree */

static int run_pipeline(const struct pipeline *p, int background, const char *text)
{
    struct xcmd *xs = calloc(p->ncmds, sizeof *xs);
    size_t expanded = 0;
    int status = 1;

    if (xs == NULL)
        return 1;
    for (; expanded < p->ncmds; expanded++)
        if (expand_command(&p->cmds[expanded], &xs[expanded]) != 0) {
            expanded++;
            goto done;
        }

    if (p->ncmds == 1 && !background) {
        struct xcmd *x = &xs[0];

        /* eger / dongu / her: run in the shell, so variables stick. */
        if (x->compound != NULL) {
            status = run_here(NULL, x);
            goto done;
        }
        /* Only assignments and/or redirections: X=1, > empty.txt */
        if (x->argv.n == 0) {
            apply_assigns(x);
            status = run_here(NULL, x);
            goto done;
        }
        const struct command_entry *cmd = commands_find(x->argv.v[0]);
        if (cmd != NULL && cmd->kind == CMD_BUILTIN && x->assigns.n == 0) {
            status = run_here(cmd->fn, x);
            goto done;
        }
    }
    status = launch(xs, p->ncmds, background, text);

done:
    for (size_t i = 0; i < expanded; i++)
        xcmd_free(&xs[i]);
    free(xs);
    return status;
}

/* Stop running the rest of a list: exit, kir / devam, or Ctrl+C. */
static int should_stop(void)
{
    if (g_sigint) {
        g_sigint = 0;
        g_shell.interrupted = 1;
    }
    return !g_shell.running || g_shell.loop_ctl != LOOP_NONE || g_shell.interrupted;
}

static int run_and_or(const struct and_or *ao, const char *text)
{
    int status = run_pipeline(&ao->pipes[0], 0, text);
    g_shell.last_status = status;

    for (size_t i = 1; i < ao->npipes && !should_stop(); i++) {
        enum andor_op op = ao->ops[i - 1];
        if ((op == OP_AND && status != 0) || (op == OP_OR && status == 0))
            continue;
        status = run_pipeline(&ao->pipes[i], 0, text);
        g_shell.last_status = status;
    }
    return status;
}

/* `a && b &`: the whole list runs in a forked copy of the shell. */
static int run_in_background(const struct item *it)
{
    if (it->ao.npipes == 1)
        return run_pipeline(&it->ao.pipes[0], 1, it->text);

    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        if (jobs_control())
            setpgid(0, 0);
        signals_child();
        jobs_disable();
        g_shell.interactive = 0;
        int status = run_and_or(&it->ao, it->text);
        fflush(stdout);
        fflush(stderr);
        _exit(status);
    }

    pid_t pgid = 0;
    if (jobs_control()) {
        pgid = pid;
        setpgid(pid, pid);
    }
    struct job *j = jobs_add(pgid, &pid, 1, it->text);
    if (j != NULL)
        jobs_announce_background(j);
    return 0;
}

int exec_script(const struct script *s)
{
    int status = g_shell.last_status;

    for (size_t i = 0; i < s->nitems && !should_stop(); i++) {
        const struct item *it = &s->items[i];

        status = it->background ? run_in_background(it) : run_and_or(&it->ao, it->text);
        g_shell.last_status = status;
    }
    return status;
}

/* ------------------------------------------------------------ compound */

/* After one pass of a loop body: 1 if the loop should end. */
static int loop_should_end(void)
{
    if (g_shell.loop_ctl == LOOP_BREAK) {
        g_shell.loop_ctl = LOOP_NONE;
        return 1;
    }
    if (g_shell.loop_ctl == LOOP_CONTINUE)
        g_shell.loop_ctl = LOOP_NONE;
    return !g_shell.running || g_shell.interrupted;
}

static int run_while(const struct command *c)
{
    int status = 0;

    g_shell.loop_depth++;
    for (;;) {
        int cond = exec_script(c->cond);
        if (should_stop() || cond != 0)
            break;
        status = exec_script(c->body);
        if (loop_should_end())
            break;
    }
    g_shell.loop_depth--;
    return status;
}

static int run_for(const struct command *c)
{
    struct strvec words = { NULL, 0, 0 };
    int status = 0;

    for (size_t i = 0; i < c->words.n; i++)
        if (expand_word(c->words.v[i], &words) != 0) {
            strvec_free(&words);
            return 1;
        }

    g_shell.loop_depth++;
    for (size_t i = 0; i < words.n; i++) {
        setenv(c->var, words.v[i], 1);
        status = exec_script(c->body);
        if (loop_should_end())
            break;
    }
    g_shell.loop_depth--;
    strvec_free(&words);
    return status;
}

int exec_compound(const struct command *c)
{
    switch (c->type) {
    case CT_IF: {
        int cond = exec_script(c->cond);
        if (should_stop())
            return cond;
        if (cond == 0)
            return exec_script(c->body);
        return c->else_body ? exec_script(c->else_body) : 0;
    }
    case CT_WHILE:
        return run_while(c);
    case CT_FOR:
        return run_for(c);
    default:
        return 0;
    }
}

int exec_argv(int argc, char **argv)
{
    struct xcmd x;
    char text[MAX_LINE];
    int status;

    if (argc < 1)
        return 0;
    const struct command_entry *cmd = commands_find(argv[0]);
    if (cmd != NULL && cmd->kind == CMD_BUILTIN)
        return cmd->fn(argc, argv);

    memset(&x, 0, sizeof x);
    for (int i = 0; i < argc; i++) {
        if (strvec_push(&x.argv, argv[i]) != 0) {
            xcmd_free(&x);
            return 1;
        }
    }
    if (!join_args(text, sizeof text, argc, argv))
        snprintf(text, sizeof text, "%s", argv[0]);
    status = launch(&x, 1, 0, text);
    xcmd_free(&x);
    return status;
}

int executor_run(char *const argv[])
{
    int argc = 0;

    while (argv[argc] != NULL)
        argc++;
    return exec_argv(argc, (char **)argv);
}
