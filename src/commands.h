/*
 * commands.h - the table of tr-shell commands.
 *
 * Every Turkish command is one entry. It is either a builtin (a C function
 * that runs inside the shell) or an alias (an external program plus fixed
 * arguments, e.g. klasor -> mkdir -p). Help text, Tab completion and
 * "did you mean" suggestions are all generated from this table.
 */
#ifndef TRSH_COMMANDS_H
#define TRSH_COMMANDS_H

#include <stddef.h>

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

/* Number of entries, and entry i (for completion and suggestions). */
size_t commands_count(void);
const struct command *commands_at(size_t i);

/*
 * Runs a command line: a builtin, an alias, or otherwise an external
 * program. Returns its exit status.
 */
int commands_exec(int argc, char **argv);

#endif
