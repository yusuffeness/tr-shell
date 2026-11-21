/*
 * trash.c - `sil` and `copkutusu`: deleting that can be undone.
 *
 * Layout:
 *   ~/.trsh/cop/<id>_<name>   the moved file or folder
 *   ~/.trsh/cop.log           one line per item: id, stored name,
 *                             original path, time (tab separated)
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "trash.h"
#include "config.h"
#include "executor.h"
#include "messages.h"
#include "ui.h"
#include "util.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define TRASH_DIR "cop"
#define TRASH_LOG "cop.log"

struct trash_item {
    long id;
    char stored[NAME_MAX + 32];
    char orig[PATH_MAX];
    long when;
};

struct trash_items {
    struct trash_item *v;
    size_t len;
};

/* ------------------------------------------------------------- the log */

static void items_free(struct trash_items *it)
{
    free(it->v);
    it->v = NULL;
    it->len = 0;
}

static int load(struct trash_items *it)
{
    char path[PATH_MAX];
    char line[PATH_MAX + NAME_MAX + 128];
    size_t cap = 0;

    it->v = NULL;
    it->len = 0;
    if (!config_path(path, sizeof path, TRASH_LOG))
        return -1;

    FILE *f = fopen(path, "r");
    if (f == NULL)
        return errno == ENOENT ? 0 : -1;

    while (fgets(line, sizeof line, f) != NULL) {
        struct trash_item x;
        char *id = strtok(line, "\t\n");
        char *stored = strtok(NULL, "\t\n");
        char *orig = strtok(NULL, "\t\n");
        char *when = strtok(NULL, "\t\n");

        if (!id || !stored || !orig || !when)
            continue;
        x.id = atol(id);
        x.when = atol(when);
        snprintf(x.stored, sizeof x.stored, "%s", stored);
        snprintf(x.orig, sizeof x.orig, "%s", orig);

        if (it->len == cap) {
            cap = cap ? cap * 2 : 16;
            struct trash_item *grown = realloc(it->v, cap * sizeof *grown);
            if (grown == NULL)
                break;
            it->v = grown;
        }
        it->v[it->len++] = x;
    }
    fclose(f);
    return 0;
}

static int save(const struct trash_items *it)
{
    char path[PATH_MAX];

    if (!config_path(path, sizeof path, TRASH_LOG))
        return -1;
    FILE *f = fopen(path, "w");
    if (f == NULL)
        return -1;
    for (size_t i = 0; i < it->len; i++)
        fprintf(f, "%ld\t%s\t%s\t%ld\n", it->v[i].id, it->v[i].stored,
                it->v[i].orig, it->v[i].when);
    return fclose(f);
}

static int append(const struct trash_item *x)
{
    char path[PATH_MAX];

    if (!config_path(path, sizeof path, TRASH_LOG))
        return -1;
    FILE *f = fopen(path, "a");
    if (f == NULL)
        return -1;
    fprintf(f, "%ld\t%s\t%s\t%ld\n", x->id, x->stored, x->orig, x->when);
    return fclose(f);
}

/* ---------------------------------------------------------------- paths */

static int trash_dir(char *buf, size_t size, const char *who)
{
    if (config_ensure_dir(who) != 0 || !config_path(buf, size, TRASH_DIR))
        return -1;
    if (mkdir(buf, 0700) == -1 && errno != EEXIST) {
        ui_error(who, "%s: %s", buf, msg_error(errno));
        return -1;
    }
    return 0;
}

/* The name shown to the user: the stored name without "<id>_". */
static const char *display_name(const struct trash_item *x)
{
    const char *u = strchr(x->stored, '_');
    return u ? u + 1 : x->stored;
}

/* Absolute path of p without resolving the last component (it may be a link). */
static int absolute(const char *p, char *out, size_t size)
{
    char cwd[PATH_MAX];

    if (p[0] == '/')
        return snprintf(out, size, "%s", p) < (int)size;
    if (getcwd(cwd, sizeof cwd) == NULL)
        return 0;
    int n = snprintf(out, size, "%s/%s", strcmp(cwd, "/") ? cwd : "", p);
    return n >= 0 && (size_t)n < size;
}

static void strip_trailing_slashes(char *p)
{
    size_t n = strlen(p);
    while (n > 1 && p[n - 1] == '/')
        p[--n] = '\0';
}

static const char *base_name(const char *p)
{
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

/*
 * rename() cannot move across file systems (in WSL, /mnt/c and the Linux
 * home are different ones), so fall back to `mv`, which copies.
 */
static int move(const char *from, const char *to, const char *who)
{
    if (rename(from, to) == 0)
        return 0;
    if (errno != EXDEV) {
        ui_error(who, "%s: %s", from, msg_error(errno));
        return -1;
    }
    char *argv[] = { "mv", "--", (char *)from, (char *)to, NULL };
    return executor_run(argv) == 0 ? 0 : -1;
}

/* ------------------------------------------------------------------- sil */

static long next_id(void)
{
    struct trash_items it;
    long max = 0;

    if (load(&it) == 0)
        for (size_t i = 0; i < it.len; i++)
            if (it.v[i].id > max)
                max = it.v[i].id;
    items_free(&it);
    return max + 1;
}

static int trash_one(const char *arg, int ask, const char *dir)
{
    char path[PATH_MAX], question[PATH_MAX + 64], dest[PATH_MAX + NAME_MAX + 64];
    struct trash_item x;
    struct stat st;

    snprintf(path, sizeof path, "%s", arg);
    strip_trailing_slashes(path);
    const char *name = base_name(path);

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || strcmp(path, "/") == 0) {
        ui_error("sil", MSG_TRASH_REFUSE, arg);
        return 1;
    }
    if (lstat(path, &st) != 0) {
        ui_error("sil", "%s: %s", arg, msg_error(errno));
        return 1;
    }
    if (!absolute(path, x.orig, sizeof x.orig)) {
        ui_error("sil", "%s: %s", arg, msg_error(ENAMETOOLONG));
        return 1;
    }
    if (strncmp(x.orig, dir, strlen(dir)) == 0) {
        ui_error("sil", MSG_TRASH_ALREADY, arg);
        return 1;
    }

    if (ask) {
        snprintf(question, sizeof question, MSG_TRASH_ASK, arg);
        if (!ask_yes_no(question)) {
            printf("%s\n", MSG_CANCELLED);
            return 1;
        }
    }

    x.id = next_id();
    x.when = (long)time(NULL);
    snprintf(x.stored, sizeof x.stored, "%ld_%s", x.id, name);
    snprintf(dest, sizeof dest, "%s/%s", dir, x.stored);

    if (move(path, dest, "sil") != 0)
        return 1;
    if (append(&x) != 0)
        ui_error("sil", "%s", MSG_TRASH_LOG_FAIL);
    printf(MSG_TRASH_MOVED "\n", arg);
    return 0;
}

int bi_sil(int argc, char **argv)
{
    char dir[PATH_MAX];
    int ask = 1, status = 0, paths = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0)
            ask = 0;
        else
            paths++;
    }
    if (paths == 0) {
        ui_error(argv[0], "%s %s", MSG_HELP_USAGE, "sil [-e] YOL...");
        return 1;
    }
    if (trash_dir(dir, sizeof dir, argv[0]) != 0)
        return 1;

    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "-e") != 0)
            status |= trash_one(argv[i], ask, dir);
    return status;
}

/* ------------------------------------------------------------- copkutusu */

static int trash_list(void)
{
    struct trash_items it;
    char date[32];

    if (load(&it) != 0) {
        ui_error("copkutusu", "%s", MSG_TRASH_LOG_FAIL);
        return 1;
    }
    if (it.len == 0) {
        printf("%s\n", MSG_TRASH_EMPTY);
        items_free(&it);
        return 0;
    }
    for (size_t i = 0; i < it.len; i++) {
        const struct trash_item *x = &it.v[i];
        fmt_short_date(date, sizeof date, (time_t)x->when);
        printf("  %s%3ld%s  %s%s%s  %-20s %s← %s%s\n", GRAY, x->id, RESET,
               GRAY, date, RESET, display_name(x), GRAY, x->orig, RESET);
    }
    printf("\n%s" MSG_TRASH_HINT "%s\n", GRAY, RESET);
    items_free(&it);
    return 0;
}

static int is_number(const char *s)
{
    if (*s == '\0')
        return 0;
    for (; *s; s++)
        if (!isdigit((unsigned char)*s))
            return 0;
    return 1;
}

/* Finds an item by id, or the most recent one with that name. */
static long find_item(const struct trash_items *it, const char *key)
{
    if (is_number(key)) {
        long id = atol(key);
        for (size_t i = 0; i < it->len; i++)
            if (it->v[i].id == id)
                return (long)i;
        return -1;
    }
    for (size_t i = it->len; i-- > 0;)
        if (strcmp(display_name(&it->v[i]), key) == 0)
            return (long)i;
    return -1;
}

static int trash_restore(const char *key)
{
    struct trash_items it;
    char dir[PATH_MAX], from[PATH_MAX + NAME_MAX + 64];
    struct stat st;

    if (trash_dir(dir, sizeof dir, "copkutusu") != 0 || load(&it) != 0)
        return 1;

    long i = find_item(&it, key);
    if (i < 0) {
        ui_error("copkutusu", MSG_TRASH_NO_ITEM, key);
        items_free(&it);
        return 1;
    }
    struct trash_item *x = &it.v[i];
    if (lstat(x->orig, &st) == 0) {
        ui_error("copkutusu", "%s: %s", x->orig, msg_error(EEXIST));
        items_free(&it);
        return 1;
    }
    snprintf(from, sizeof from, "%s/%s", dir, x->stored);
    if (move(from, x->orig, "copkutusu") != 0) {
        items_free(&it);
        return 1;
    }
    printf(MSG_TRASH_RESTORED "\n", x->orig);

    /* Drop the restored item from the log. */
    memmove(x, x + 1, (it.len - (size_t)i - 1) * sizeof *x);
    it.len--;
    int status = save(&it) == 0 ? 0 : 1;
    items_free(&it);
    return status;
}

static int trash_empty(int ask)
{
    struct trash_items it;
    char dir[PATH_MAX], question[128], path[PATH_MAX + NAME_MAX + 64];
    int failed = 0;

    if (trash_dir(dir, sizeof dir, "copkutusu") != 0 || load(&it) != 0)
        return 1;
    if (it.len == 0) {
        printf("%s\n", MSG_TRASH_EMPTY);
        items_free(&it);
        return 0;
    }

    snprintf(question, sizeof question, MSG_TRASH_ASK_EMPTY, it.len);
    if (ask && !ask_yes_no(question)) {
        printf("%s\n", MSG_CANCELLED);
        items_free(&it);
        return 1;
    }

    /* Keep in the log only what could not be removed. */
    size_t kept = 0;
    for (size_t i = 0; i < it.len; i++) {
        snprintf(path, sizeof path, "%s/%s", dir, it.v[i].stored);
        char *argv[] = { "rm", "-rf", "--", path, NULL };
        if (executor_run(argv) == 0) {
            continue;
        }
        it.v[kept++] = it.v[i];
        failed = 1;
    }
    size_t removed = it.len - kept;
    it.len = kept;
    save(&it);
    printf(MSG_TRASH_EMPTIED "\n", removed);
    items_free(&it);
    return failed;
}

int bi_copkutusu(int argc, char **argv)
{
    if (argc == 1)
        return trash_list();

    if (strcmp(argv[1], "geri") == 0) {
        if (argc != 3) {
            ui_error(argv[0], "%s %s", MSG_HELP_USAGE, "copkutusu geri NO|AD");
            return 1;
        }
        return trash_restore(argv[2]);
    }

    if (strcmp(argv[1], "bosalt") == 0) {
        int ask = !(argc == 3 && strcmp(argv[2], "-e") == 0);
        if (argc > 3 || (argc == 3 && ask)) {
            ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
            return 1;
        }
        return trash_empty(ask);
    }

    ui_error(argv[0], MSG_UNKNOWN_SUBCMD, argv[1]);
    return 1;
}
