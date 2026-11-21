/*
 * sysinfo.c - the `sistem` command: a one-screen system summary.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "sysinfo.h"
#include "messages.h"
#include "ui.h"
#include "util.h"

#define BAR_WIDTH   20
#define LABEL_WIDTH 19

/* "Label:" padded to a fixed width so the values line up. */
static void label(const char *name)
{
    printf("  %s%s:%s", BOLD, name, RESET);
    printf("%*s", LABEL_WIDTH - text_width(name), "");
}

/* [██████████░░░░░░░░░░]  52%  - colored red above 90%. */
static void bar(double fraction)
{
    if (fraction < 0)
        fraction = 0;
    if (fraction > 1)
        fraction = 1;
    int filled = (int)(fraction * BAR_WIDTH + 0.5);
    const char *color = fraction > 0.9 ? RED : ACCENT;

    printf("%s", color);
    for (int i = 0; i < BAR_WIDTH; i++)
        fputs(i < filled ? "█" : "░", stdout);
    printf("%s %3d%%", RESET, (int)(fraction * 100 + 0.5));
}

/* Value of "key: value" in a /proc file, as text. Returns 0 if absent. */
static int proc_field(const char *file, const char *key, char *out, size_t size)
{
    char line[512];
    size_t klen = strlen(key);
    FILE *f = fopen(file, "r");
    int found = 0;

    if (f == NULL)
        return 0;
    while (fgets(line, sizeof line, f) != NULL) {
        if (strncmp(line, key, klen) != 0)
            continue;
        char *v = line + klen;
        while (*v == ' ' || *v == '\t' || *v == ':' || *v == '=')
            v++;
        v[strcspn(v, "\n")] = '\0';
        /* Values in os-release are quoted. */
        if (*v == '"') {
            v++;
            v[strcspn(v, "\"")] = '\0';
        }
        snprintf(out, size, "%s", v);
        found = 1;
        break;
    }
    fclose(f);
    return found;
}

/* A /proc/meminfo value in bytes (the file lists kB). */
static long long meminfo_bytes(const char *key)
{
    char v[64];

    if (!proc_field("/proc/meminfo", key, v, sizeof v))
        return -1;
    return atoll(v) * 1024;
}

static void show_os(void)
{
    char name[256];
    struct utsname u;

    if (!proc_field("/etc/os-release", "PRETTY_NAME", name, sizeof name))
        name[0] = '\0';
    if (uname(&u) == 0) {
        if (name[0] == '\0')
            snprintf(name, sizeof name, "%s", u.sysname);
        label(MSG_SYS_OS);
        printf("%s\n", name);
        label(MSG_SYS_KERNEL);
        printf("%s %s (%s)\n", u.sysname, u.release, u.machine);
        label(MSG_SYS_HOST);
        printf("%s\n", u.nodename);
    }
}

static void show_cpu(void)
{
    char model[256];
    long cores = sysconf(_SC_NPROCESSORS_ONLN);

    label(MSG_SYS_CPU);
    if (proc_field("/proc/cpuinfo", "model name", model, sizeof model))
        printf("%s", model);
    else
        printf("?");
    if (cores > 0)
        printf(" %s(" MSG_SYS_CORES ")%s", GRAY, cores, RESET);
    putchar('\n');
}

static void show_usage(const char *name, long long total, long long used)
{
    char u[16], t[16];

    if (total <= 0)
        return;
    fmt_size(u, sizeof u, used);
    fmt_size(t, sizeof t, total);
    label(name);
    bar((double)used / (double)total);
    printf("  %s%s / %s%s\n", GRAY, u, t, RESET);
}

static void show_memory(void)
{
    long long total = meminfo_bytes("MemTotal");
    long long avail = meminfo_bytes("MemAvailable");
    long long swap_total = meminfo_bytes("SwapTotal");
    long long swap_free = meminfo_bytes("SwapFree");

    if (total > 0 && avail >= 0)
        show_usage(MSG_SYS_MEMORY, total, total - avail);
    if (swap_total > 0 && swap_free >= 0)
        show_usage(MSG_SYS_SWAP, swap_total, swap_total - swap_free);
}

static void show_disk(void)
{
    struct statvfs vfs;

    if (statvfs("/", &vfs) != 0)
        return;
    long long total = (long long)vfs.f_blocks * (long long)vfs.f_frsize;
    long long avail = (long long)vfs.f_bavail * (long long)vfs.f_frsize;
    show_usage(MSG_SYS_DISK, total, total - avail);
}

static void show_uptime(void)
{
    double secs;
    FILE *f = fopen("/proc/uptime", "r");

    if (f == NULL)
        return;
    int ok = fscanf(f, "%lf", &secs) == 1;
    fclose(f);
    if (!ok)
        return;

    long s = (long)secs;
    long days = s / 86400, hours = s % 86400 / 3600, mins = s % 3600 / 60;
    label(MSG_SYS_UPTIME);
    if (days > 0)
        printf("%ld %s ", days, MSG_SYS_DAYS);
    if (days > 0 || hours > 0)
        printf("%ld %s ", hours, MSG_SYS_HOURS);
    printf("%ld %s\n", mins, MSG_SYS_MINUTES);
}

static void show_load(void)
{
    double l1, l5, l15;
    int running, total;
    FILE *f = fopen("/proc/loadavg", "r");

    if (f == NULL)
        return;
    int n = fscanf(f, "%lf %lf %lf %d/%d", &l1, &l5, &l15, &running, &total);
    fclose(f);
    if (n < 3)
        return;

    label(MSG_SYS_LOAD);
    printf("%.2f  %.2f  %.2f %s(1 / 5 / 15 dk)%s\n", l1, l5, l15, GRAY, RESET);
    if (n == 5) {
        label(MSG_SYS_PROCS);
        printf("%d\n", total);
    }
}

int bi_sistem(int argc, char **argv)
{
    if (argc > 1) {
        ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
        return 1;
    }

    show_os();
    show_cpu();
    show_memory();
    show_disk();
    show_uptime();
    show_load();
    return 0;
}
