/*
 * builtins.c - commands that run inside the shell process.
 *
 * These change the shell's own state (working directory, main loop), so
 * they cannot run in a forked child.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "builtins.h"
#include "messages.h"
#include "ui.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Rejects extra arguments for commands that take at most max_args. */
static int too_many(int argc, char **argv, int max_args)
{
    if (argc - 1 > max_args) {
        ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
        return 1;
    }
    return 0;
}

/*
 * Changes directory and keeps PWD / OLDPWD up to date.
 * If print is set, the new directory is printed (like `cd -` in bash).
 */
static int change_dir(const char *who, const char *path, int print)
{
    char old[PATH_MAX];
    char now[PATH_MAX];
    int have_old = getcwd(old, sizeof old) != NULL;

    if (chdir(path) == -1) {
        ui_error(who, "%s: %s", path, msg_error(errno));
        return 1;
    }
    if (have_old)
        setenv("OLDPWD", old, 1);
    if (getcwd(now, sizeof now) != NULL) {
        setenv("PWD", now, 1);
        if (print)
            puts(now);
    }
    return 0;
}

static const char *home_dir(const char *who)
{
    const char *home = getenv("HOME");

    if (home == NULL || *home == '\0') {
        ui_error(who, "%s", MSG_NO_HOME);
        return NULL;
    }
    return home;
}

int bi_cd(int argc, char **argv)
{
    if (argc < 2) {
        ui_error(argv[0], "%s", MSG_MISSING_DIR);
        return 1;
    }
    if (too_many(argc, argv, 1))
        return 1;
    return change_dir(argv[0], argv[1], 0);
}

int bi_gec(int argc, char **argv)
{
    if (too_many(argc, argv, 1))
        return 1;
    if (argc == 2)
        return change_dir(argv[0], argv[1], 0);

    const char *home = home_dir(argv[0]);
    return home ? change_dir(argv[0], home, 0) : 1;
}

int bi_geri(int argc, char **argv)
{
    char target[PATH_MAX];
    const char *old = getenv("OLDPWD");

    if (too_many(argc, argv, 0))
        return 1;
    if (old == NULL || *old == '\0') {
        ui_error(argv[0], "%s", MSG_NO_OLDPWD);
        return 1;
    }
    /* change_dir overwrites OLDPWD, so keep a copy of the target. */
    snprintf(target, sizeof target, "%s", old);
    return change_dir(argv[0], target, 1);
}

int bi_ev(int argc, char **argv)
{
    if (too_many(argc, argv, 0))
        return 1;

    const char *home = home_dir(argv[0]);
    return home ? change_dir(argv[0], home, 0) : 1;
}

int bi_nerdeyim(int argc, char **argv)
{
    char cwd[PATH_MAX];

    if (too_many(argc, argv, 0))
        return 1;
    if (getcwd(cwd, sizeof cwd) == NULL) {
        ui_error(argv[0], "%s", msg_error(errno));
        return 1;
    }
    puts(cwd);
    return 0;
}

int bi_exit(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    g_shell.running = 0;
    return g_shell.last_status;
}

int bi_tarih(int argc, char **argv)
{
    static const char *const days[] = {
        "Pazar", "Pazartesi", "Salı", "Çarşamba", "Perşembe", "Cuma", "Cumartesi",
    };
    static const char *const months[] = {
        "Ocak", "Şubat", "Mart", "Nisan", "Mayıs", "Haziran",
        "Temmuz", "Ağustos", "Eylül", "Ekim", "Kasım", "Aralık",
    };

    if (too_many(argc, argv, 0))
        return 1;

    time_t now = time(NULL);
    struct tm tm;
    if (localtime_r(&now, &tm) == NULL) {
        ui_error(argv[0], "%s", msg_error(errno));
        return 1;
    }
    printf("%s, %d %s %d · %02d:%02d\n", days[tm.tm_wday], tm.tm_mday,
           months[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
    return 0;
}

int bi_hakkinda(int argc, char **argv)
{
    if (too_many(argc, argv, 0))
        return 1;

    printf("%s%s%s %s\n", BOLD, TRSH_NAME, RESET, TRSH_VERSION);
    printf("%s  %s\n", MSG_ABOUT_AUTHOR, TRSH_USER);
    printf("%s %s\n", MSG_ABOUT_LICENSE, "MIT");
    printf("%s\n", TRSH_URL);
    return 0;
}
