/*
 * main.c - tr-shell main loop.
 *
 * Each iteration: show the prompt, read a line, split it into arguments,
 * run it as a tr-shell command if it is one, otherwise as an external
 * program in a child process.
 */
#include <stdio.h>

#include "shell.h"
#include "ui.h"
#include "input.h"
#include "parser.h"
#include "commands.h"
#include "executor.h"

struct shell g_shell = {
    .last_status = 0,
    .interactive = 0,
    .use_color   = 0,
    .running     = 1,
};

int main(void)
{
    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    ui_init();
    if (g_shell.interactive)
        ui_banner();

    while (g_shell.running) {
        if (g_shell.interactive)
            ui_prompt();

        int r = input_read_line(line, sizeof line);
        if (r == 0) {           /* Ctrl+D or end of file */
            if (g_shell.interactive)
                putchar('\n');
            break;
        }
        if (r < 0) {            /* line too long */
            g_shell.last_status = 1;
            continue;
        }

        int argc = parse_line(line, argv, MAX_ARGS);
        if (argc < 0) {
            g_shell.last_status = 1;
            continue;
        }
        if (argc == 0)          /* empty line */
            continue;

        if (!commands_run(argc, argv))
            g_shell.last_status = executor_run(argv);

        /* Builtin output must not lag behind the next command's output. */
        fflush(stdout);
    }

    if (g_shell.interactive)
        ui_goodbye();
    return g_shell.last_status;
}
