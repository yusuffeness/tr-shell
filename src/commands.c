/*
 * commands.c - the table of tr-shell commands, dispatch, `yardim` and
 * `zamanla`.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>

#include "commands.h"
#include "alias.h"
#include "builtins.h"
#include "config.h"
#include "executor.h"
#include "calc.h"
#include "files.h"
#include "history.h"
#include "jobs.h"
#include "suggest.h"
#include "notes.h"
#include "sysinfo.h"
#include "trash.h"
#include "messages.h"
#include "shell.h"
#include "theme.h"
#include "ui.h"

#define EXIT_NOT_FOUND  127   /* same as bash: command not found */
#define EXIT_CANNOT_RUN 126   /* same as bash: found but not executable */

static int bi_yardim(int argc, char **argv);
static int bi_zamanla(int argc, char **argv);

/* A NULL-terminated argv for an alias target. */
#define RUN(...) ((const char *const[]){ __VA_ARGS__, NULL })

#define BUILTIN(n, f, g, u, d) { n, CMD_BUILTIN, f, NULL, g, u, d }
#define ALIAS(n, t, g, u, d)   { n, CMD_ALIAS, NULL, t, g, u, d }

/* Some tools take different flags on macOS (BSD) and Linux (GNU). */
#if defined(__APPLE__)
#define PS_CMD     RUN("ps", "-Ao", "pid,user,%cpu,%mem,comm", "-r")
#define MEM_CMD    RUN("vm_stat")
#else
#define PS_CMD     RUN("ps", "-eo", "pid,user,%cpu,%mem,comm", "--sort=-%cpu")
#define MEM_CMD    RUN("free", "-h")
#endif

#define G_FILES  "Dosya"
#define G_DIRS   "Dizin"
#define G_SYSTEM "Sistem"
#define G_TOOLS  "Araçlar"
#define G_SHELL  "Kabuk"

static const char *const groups[] = { G_FILES, G_DIRS, G_SYSTEM, G_TOOLS, G_SHELL };

static const struct command_entry table[] = {
    BUILTIN("listele", bi_listele, G_FILES,
            "listele [-a] [-d] [YOL...]", "Dosyaları listeler; -a gizliler, -d ayrıntılı"),
    BUILTIN("agac", bi_agac, G_FILES,
            "agac [-a] [YOL] [DERİNLİK]", "Klasör yapısını ağaç olarak çizer"),
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
          "bul \"DESEN\"", "Adı desene uyan dosyaları bulur (ör. bul \"*.c\")"),
    ALIAS("ara", RUN("grep", "-rn", "--color=auto"), G_FILES,
          "ara KELİME [YER]", "Dosyaların içinde metin arar"),
    ALIAS("boyut", RUN("du", "-sh"), G_FILES,
          "boyut [YER]", "Dosya ya da klasörün boyutunu gösterir"),
    BUILTIN("sil", bi_sil, G_FILES,
            "sil [-e] YOL...", "Onay sorar ve çöp kutusuna taşır; -e sormaz"),
    BUILTIN("copkutusu", bi_copkutusu, G_FILES,
            "copkutusu [geri NO|AD | bosalt]", "Çöp kutusunu listeler, geri alır, boşaltır"),

    BUILTIN("gec", bi_gec, G_DIRS,
            "gec [DİZİN]", "Dizin değiştirir; dizin verilmezse ev dizinine gider"),
    BUILTIN("geri", bi_geri, G_DIRS,
            "geri", "Bir önceki dizine döner"),
    BUILTIN("ev", bi_ev, G_DIRS,
            "ev", "Ev dizinine gider"),
    BUILTIN("nerdeyim", bi_nerdeyim, G_DIRS,
            "nerdeyim", "Bulunduğun dizini gösterir"),

    BUILTIN("sistem", bi_sistem, G_SYSTEM,
            "sistem", "İşletim sistemi, işlemci, bellek, disk ve açık kalma süresi"),
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
    BUILTIN("zamanla", bi_zamanla, G_SYSTEM,
            "zamanla KOMUT [ARGÜMAN...]", "Komutu çalıştırır ve ne kadar sürdüğünü gösterir"),

    BUILTIN("hesapla", bi_hesapla, G_TOOLS,
            "hesapla İFADE", "Hesap makinesi: + - * / % ^ ve parantez"),
    BUILTIN("not", bi_not, G_TOOLS,
            "not METİN...", "Hızlı not alır"),
    BUILTIN("notlar", bi_notlar, G_TOOLS,
            "notlar [sil NO]", "Notları listeler ya da birini siler"),

    BUILTIN("yardim", bi_yardim, G_SHELL,
            "yardim [KOMUT]", "Komutları listeler ya da bir komutu açıklar"),
    BUILTIN("isler", bi_isler, G_SHELL,
            "isler", "Arka plandaki ve durdurulmuş işleri listeler"),
    BUILTIN("on", bi_on, G_SHELL,
            "on [NO]", "İşi ön plana alır (Ctrl+Z ile durdurulanı da)"),
    BUILTIN("arka", bi_arka, G_SHELL,
            "arka [NO]", "Durdurulmuş işi arka planda sürdürür"),
    BUILTIN("degisken", bi_degisken, G_SHELL,
            "degisken [AD=DEĞER... | sil AD]", "Ortam değişkenlerini listeler, atar ya da siler"),
    BUILTIN("takmaad", bi_takmaad, G_SHELL,
            "takmaad [AD=KOMUT]", "Kendi komutunu tanımlar ya da takma adları listeler"),
    BUILTIN("takmaadsil", bi_takmaadsil, G_SHELL,
            "takmaadsil AD...", "Takma adı siler"),
    BUILTIN("ayar", bi_ayar, G_SHELL,
            "ayar [ANAHTAR DEĞER | duzenle | sifirla]", "Ayarları gösterir ya da değiştirir"),
    BUILTIN("gecmis", bi_gecmis, G_SHELL,
            "gecmis [N]", "Önceki komutları listeler; !N ile tekrar çalıştırılır"),
    BUILTIN("tema", bi_tema, G_SHELL,
            "tema [AD]", "Temaları listeler ya da temayı değiştirir"),
    BUILTIN("temizle", bi_temizle, G_SHELL,
            "temizle", "Ekranı temizler"),
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

const struct command_entry *commands_find(const char *name)
{
    for (size_t i = 0; i < TABLE_LEN; i++)
        if (strcmp(table[i].name, name) == 0)
            return &table[i];
    return NULL;
}

int commands_exec(int argc, char **argv)
{
    return exec_argv(argc, argv);
}

void commands_exec_child(int argc, char **argv)
{
    const struct command_entry *cmd = commands_find(argv[0]);
    char **run = argv;

    if (cmd != NULL && cmd->kind == CMD_BUILTIN) {
        int status = cmd->fn(argc, argv);
        fflush(stdout);
        fflush(stderr);
        _exit(status);
    }

    /* An alias runs its target followed by the user's arguments. */
    if (cmd != NULL && cmd->kind == CMD_ALIAS) {
        size_t nt = 0;
        while (cmd->target[nt] != NULL)
            nt++;
        run = malloc((nt + (size_t)argc) * sizeof *run);
        if (run == NULL)
            _exit(1);
        for (size_t i = 0; i < nt; i++)
            run[i] = (char *)cmd->target[i];
        for (int i = 1; i < argc; i++)
            run[nt + (size_t)i - 1] = argv[i];
        run[nt + (size_t)argc - 1] = NULL;
    }

    execvp(run[0], run);

    /* execvp only returns on failure. */
    int err = errno;
    if (err == ENOENT) {
        const char *guess = cmd == NULL ? suggest_command(argv[0]) : NULL;
        if (guess != NULL)
            ui_error(run[0], "%s. " MSG_DID_YOU_MEAN, MSG_NOT_FOUND, guess);
        else
            ui_error(run[0], "%s", MSG_NOT_FOUND);
    } else {
        ui_error(run[0], "%s", msg_error(err));
    }
    _exit(err == ENOENT ? EXIT_NOT_FOUND : EXIT_CANNOT_RUN);
}

size_t commands_count(void)
{
    return TABLE_LEN;
}

const struct command_entry *commands_at(size_t i)
{
    return i < TABLE_LEN ? &table[i] : NULL;
}

/* Formats seconds Turkish style: 1,23 */
static void print_seconds(const char *label, double s)
{
    long cs = (long)(s * 100 + 0.5);
    fprintf(stderr, "%s %ld,%02ld %s", label, cs / 100, cs % 100, MSG_SECONDS);
}

static double tv_seconds(const struct timeval *tv)
{
    return (double)tv->tv_sec + (double)tv->tv_usec / 1e6;
}

/*
 * `zamanla KOMUT ...`: runs the command and reports wall-clock, user and
 * system time on stderr, like `time` in bash. CPU times come from
 * getrusage(RUSAGE_CHILDREN), so they cover external programs only.
 */
static int bi_zamanla(int argc, char **argv)
{
    struct timespec t0, t1;
    struct rusage r0, r1;

    if (argc < 2) {
        ui_error(argv[0], "%s %s", MSG_HELP_USAGE, "zamanla KOMUT [ARGÜMAN...]");
        return 1;
    }

    getrusage(RUSAGE_CHILDREN, &r0);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int status = commands_exec(argc - 1, argv + 1);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    getrusage(RUSAGE_CHILDREN, &r1);

    double real = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
    double user = tv_seconds(&r1.ru_utime) - tv_seconds(&r0.ru_utime);
    double sys = tv_seconds(&r1.ru_stime) - tv_seconds(&r0.ru_stime);

    fflush(stdout);
    fputs(GRAY, stderr);
    print_seconds(MSG_TIME_REAL, real);
    fputs(" · ", stderr);
    print_seconds(MSG_TIME_USER, user);
    fputs(" · ", stderr);
    print_seconds(MSG_TIME_SYS, sys);
    fprintf(stderr, "%s\n", RESET);
    return status;
}

static void print_target(const char *const *target)
{
    for (const char *const *t = target; *t != NULL; t++)
        printf("%s%s", t == target ? "" : " ", *t);
    putchar('\n');
}

static int help_one(const char *name)
{
    const struct command_entry *cmd = commands_find(name);

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
