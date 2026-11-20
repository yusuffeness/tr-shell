/*
 * executor.h - harici komutları çocuk işlemde çalıştırma.
 */
#ifndef TRSH_EXECUTOR_H
#define TRSH_EXECUTOR_H

/*
 * argv[0] programını fork + execvp ile çalıştırır ve bitmesini bekler.
 * Sonucu g_shell.last_status'a yazar.
 */
void executor_run(char **argv);

#endif
