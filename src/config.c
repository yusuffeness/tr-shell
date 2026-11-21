/*
 * config.c - paths under the per-user directory ~/.trsh.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "config.h"
#include "messages.h"
#include "ui.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define CONFIG_DIR ".trsh"

int config_path(char *buf, size_t size, const char *file)
{
    const char *home = getenv("HOME");
    int n;

    if (home == NULL || *home == '\0')
        return 0;
    if (file)
        n = snprintf(buf, size, "%s/%s/%s", home, CONFIG_DIR, file);
    else
        n = snprintf(buf, size, "%s/%s", home, CONFIG_DIR);
    return n >= 0 && (size_t)n < size;
}

int config_ensure_dir(const char *who)
{
    char dir[PATH_MAX];

    if (!config_path(dir, sizeof dir, NULL)) {
        ui_error(who, "%s", MSG_NO_HOME);
        return -1;
    }
    if (mkdir(dir, 0755) == -1 && errno != EEXIST) {
        ui_error(who, "%s: %s", dir, msg_error(errno));
        return -1;
    }
    return 0;
}
