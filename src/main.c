/*
 * main.c - tr-shell ana döngüsü.
 *
 * Her turda: istemi göster, satırı oku, argümanlara ayır,
 * yerleşik komutsa kendin çalıştır, değilse çocuk işlemde çalıştır.
 */
#include <stdio.h>

#include "shell.h"
#include "ui.h"
#include "input.h"
#include "parser.h"
#include "builtins.h"
#include "executor.h"

struct shell g_shell = {
    .last_status = 0,
    .use_color   = 0,
    .running     = 1,
};

int main(void)
{
    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    ui_init();
    ui_banner();

    while (g_shell.running) {
        ui_prompt();

        int r = input_read_line(line, sizeof line);
        if (r == 0) {           /* Ctrl+D */
            putchar('\n');
            break;
        }
        if (r < 0) {            /* satır çok uzun */
            g_shell.last_status = 1;
            continue;
        }

        int argc = parse_line(line, argv, MAX_ARGS);
        if (argc < 0) {
            g_shell.last_status = 1;
            continue;
        }
        if (argc == 0)          /* boş satır */
            continue;

        if (builtin_run(argc, argv))
            continue;

        executor_run(argv);
    }

    ui_goodbye();
    return g_shell.last_status;
}
