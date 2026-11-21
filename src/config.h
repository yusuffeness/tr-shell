/*
 * config.h - paths under the per-user directory ~/.trsh.
 */
#ifndef TRSH_CONFIG_H
#define TRSH_CONFIG_H

#include <stddef.h>

/*
 * Writes ~/.trsh/<file> into buf, or ~/.trsh itself when file is NULL.
 * Returns 1 on success, 0 if HOME is unset or the path does not fit.
 */
int config_path(char *buf, size_t size, const char *file);

/* Creates ~/.trsh if needed. Returns 0 on success; prints an error as who. */
int config_ensure_dir(const char *who);

#endif
