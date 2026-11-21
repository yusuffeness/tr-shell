/*
 * signals.c - keyboard signals in an interactive shell.
 */
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <string.h>

#include "signals.h"
#include "shell.h"

volatile sig_atomic_t g_sigint;

static void on_sigint(int sig)
{
    (void)sig;
    g_sigint = 1;
}

void signals_init(void)
{
    struct sigaction sa;

    if (!g_shell.interactive)
        return;

    /*
     * No SA_RESTART: a blocking read() at the prompt must return with
     * EINTR so the line can be dropped. waitpid() retries on EINTR itself.
     */
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    /*
     * The shell itself must never be stopped: Ctrl+Z is for the foreground
     * job, and SIGTTOU/SIGTTIN would stop us when we hand the terminal
     * back and forth with tcsetpgrp().
     */
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}

void signals_child(void)
{
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
}
