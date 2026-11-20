/*
 * commands.c - the table of tr-shell commands, dispatch and `yardim`.
 */
#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "builtins.h"
#include "executor.h"
#include "messages.h"
#include "shell.h"
#include "ui.h"

static int bi_yardim(int argc, char **argv);

/* A NULL-terminated argv for an alias target. */
#define RUN(...) ((const char *const[]){ __VA_ARGS__, NULL })

#define BUILTIN(n, f, g, u, d) { n, CMD_BUILTIN, f, NULL, g, u, d }
#define ALIAS(n, t, g, u, d)   { n, CMD_ALIAS, NULL, t, g, u, d }

/* Some tools take different flags on macOS (BSD) and Linux (GNU). */
#if defined(__APPLE__)
#define LS_CMD     RUN("ls", "-G")
#define PS_CMD     RUN("ps", "-Ao", "pid,user,%cpu,%mem,comm", "-r")
#define MEM_CMD    RUN("vm_stat")
#else
#define LS_CMD     RUN("ls", "--color=auto")
#define PS_CMD     RUN("ps", "-eo", "pid,user,%cpu,%mem,comm", "--sort=-%cpu")
#define MEM_CMD    RUN("free", "-h")
#endif

#define G_FILES  "Dosya"
#define G_DIRS   "Dizin"
#define G_SYSTEM "Sistem"
#define G_SHELL  "Kabuk"

static const char *const groups[] = { G_FILES, G_DIRS, G_SYSTEM, G_SHELL };

static const struct command table[] = {
    ALIAS("listele", LS_CMD, G_FILES,
          "listele [DİZİN]", "Dizindeki dosyaları listeler"),
    ALIAS("oku", RUN("cat"), G_FILES,
          "oku DOSYA...", "Dosyanın içeriğini gösterir"),
    ALIAS("kopyala", RUN("cp", "-r"), G_FILES,
          "kopyala KAYNAK HEDEF", "Dosya ya da klasör kopyalar"),
    ALIAS("tasi", RUN("mv"), G_FILES,
          "tasi KAYNAK HEDEF", "Taşır ya da yeniden adlandırır"),
    ALIAS("klasor", RUN("mkdir", "-p"), G_FILES,
          "klasor AD...", "Klasör oluşturur (ara klasörlerle birlikte)"),
    ALIAS("olustur", RUN("touch"), G_FILES,
          "olustur DOSYA...", "Boş dosya oluşturur"),
    ALIAS("bul", RUN("find", ".", "-name"), G_FILES,
          "bul DESEN", "Bu dizinin altında adı desene uyan dosyaları bulur"),
    ALIAS("ara", RUN("grep", "-rn", "--color=auto"), G_FILES,
          "ara KELİME [YER]", "Dosyaların içinde metin arar"),
    ALIAS("boyut", RUN("du", "-sh"), G_FILES,
          "boyut [YER]", "Dosya ya da klasörün boyutunu gösterir"),

    BUILTIN("gec", bi_gec, G_DIRS,
            "gec [DİZİN]", "Dizin değiştirir; dizin verilmezse ev dizinine gider"),
    BUILTIN("geri", bi_geri, G_DIRS,
            "geri", "Bir önceki dizine döner"),
    BUILTIN("ev", bi_ev, G_DIRS,
            "ev", "Ev dizinine gider"),
    BUILTIN("nerdeyim", bi_nerdeyim, G_DIRS,
            "nerdeyim", "Bulunduğun dizini gösterir"),

    ALIAS("surecler", PS_CMD, G_SYSTEM,
          "surecler", "Çalışan süreçleri CPU kullanımına göre listeler"),
    ALIAS("oldur", RUN("kill"), G_SYSTEM,
          "oldur PID...", "Süreci sonlandırır"),
    ALIAS("disk", RUN("df", "-h"), G_SYSTEM,
          "disk", "Disk kullanımını gösterir"),
    ALIAS("bellek", MEM_CMD, G_SYSTEM,
          "bellek", "Bellek kullanımını gösterir"),
    ALIAS("kimim", RUN("whoami"), G_SYSTEM,
          "kimim", "Kullanıcı adını gösterir"),
    BUILTIN("tarih", bi_tarih, G_SYSTEM,
            "tarih", "Tarihi ve saati Türkçe gösterir"),

    BUILTIN("yardim", bi_yardim, G_SHELL,
            "yardim [KOMUT]", "Komutları listeler ya da bir komutu açıklar"),
    BUILTIN("hakkinda", bi_hakkinda, G_SHELL,
            "hakkinda", "Sürüm ve yazar bilgisi"),
    BUILTIN("cikis", bi_exit, G_SHELL,
            "cikis", "Kabuktan çıkar"),
    BUILTIN("cd", bi_cd, G_SHELL,
            "cd DİZİN", "Dizin değiştirir (gec'in klasik hâli)"),
    BUILTIN("exit", bi_exit, G_SHELL,
            "exit", "Kabuktan çıkar (cikis ile aynı)"),
};

#define TABLE_LEN (sizeof table / sizeof table[0])
#define GROUPS_LEN (sizeof groups / sizeof groups[0])

const struct command *commands_find(const char *name)
{
    for (size_t i = 0; i < TABLE_LEN; i++)
        if (strcmp(table[i].name, name) == 0)
            return &table[i];
    return NULL;
}

/* Builds "target... user-args..." and runs it as an external program. */
static int run_alias(const struct command *cmd, int argc, char **argv)
{
    char *expanded[MAX_ARGS];
    int n = 0;

    for (const char *const *t = cmd->target; *t != NULL; t++)
        expanded[n++] = (char *)*t;

    if (n + argc - 1 > MAX_ARGS - 1) {
        ui_error(TRSH_NAME, MSG_TOO_MANY_ARGS, MAX_ARGS - 1);
        return 1;
    }
    for (int i = 1; i < argc; i++)
        expanded[n++] = argv[i];
    expanded[n] = NULL;

    return executor_run(expanded);
}

int commands_run(int argc, char **argv)
{
    const struct command *cmd = commands_find(argv[0]);

    if (cmd == NULL)
        return 0;

    if (cmd->kind == CMD_BUILTIN)
        g_shell.last_status = cmd->fn(argc, argv);
    else
        g_shell.last_status = run_alias(cmd, argc, argv);
    return 1;
}

static void print_target(const char *const *target)
{
    for (const char *const *t = target; *t != NULL; t++)
        printf("%s%s", t == target ? "" : " ", *t);
    putchar('\n');
}

static int help_one(const char *name)
{
    const struct command *cmd = commands_find(name);

    if (cmd == NULL) {
        ui_error("yardim", MSG_UNKNOWN_HELP, name);
        return 1;
    }
    printf("%s%s%s - %s\n", GREEN, cmd->name, RESET, cmd->desc);
    printf("%s %s\n", MSG_HELP_USAGE, cmd->usage);
    if (cmd->kind == CMD_ALIAS) {
        printf("%s ", MSG_HELP_RUNS);
        print_target(cmd->target);
    }
    return 0;
}

static void help_all(void)
{
    printf("%s%s%s\n", BOLD, MSG_HELP_TITLE, RESET);

    for (size_t g = 0; g < GROUPS_LEN; g++) {
        printf("\n%s%s%s\n", BOLD, groups[g], RESET);
        for (size_t i = 0; i < TABLE_LEN; i++) {
            if (strcmp(table[i].group, groups[g]) != 0)
                continue;
            printf("  %s%-10s%s %s\n", GREEN, table[i].name, RESET, table[i].desc);
        }
    }
    printf("\n%s%s\n%s%s\n", GRAY, MSG_HELP_DETAIL, MSG_HELP_LINUX, RESET);
}

static int bi_yardim(int argc, char **argv)
{
    if (argc > 2) {
        ui_error(argv[0], "%s", MSG_EXTRA_ARGS);
        return 1;
    }
    if (argc == 2)
        return help_one(argv[1]);

    help_all();
    return 0;
}
