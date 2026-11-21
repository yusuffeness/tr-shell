/*
 * config.c - settings and aliases, kept in ~/.trsh/config.
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "executor.h"
#include "messages.h"
#include "theme.h"
#include "ui.h"
#include "util.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define CONFIG_DIR      ".trsh"
#define CONFIG_FILE     "config"
#define OLD_THEME_FILE  "theme"     /* before v0.7 the theme had its own file */
#define ALIAS_SECTION   "[takmaad]"

#define HISTORY_DEFAULT 1000
#define HISTORY_MIN     10
#define HISTORY_MAX_CFG 100000

struct settings g_settings;

/* ----------------------------------------------------------------- paths */

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

/* -------------------------------------------------------------- values */

static const char *const banner_names[] = { "acik", "sade", "kapali" };

/* The user's full name from the system, or the login name. */
static void default_name(char *buf, size_t size)
{
    struct passwd *pw = getpwuid(getuid());

    if (pw != NULL && pw->pw_gecos != NULL && pw->pw_gecos[0] != '\0') {
        size_t n = strcspn(pw->pw_gecos, ",");
        if (n > 0) {
            snprintf(buf, size, "%.*s", (int)n, pw->pw_gecos);
            return;
        }
    }
    const char *user = pw != NULL ? pw->pw_name : getenv("USER");
    snprintf(buf, size, "%s", user != NULL ? user : "kullanıcı");
}

static int parse_int(const char *s, int *out)
{
    char *end;
    long v;

    if (*s == '\0')
        return -1;
    v = strtol(s, &end, 10);
    if (*end != '\0' || v < INT_MIN || v > INT_MAX)
        return -1;
    *out = (int)v;
    return 0;
}

/*
 * Applies one setting. Returns 0, or -1 with *why pointing at a message.
 * Shared by the config file and the `ayar` command.
 */
static int apply_setting(const char *key, const char *value, const char **why)
{
    if (strcmp(key, "tema") == 0) {
        if (theme_set(value) != 0) {
            *why = MSG_CFG_BAD_THEME;
            return -1;
        }
        return 0;
    }
    if (strcmp(key, "isim") == 0) {
        if (*value == '\0' || strlen(value) >= sizeof g_settings.name) {
            *why = MSG_CFG_BAD_NAME;
            return -1;
        }
        snprintf(g_settings.name, sizeof g_settings.name, "%s", value);
        return 0;
    }
    if (strcmp(key, "karsilama") == 0) {
        for (size_t i = 0; i < sizeof banner_names / sizeof banner_names[0]; i++) {
            if (strcmp(value, banner_names[i]) == 0) {
                g_settings.banner = (enum banner_mode)i;
                return 0;
            }
        }
        *why = MSG_CFG_BAD_BANNER;
        return -1;
    }
    if (strcmp(key, "gecmis_boyutu") == 0) {
        int n;
        if (parse_int(value, &n) != 0 || n < HISTORY_MIN || n > HISTORY_MAX_CFG) {
            *why = MSG_CFG_BAD_HISTORY;
            return -1;
        }
        g_settings.history_size = n;
        return 0;
    }
    *why = MSG_CFG_UNKNOWN_KEY;
    return -1;
}

/* ----------------------------------------------------------- the file */

/* Strips leading and trailing white space in place. */
static char *trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        *--end = '\0';
    return s;
}

static void warn(const char *path, int line, const char *why, const char *what)
{
    ui_error(TRSH_NAME, MSG_CFG_LINE, path, line, why, what);
}

static void reset_defaults(void)
{
    strvec_free(&g_settings.alias_names);
    strvec_free(&g_settings.alias_values);
    default_name(g_settings.name, sizeof g_settings.name);
    g_settings.banner = BANNER_FULL;
    g_settings.history_size = HISTORY_DEFAULT;
    theme_set("mavi");
}

/* Before v0.7 the theme was kept in ~/.trsh/theme; honor it once. */
static void load_old_theme(void)
{
    char path[PATH_MAX], name[32];

    if (!config_path(path, sizeof path, OLD_THEME_FILE))
        return;
    FILE *f = fopen(path, "r");
    if (f == NULL)
        return;
    if (fgets(name, sizeof name, f) != NULL) {
        name[strcspn(name, "\r\n")] = '\0';
        theme_set(name);
    }
    fclose(f);
}

void config_load(void)
{
    char path[PATH_MAX];
    char buf[MAX_LINE];
    int lineno = 0, in_aliases = 0;
    const char *why;

    reset_defaults();
    if (!config_path(path, sizeof path, CONFIG_FILE))
        return;

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        load_old_theme();
        return;
    }

    while (fgets(buf, sizeof buf, f) != NULL) {
        lineno++;
        char *line = trim(buf);
        if (*line == '\0' || *line == '#')
            continue;
        if (*line == '[') {
            in_aliases = strcmp(line, ALIAS_SECTION) == 0;
            if (!in_aliases)
                warn(path, lineno, MSG_CFG_UNKNOWN_SECTION, line);
            continue;
        }

        char *eq = strchr(line, '=');
        if (eq == NULL) {
            warn(path, lineno, MSG_CFG_NO_EQUALS, line);
            continue;
        }
        *eq = '\0';
        char *key = trim(line);
        char *value = trim(eq + 1);

        if (in_aliases) {
            if (alias_name_ok(key) && *value != '\0')
                alias_define(key, value);
            else
                warn(path, lineno, MSG_CFG_BAD_ALIAS, key);
        } else if (apply_setting(key, value, &why) != 0) {
            warn(path, lineno, why, key);
        }
    }
    fclose(f);
}

int config_save(const char *who)
{
    char path[PATH_MAX], tmp[PATH_MAX + 8];

    if (config_ensure_dir(who) != 0 || !config_path(path, sizeof path, CONFIG_FILE))
        return 1;
    snprintf(tmp, sizeof tmp, "%s.yeni", path);

    /* Write a new file next to the old one, then swap them. */
    FILE *f = fopen(tmp, "w");
    if (f == NULL) {
        ui_error(who, "%s: %s", tmp, msg_error(errno));
        return 1;
    }
    fprintf(f, "%s", MSG_CFG_HEADER);
    fprintf(f, "\n%s\ntema = %s\n", MSG_CFG_DOC_THEME, theme_current()->name);
    fprintf(f, "\n%s\nisim = %s\n", MSG_CFG_DOC_NAME, g_settings.name);
    fprintf(f, "\n%s\nkarsilama = %s\n", MSG_CFG_DOC_BANNER,
            banner_names[g_settings.banner]);
    fprintf(f, "\n%s\ngecmis_boyutu = %d\n", MSG_CFG_DOC_HISTORY,
            g_settings.history_size);
    fprintf(f, "\n%s\n%s\n", ALIAS_SECTION, MSG_CFG_DOC_ALIAS);
    for (size_t i = 0; i < g_settings.alias_names.n; i++)
        fprintf(f, "%s = %s\n", g_settings.alias_names.v[i], g_settings.alias_values.v[i]);

    if (fclose(f) != 0 || rename(tmp, path) != 0) {
        ui_error(who, "%s: %s", path, msg_error(errno));
        unlink(tmp);
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------- aliases */

int alias_name_ok(const char *name)
{
    if (*name == '\0' || strcmp(name, "takmaad") == 0 || strcmp(name, "takmaadsil") == 0)
        return 0;
    for (const char *c = name; *c; c++)
        if (!isalnum((unsigned char)*c) && *c != '_' && *c != '-' && *c != '.')
            return 0;
    return 1;
}

static long alias_index(const char *name)
{
    for (size_t i = 0; i < g_settings.alias_names.n; i++)
        if (strcmp(g_settings.alias_names.v[i], name) == 0)
            return (long)i;
    return -1;
}

const char *alias_get(const char *name)
{
    long i = alias_index(name);
    return i < 0 ? NULL : g_settings.alias_values.v[i];
}

int alias_define(const char *name, const char *value)
{
    long i = alias_index(name);

    if (i >= 0) {
        char *copy = strdup(value);
        if (copy == NULL)
            return -1;
        free(g_settings.alias_values.v[i]);
        g_settings.alias_values.v[i] = copy;
        return 0;
    }
    if (strvec_push(&g_settings.alias_names, name) != 0)
        return -1;
    if (strvec_push(&g_settings.alias_values, value) != 0) {
        /* keep the two lists the same length */
        free(g_settings.alias_names.v[--g_settings.alias_names.n]);
        g_settings.alias_names.v[g_settings.alias_names.n] = NULL;
        return -1;
    }
    return 0;
}

int alias_remove(const char *name)
{
    long i = alias_index(name);
    struct strvec *n = &g_settings.alias_names, *v = &g_settings.alias_values;

    if (i < 0)
        return -1;
    free(n->v[i]);
    free(v->v[i]);
    memmove(&n->v[i], &n->v[i + 1], (n->n - (size_t)i) * sizeof n->v[0]);
    memmove(&v->v[i], &v->v[i + 1], (v->n - (size_t)i) * sizeof v->v[0]);
    n->n--;
    v->n--;
    return 0;
}

/* ---------------------------------------------------------------- ayar */

static void show_settings(void)
{
    char path[PATH_MAX];

    printf("  %s%-14s%s %s\n", BOLD, "tema", RESET, theme_current()->name);
    printf("  %s%-14s%s %s\n", BOLD, "isim", RESET, g_settings.name);
    printf("  %s%-14s%s %s\n", BOLD, "karsilama", RESET, banner_names[g_settings.banner]);
    printf("  %s%-14s%s %d\n", BOLD, "gecmis_boyutu", RESET, g_settings.history_size);
    printf("  %s%-14s%s %zu\n", BOLD, "takma adlar", RESET, g_settings.alias_names.n);
    if (config_path(path, sizeof path, CONFIG_FILE))
        printf("\n%s" MSG_CFG_FILE_IS "%s\n", GRAY, path, RESET);
}

/* `ayar tema` shows just that setting. */
static int show_one(const char *who, const char *key)
{
    if (strcmp(key, "tema") == 0)
        printf("%s\n", theme_current()->name);
    else if (strcmp(key, "isim") == 0)
        printf("%s\n", g_settings.name);
    else if (strcmp(key, "karsilama") == 0)
        printf("%s\n", banner_names[g_settings.banner]);
    else if (strcmp(key, "gecmis_boyutu") == 0)
        printf("%d\n", g_settings.history_size);
    else {
        ui_error(who, "%s: %s", key, MSG_CFG_UNKNOWN_KEY);
        return 1;
    }
    return 0;
}

static int edit_file(const char *who)
{
    char path[PATH_MAX];
    struct stat st;
    const char *editor = getenv("VISUAL");

    if (editor == NULL || *editor == '\0')
        editor = getenv("EDITOR");
    if (editor == NULL || *editor == '\0')
        editor = "nano";

    if (!config_path(path, sizeof path, CONFIG_FILE)) {
        ui_error(who, "%s", MSG_NO_HOME);
        return 1;
    }
    /* Create the file with its comments first, so there is something to edit. */
    if (stat(path, &st) != 0 && config_save(who) != 0)
        return 1;

    char *argv[] = { (char *)editor, path, NULL };
    int status = executor_run(argv);
    config_load();      /* pick up what was changed */
    return status;
}

int bi_ayar(int argc, char **argv)
{
    char value[MAX_LINE];
    const char *why;

    if (argc == 1) {
        show_settings();
        return 0;
    }
    if (strcmp(argv[1], "duzenle") == 0 && argc == 2)
        return edit_file(argv[0]);
    if (strcmp(argv[1], "sifirla") == 0 && argc == 2) {
        if (!ask_yes_no(MSG_CFG_ASK_RESET)) {
            printf("%s\n", MSG_CANCELLED);
            return 1;
        }
        reset_defaults();
        if (config_save(argv[0]) != 0)
            return 1;
        printf("%s\n", MSG_CFG_RESET_DONE);
        return 0;
    }
    if (argc == 2)
        return show_one(argv[0], argv[1]);
    if (argc < 3) {
        ui_error(argv[0], "%s %s", MSG_HELP_USAGE,
                 "ayar [ANAHTAR DEĞER | duzenle | sifirla]");
        return 1;
    }

    /* The value may be several words: ayar isim Yusuf Enes Kütük */
    if (!join_args(value, sizeof value, argc - 2, argv + 2)) {
        ui_error(argv[0], MSG_LINE_TOO_LONG, MAX_LINE - 2);
        return 1;
    }
    if (apply_setting(argv[1], value, &why) != 0) {
        ui_error(argv[0], "%s: %s", argv[1], why);
        return 1;
    }
    if (config_save(argv[0]) != 0)
        return 1;
    printf(MSG_CFG_SET "\n", argv[1], value);
    return 0;
}
