/*
 * completion.h - Tab completion (readline builds only).
 *
 * The first word completes to tr-shell commands, aliases and programs in PATH;
 * later words complete to file names.
 */
#ifndef TRSH_COMPLETION_H
#define TRSH_COMPLETION_H

void completion_init(void);

#endif
