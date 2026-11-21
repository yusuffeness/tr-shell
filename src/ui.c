/*
 * ui.c - screen handling, colors, banner, prompt and error output.
 */
#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "ui.h"
#include "git.h"
#include "messages.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define ALT_SCREEN_ON  "\033[?1049h\033[H"
#define ALT_SCREEN_OFF "\033[?1049l"
#define CLEAR_SCREEN   "\033[H\033[2J\033[3J"

#define BOX_INNER   64          /* columns between the two │ of the banner */
#define LOGO_WIDTH  33
#define LOGO_ROWS   6
#define INFO_WIDTH  (BOX_INNER - 3 - LOGO_WIDTH - 3 - 1)

static volatile sig_atomic_t alt_screen;

/* ---------------------------------------------------------------- helpers */

/*
 * Display width of a UTF-8 string, counting one column per code point.
 * Good enough for our own text (Turkish letters, box drawing); it does not
 * try to handle double-width characters such as CJK or emoji.
 */
static int text_width(const char *s)
{
    int n = 0;

    for (; *s; s++)
        if (((unsigned char)*s & 0xC0) != 0x80)
            n++;
    return n;
}

/* Returns the last max_cols columns of s (a pointer into s). */
static const char *text_tail(const char *s, int max_cols)
{
    int skip = text_width(s) - max_cols;

    while (skip > 0 && *s) {
        s++;
        while (((unsigned char)*s & 0xC0) == 0x80)
            s++;
        skip--;
    }
    return s;
}

static void spaces(int n)
{
    while (n-- > 0)
        putchar(' ');
}

static void repeat(const char *s, int n)
{
    while (n-- > 0)
        fputs(s, stdout);
}

static int term_cols(void)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;

    const char *env = getenv("COLUMNS");
    int cols = env ? atoi(env) : 0;
    return cols > 0 ? cols : 80;
}

/* Current directory with the home directory shortened to ~. Static buffer. */
static const char *shown_cwd(void)
{
    static char cwd[PATH_MAX];
    static char buf[PATH_MAX + 1];
    const char *home = getenv("HOME");

    if (getcwd(cwd, sizeof cwd) == NULL)
        return "?";

    size_t hl = home ? strlen(home) : 0;
    if (hl > 1 && strncmp(cwd, home, hl) == 0 &&
        (cwd[hl] == '/' || cwd[hl] == '\0')) {
        snprintf(buf, sizeof buf, "~%s", cwd + hl);
        return buf;
    }
    return cwd;
}

void ui_format_date(char *buf, size_t size, int with_weekday)
{
    static const char *const days[] = {
        "Pazar", "Pazartesi", "Salı", "Çarşamba", "Perşembe", "Cuma", "Cumartesi",
    };
    static const char *const months[] = {
        "Ocak", "Şubat", "Mart", "Nisan", "Mayıs", "Haziran",
        "Temmuz", "Ağustos", "Eylül", "Ekim", "Kasım", "Aralık",
    };
    time_t now = time(NULL);
    struct tm tm;

    if (localtime_r(&now, &tm) == NULL) {
        snprintf(buf, size, "?");
        return;
    }
    if (with_weekday)
        snprintf(buf, size, "%s, %d %s %d · %02d:%02d", days[tm.tm_wday],
                 tm.tm_mday, months[tm.tm_mon], tm.tm_year + 1900,
                 tm.tm_hour, tm.tm_min);
    else    /* at most 23 columns: "30 Ağustos 2026 · 23:59" */
        snprintf(buf, size, "%d %s %d · %02d:%02d", tm.tm_mday,
                 months[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
}

/* ------------------------------------------------------- screen handling */

void ui_shutdown(void)
{
    if (alt_screen) {
        fputs(ALT_SCREEN_OFF, stdout);
        fflush(stdout);
        alt_screen = 0;
    }
}

/*
 * If the shell is killed while on the alternate screen, put the terminal
 * back first; otherwise the user would be left on a blank screen.
 */
static void on_fatal_signal(int sig)
{
    if (alt_screen) {
        ssize_t r = write(STDOUT_FILENO, ALT_SCREEN_OFF, sizeof ALT_SCREEN_OFF - 1);
        (void)r;
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

static void install_signal_handlers(void)
{
    /* Ctrl+C, Ctrl+\ and Ctrl+Z are handled in signals.c and do not end the shell. */
    static const int sigs[] = { SIGHUP, SIGTERM };
    struct sigaction sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_fatal_signal;
    sigemptyset(&sa.sa_mask);
    for (size_t i = 0; i < sizeof sigs / sizeof sigs[0]; i++)
        sigaction(sigs[i], &sa, NULL);
}

void ui_init(void)
{
    /*
     * When input comes from a file or a pipe (e.g. the test suite) the
     * shell runs non-interactively: no banner, prompt or screen changes.
     */
    g_shell.interactive = isatty(STDIN_FILENO);
    g_shell.use_color = isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;
    theme_load();

    const char *term = getenv("TERM");
    int dumb = term != NULL && strcmp(term, "dumb") == 0;

    if (g_shell.interactive && isatty(STDOUT_FILENO) && !dumb) {
        install_signal_handlers();
        atexit(ui_shutdown);
        fputs(ALT_SCREEN_ON, stdout);
        alt_screen = 1;
    }
}

void ui_clear(void)
{
    if (isatty(STDOUT_FILENO))
        fputs(CLEAR_SCREEN, stdout);
    if (g_shell.interactive)
        ui_banner();
}

/* ---------------------------------------------------------------- banner */

static const char *const logo[LOGO_ROWS] = {
    "████████╗██████╗ ███████╗██╗  ██╗",
    "╚══██╔══╝██╔══██╗██╔════╝██║  ██║",
    "   ██║   ██████╔╝███████╗███████║",
    "   ██║   ██╔══██╗╚════██║██╔══██║",
    "   ██║   ██║  ██║███████║██║  ██║",
    "   ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝",
};

static void box_edge(const char *left, const char *right)
{
    printf("%s%s", ACCENT, left);
    repeat("─", BOX_INNER);
    printf("%s%s\n", right, RESET);
}

static void box_open(void)
{
    printf("%s│%s", ACCENT, RESET);
}

/* Pads the row to the box width and draws the right border. */
static void box_close(int used)
{
    spaces(BOX_INNER - used);
    printf("%s│%s\n", ACCENT, RESET);
}

static void box_empty(void)
{
    box_open();
    box_close(0);
}

static void box_tip(const char *label, const char *cmd)
{
    const int label_cols = 26;

    box_open();
    printf("   %s✦%s %s", ACCENT, RESET, label);
    spaces(label_cols - text_width(label));
    printf("%s%s%s", BOLD, cmd, RESET);
    box_close(3 + 2 + label_cols + text_width(cmd));
}

static void banner_full(void)
{
    char date[64];
    const char *cwd = shown_cwd();
    const char *info[LOGO_ROWS] = { NULL };
    const char *style[LOGO_ROWS] = { NULL };

    ui_format_date(date, sizeof date, 0);
    if (text_width(cwd) > INFO_WIDTH)
        cwd = text_tail(cwd, INFO_WIDTH);

    info[1] = TRSH_NAME " " TRSH_VERSION;  style[1] = BOLD;
    info[2] = TRSH_USER;                   style[2] = "";
    info[4] = cwd;                         style[4] = ACCENT2;
    info[5] = date;                        style[5] = GRAY;

    box_edge("╭", "╮");
    box_empty();
    for (int i = 0; i < LOGO_ROWS; i++) {
        int used = 3 + LOGO_WIDTH + 3;

        box_open();
        printf("   %s%s%s   ", ACCENT, logo[i], RESET);
        if (info[i] != NULL) {
            /* Never let a line push the right border out. */
            const char *text = info[i];
            if (text_width(text) > INFO_WIDTH)
                text = text_tail(text, INFO_WIDTH);
            printf("%s%s%s", style[i], text, RESET);
            used += text_width(text);
        }
        box_close(used);
    }
    box_empty();
    box_tip(MSG_TIP_HELP, "yardim");
    box_tip(MSG_TIP_THEME, "tema mor");
    box_tip(MSG_TIP_EXIT, "cikis");
    box_empty();
    box_edge("╰", "╯");
    putchar('\n');
}

/* For narrow terminals: no box, no logo. */
static void banner_compact(void)
{
    printf("%s%s%s %s · %s\n", BOLD, TRSH_NAME, RESET, TRSH_VERSION, TRSH_USER);
    printf("%s%s%s\n\n", GRAY, MSG_BANNER_HINT, RESET);
}

void ui_banner(void)
{
    if (term_cols() >= BOX_INNER + 2)
        banner_full();
    else
        banner_compact();
}

void ui_goodbye(void)
{
    printf("%s%s%s\n", GRAY, MSG_GOODBYE, RESET);
}

/* ---------------------------------------------------------------- prompt */

/*
 *   ╭─ Yusuf Enes Kütük · ~/projects/tr-shell · ⎇ main · 12:57  ✓
 *   ╰─❯
 *
 * The status mark appears after the first command: ✓ on success, ✗ and the
 * exit code on failure. Commands that took a second or more show their time.
 *
 * ui_prompt() prints the first line; ui_prompt_tail() returns the second,
 * which is handed to the line reader so it can redraw it while editing.
 */
const char *ui_prompt_tail(int for_readline)
{
    static char buf[128];

    if (!g_shell.use_color || theme_current()->accent[0] == '\0') {
        snprintf(buf, sizeof buf, "╰─❯ ");
    } else if (for_readline) {
        /* \001 and \002 tell readline the escape codes take no space. */
        snprintf(buf, sizeof buf, "\001%s\002╰─❯\001%s\002 ",
                 theme_current()->accent, "\033[0m");
    } else {
        snprintf(buf, sizeof buf, "%s╰─❯%s ", theme_current()->accent, "\033[0m");
    }
    return buf;
}

void ui_prompt(void)
{
    static int first = 1;
    char clock[8];
    time_t now = time(NULL);
    struct tm tm;
    const char *branch = git_branch();

    if (localtime_r(&now, &tm) != NULL)
        snprintf(clock, sizeof clock, "%02d:%02d", tm.tm_hour, tm.tm_min);
    else
        clock[0] = '\0';

    /* A blank line between a command's output and the next prompt. */
    if (!first)
        putchar('\n');
    first = 0;

    printf("%s╭─%s %s%s%s %s·%s %s%s%s",
           ACCENT, RESET, BOLD, TRSH_USER, RESET,
           GRAY, RESET, ACCENT, shown_cwd(), RESET);
    if (branch != NULL)
        printf(" %s·%s %s⎇ %s%s", GRAY, RESET, ACCENT2, branch, RESET);
    if (clock[0])
        printf(" %s· %s%s", GRAY, clock, RESET);

    if (g_shell.has_run) {
        if (g_shell.last_status == 0)
            printf("  %s✓%s", GREEN, RESET);
        else
            printf("  %s✗ %d%s", RED, g_shell.last_status, RESET);

        long ms = g_shell.last_duration_ms;
        if (ms >= 1000)
            printf(" %s%ld,%ld %s%s", GRAY, ms / 1000, (ms % 1000) / 100,
                   MSG_SECONDS, RESET);
    }

    putchar('\n');
    fflush(stdout);
}

/* ---------------------------------------------------------------- errors */

void ui_error(const char *who, const char *fmt, ...)
{
    va_list ap;

    /* Keep stdout and stderr in order when both go to the same pipe. */
    fflush(stdout);
    fprintf(stderr, "%s%s:%s ", RED, who, RESET);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
