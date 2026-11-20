/*
 * commands.h - the table of tr-shell commands.
 *
 * Every Turkish command is one entry. It is either a builtin (a C function
 * that runs inside the shell) or an alias (an external program plus fixed
 * arguments, e.g. klasor -> mkdir -p). The help text, and later Tab
 * completion and "did you mean" suggestions, are generated from this table.
 */
#ifndef TRSH_COMMANDS_H
#define TRSH_COMMANDS_H

enum cmd_kind {
    CMD_BUILTIN,
    CMD_ALIAS,
};

struct command {
    const char *name;
    enum cmd_kind kind;
    int (*fn)(int argc, char **argv);   /* CMD_BUILTIN */
    const char *const *target;          /* CMD_ALIAS: NULL-terminated argv */
    const char *group;                  /* section in the help list */
    const char *usage;
    const char *desc;
};

/* Returns the entry for name, or NULL if it is not a tr-shell command. */
const struct command *commands_find(const char *name);

/*
 * If argv[0] is a tr-shell command, runs it, stores its exit status in
 * g_shell.last_status and returns 1. Otherwise returns 0 and does nothing.
 */
int commands_run(int argc, char **argv);

#endif
