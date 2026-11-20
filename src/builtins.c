/*
 * builtins.c - shell'in kendi işleminde çalışan yerleşik komutlar.
 *
 * Bu komutlar shell'in kendi durumunu (çalışma dizini, döngü) değiştirdiği
 * için fork ile açılan bir çocukta çalıştırılamaz.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "builtins.h"
#include "ui.h"

static int builtin_cd(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "%scd:%s dizin argumani eksik\n", RED, RESET);
        return 1;
    }
    if (argc > 2) {
        fprintf(stderr, "%scd:%s cok fazla arguman\n", RED, RESET);
        return 1;
    }
    if (chdir(argv[1]) == -1) {
        fprintf(stderr, "%scd:%s %s: %s\n", RED, RESET, argv[1], strerror(errno));
        return 1;
    }
    return 0;
}

static int builtin_exit(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    g_shell.running = 0;
    return g_shell.last_status;
}

struct builtin {
    const char *name;
    int (*fn)(int argc, char **argv);
};

static const struct builtin builtins[] = {
    { "cd",   builtin_cd   },
    { "exit", builtin_exit },
};

int builtin_run(int argc, char **argv)
{
    size_t n = sizeof builtins / sizeof builtins[0];

    for (size_t i = 0; i < n; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0) {
            g_shell.last_status = builtins[i].fn(argc, argv);
            return 1;
        }
    }
    return 0;
}
