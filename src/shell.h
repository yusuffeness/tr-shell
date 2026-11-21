/*
 * shell.h - global constants and shared shell state.
 */
#ifndef TRSH_SHELL_H
#define TRSH_SHELL_H

#define TRSH_NAME    "tr-shell"
#define TRSH_VERSION "0.5.0"
#define TRSH_USER    "Yusuf Enes Kütük"
#define TRSH_URL     "https://github.com/yusuffeness/tr-shell"

#define MAX_LINE 1024   /* max bytes in one input line (including \n and \0) */
#define MAX_ARGS 64     /* size of argv arrays (including the NULL terminator) */

struct shell {
    int last_status;        /* exit status of the last command (0 = success) */
    long last_duration_ms;  /* wall-clock time of the last command */
    int has_run;            /* 1 once at least one command has run */
    int interactive;        /* 1 if input comes from a terminal */
    int use_color;          /* 1 if output goes to a terminal and NO_COLOR is unset */
    int running;            /* main loop stops when this becomes 0 */
};

extern struct shell g_shell;

#endif
