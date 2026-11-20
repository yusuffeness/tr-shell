/*
 * executor.c - harici komutları çocuk işlemde çalıştırma.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "executor.h"
#include "ui.h"

#define EXIT_NOT_FOUND 127   /* bash ile aynı: komut bulunamadı */
#define EXIT_CANNOT_RUN 126  /* bash ile aynı: bulundu ama çalıştırılamadı */

static void child_exec(char **argv)
{
    execvp(argv[0], argv);

    /* execvp yalnızca hata olursa döner. */
    int err = errno;
    if (err == ENOENT)
        fprintf(stderr, "%s%s:%s komut bulunamadi\n", RED, argv[0], RESET);
    else
        fprintf(stderr, "%s%s:%s %s\n", RED, argv[0], RESET, strerror(err));
    _exit(err == ENOENT ? EXIT_NOT_FOUND : EXIT_CANNOT_RUN);
}

static int wait_child(pid_t pid)
{
    int status;

    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        fprintf(stderr, "%s%s%s\n", RED, strsignal(sig), RESET);
        return 128 + sig;
    }
    return 1;
}

void executor_run(char **argv)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        g_shell.last_status = 1;
        return;
    }
    if (pid == 0)
        child_exec(argv);

    g_shell.last_status = wait_child(pid);
}
