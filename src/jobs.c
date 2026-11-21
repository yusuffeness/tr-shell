/*
 * jobs.c - jobs and job control.
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "jobs.h"
#include "messages.h"
#include "shell.h"
#include "ui.h"
#include "util.h"

enum proc_state { P_RUNNING, P_STOPPED, P_DONE };

struct proc {
    pid_t pid;
    enum proc_state state;
    int status;             /* raw wait status once done */
};

struct job {
    int id;
    pid_t pgid;             /* 0 without job control */
    struct proc *procs;
    size_t nprocs;
    char *text;
    struct termios tmodes;  /* terminal settings when it was stopped */
    int has_tmodes;
    struct job *next;
};

static struct job *jobs;            /* newest last */
static int job_control;
static pid_t shell_pgid;
static struct termios shell_tmodes;

/* ------------------------------------------------------------- setup */

void jobs_init(void)
{
    if (!g_shell.interactive || !isatty(STDIN_FILENO))
        return;

    /* If we were started in the background, wait until we are in front. */
    while (tcgetpgrp(STDIN_FILENO) != (shell_pgid = getpgrp()))
        kill(-shell_pgid, SIGTTIN);

    /* Our own process group, owning the terminal. */
    if (setpgid(0, 0) == 0)
        shell_pgid = getpid();
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) != 0)
        return;
    tcgetattr(STDIN_FILENO, &shell_tmodes);
    job_control = 1;
}

int jobs_control(void)
{
    return job_control;
}

void jobs_disable(void)
{
    job_control = 0;
}

pid_t jobs_shell_pgid(void)
{
    return shell_pgid;
}

/* ---------------------------------------------------------- the table */

static int next_id(void)
{
    int max = 0;
    for (struct job *j = jobs; j; j = j->next)
        if (j->id > max)
            max = j->id;
    return max + 1;
}

struct job *jobs_add(pid_t pgid, const pid_t *pids, size_t n, const char *text)
{
    struct job *j = calloc(1, sizeof *j);

    if (j == NULL)
        return NULL;
    j->procs = calloc(n, sizeof *j->procs);
    j->text = strdup(text ? text : "");
    if (j->procs == NULL || j->text == NULL) {
        free(j->procs);
        free(j->text);
        free(j);
        return NULL;
    }
    for (size_t i = 0; i < n; i++)
        j->procs[i].pid = pids[i];
    j->nprocs = n;
    j->pgid = pgid;
    j->id = next_id();

    struct job **tail = &jobs;
    while (*tail)
        tail = &(*tail)->next;
    *tail = j;
    return j;
}

static void job_remove(struct job *j)
{
    for (struct job **p = &jobs; *p; p = &(*p)->next) {
        if (*p == j) {
            *p = j->next;
            break;
        }
    }
    free(j->procs);
    free(j->text);
    free(j);
}

static int job_done(const struct job *j)
{
    for (size_t i = 0; i < j->nprocs; i++)
        if (j->procs[i].state != P_DONE)
            return 0;
    return 1;
}

static int job_stopped(const struct job *j)
{
    int any = 0;
    for (size_t i = 0; i < j->nprocs; i++) {
        if (j->procs[i].state == P_RUNNING)
            return 0;
        if (j->procs[i].state == P_STOPPED)
            any = 1;
    }
    return any;
}

/* Exit status of a job: that of its last process, 128 + N if killed. */
static int job_status(const struct job *j)
{
    int st = j->procs[j->nprocs - 1].status;

    if (WIFEXITED(st))
        return WEXITSTATUS(st);
    if (WIFSIGNALED(st))
        return 128 + WTERMSIG(st);
    return 1;
}

/* Records a status change reported by waitpid for one of j's processes. */
static void update(struct job *j, size_t i, int st)
{
    if (WIFSTOPPED(st)) {
        j->procs[i].state = P_STOPPED;
    } else if (WIFCONTINUED(st)) {
        j->procs[i].state = P_RUNNING;
    } else {
        j->procs[i].state = P_DONE;
        j->procs[i].status = st;
    }
}

static void job_signal(struct job *j, int sig)
{
    if (j->pgid > 0) {
        kill(-j->pgid, sig);
        return;
    }
    for (size_t i = 0; i < j->nprocs; i++)
        if (j->procs[i].state != P_DONE)
            kill(j->procs[i].pid, sig);
}

static const char *state_name(const struct job *j)
{
    if (job_done(j))
        return job_status(j) == 0 ? MSG_JOB_DONE : MSG_JOB_FAILED;
    return job_stopped(j) ? MSG_JOB_STOPPED : MSG_JOB_RUNNING;
}

/* The job `on` and `arka` use when no number is given: the newest one. */
static struct job *current_job(void)
{
    struct job *last = NULL;
    for (struct job *j = jobs; j; j = j->next)
        last = j;
    return last;
}

static void print_job(const struct job *j, int with_amp)
{
    int failed = job_done(j) && job_status(j) != 0;
    const char *state = state_name(j);

    /* Pad by display width: "Çalışıyor" has more bytes than letters. */
    printf("[%d]%s  %s%*s %s%s", j->id, j == current_job() ? "+" : " ",
           state, 12 - text_width(state), "", j->text, with_amp ? " &" : "");
    if (failed)
        printf(" %s(%d)%s", GRAY, job_status(j), RESET);
    putchar('\n');
}

/* --------------------------------------------------------- foreground */

/* Reports how the last process of a foreground job ended, like bash. */
static void report_signal(const struct job *j)
{
    int st = j->procs[j->nprocs - 1].status;

    if (!WIFSIGNALED(st))
        return;
    int sig = WTERMSIG(st);
    if (sig == SIGINT)
        fputc('\n', stderr);
    else if (sig != SIGPIPE)
        fprintf(stderr, "%s%s%s\n", RED, strsignal(sig), RESET);
}

int jobs_wait_foreground(struct job *j)
{
    if (job_control) {
        tcsetpgrp(STDIN_FILENO, j->pgid);
        if (j->has_tmodes)
            tcsetattr(STDIN_FILENO, TCSADRAIN, &j->tmodes);
    }

    for (size_t i = 0; i < j->nprocs; i++) {
        int st;
        pid_t r;

        if (j->procs[i].state == P_DONE)
            continue;
        do
            r = waitpid(j->procs[i].pid, &st, WUNTRACED);
        while (r < 0 && errno == EINTR);
        if (r < 0) {
            j->procs[i].state = P_DONE;     /* already reaped elsewhere */
            continue;
        }
        update(j, i, st);
        if (WIFSTOPPED(st))
            break;
    }

    /* Mark the rest stopped too: Ctrl+Z stops the whole process group. */
    int stopped = !job_done(j);
    if (stopped)
        for (size_t i = 0; i < j->nprocs; i++)
            if (j->procs[i].state == P_RUNNING)
                j->procs[i].state = P_STOPPED;

    if (job_control) {
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        if (stopped) {
            tcgetattr(STDIN_FILENO, &j->tmodes);
            j->has_tmodes = 1;
        }
        tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_tmodes);
    }

    if (stopped) {
        putchar('\n');
        print_job(j, 0);
        return 128 + SIGTSTP;
    }

    report_signal(j);
    int status = job_status(j);
    job_remove(j);
    return status;
}

void jobs_announce_background(const struct job *j)
{
    printf("[%d] %ld\n", j->id, (long)j->procs[j->nprocs - 1].pid);
}

/* --------------------------------------------------------- background */

/* notify_stop is off for `isler`, which lists stopped jobs anyway. */
static void poll_jobs(int notify_done, int notify_stop)
{
    struct job *j = jobs;

    while (j) {
        struct job *following = j->next;
        int was_stopped = job_stopped(j);

        for (size_t i = 0; i < j->nprocs; i++) {
            int st;
            if (j->procs[i].state == P_DONE)
                continue;
            pid_t r = waitpid(j->procs[i].pid, &st, WNOHANG | WUNTRACED | WCONTINUED);
            if (r > 0)
                update(j, i, st);
            else if (r < 0 && errno == ECHILD)
                j->procs[i].state = P_DONE;
        }

        if (job_done(j)) {
            if (notify_done)
                print_job(j, 0);
            job_remove(j);
        } else if (notify_stop && job_stopped(j) && !was_stopped) {
            print_job(j, 0);
        }
        j = following;
    }
    fflush(stdout);
}

void jobs_poll(int notify)
{
    poll_jobs(notify, notify);
}

void jobs_cleanup(void)
{
    for (struct job *j = jobs; j; j = j->next) {
        if (job_stopped(j)) {
            job_signal(j, SIGHUP);
            job_signal(j, SIGCONT);
        }
    }
}

/* ------------------------------------------------------------ builtins */

int bi_isler(int argc, char **argv)
{
    if (argc > 1) {
        ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
        return 1;
    }
    poll_jobs(1, 0);
    for (struct job *j = jobs; j; j = j->next)
        print_job(j, !job_stopped(j));
    return 0;
}

/* Finds the job named by "N" or "%N", or the current job if arg is NULL. */
static struct job *find_job(const char *who, const char *arg)
{
    if (arg == NULL) {
        struct job *j = current_job();
        if (j == NULL)
            ui_error(who, "%s", MSG_NO_JOBS);
        return j;
    }
    if (*arg == '%')
        arg++;
    for (const char *c = arg; *c; c++) {
        if (!isdigit((unsigned char)*c)) {
            ui_error(who, MSG_BAD_NUMBER, arg);
            return NULL;
        }
    }
    int id = atoi(arg);
    for (struct job *j = jobs; j; j = j->next)
        if (j->id == id)
            return j;
    ui_error(who, MSG_NO_SUCH_JOB, arg);
    return NULL;
}

int bi_on(int argc, char **argv)
{
    if (argc > 2) {
        ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
        return 1;
    }
    jobs_poll(1);
    struct job *j = find_job(argv[0], argc == 2 ? argv[1] : NULL);
    if (j == NULL)
        return 1;

    printf("%s\n", j->text);
    fflush(stdout);
    if (job_stopped(j)) {
        for (size_t i = 0; i < j->nprocs; i++)
            if (j->procs[i].state == P_STOPPED)
                j->procs[i].state = P_RUNNING;
        /* Hand over the terminal first, then wake the job up. */
        if (job_control) {
            tcsetpgrp(STDIN_FILENO, j->pgid);
            if (j->has_tmodes)
                tcsetattr(STDIN_FILENO, TCSADRAIN, &j->tmodes);
        }
        job_signal(j, SIGCONT);
    }
    return jobs_wait_foreground(j);
}

int bi_arka(int argc, char **argv)
{
    if (argc > 2) {
        ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
        return 1;
    }
    jobs_poll(1);
    struct job *j = find_job(argv[0], argc == 2 ? argv[1] : NULL);
    if (j == NULL)
        return 1;
    if (!job_stopped(j)) {
        ui_error(argv[0], MSG_JOB_NOT_STOPPED, j->id);
        return 1;
    }
    for (size_t i = 0; i < j->nprocs; i++)
        if (j->procs[i].state == P_STOPPED)
            j->procs[i].state = P_RUNNING;
    job_signal(j, SIGCONT);
    print_job(j, 1);
    return 0;
}
