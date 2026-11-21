/*
 * executor.c - running external programs in a child process.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "executor.h"
#include "messages.h"
#include "signals.h"
#include "suggest.h"
#include "ui.h"

#define EXIT_NOT_FOUND  127   /* same as bash: command not found */
#define EXIT_CANNOT_RUN 126   /* same as bash: found but not executable */

static void child_exec(char *const argv[])
{
    signals_child();
    execvp(argv[0], argv);

    /* execvp only returns on failure. */
    int err = errno;
    if (err == ENOENT) {
        const char *guess = suggest_command(argv[0]);
        if (guess != NULL)
            ui_error(argv[0], "%s. " MSG_DID_YOU_MEAN, MSG_NOT_FOUND, guess);
        else
            ui_error(argv[0], "%s", MSG_NOT_FOUND);
    } else {
        ui_error(argv[0], "%s", msg_error(err));
    }
    _exit(err == ENOENT ? EXIT_NOT_FOUND : EXIT_CANNOT_RUN);
}

static int wait_child(pid_t pid)
{
    int status;

    /* Ctrl+C interrupts waitpid with EINTR; the child gets the signal too. */
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }
    g_sigint = 0;

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);

        /* Like bash: Ctrl+C just ends the line, a broken pipe is silent. */
        if (sig == SIGINT)
            fputc('\n', stderr);
        else if (sig != SIGPIPE)
            fprintf(stderr, "%s%s%s\n", RED, strsignal(sig), RESET);
        return 128 + sig;
    }
    return 1;
}

int executor_run(char *const argv[])
{
    /*
     * Flush our own buffered output first; otherwise text printed by a
     * builtin could appear after the child's output when stdout is a pipe.
     */
    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0)
        child_exec(argv);

    return wait_child(pid);
}
