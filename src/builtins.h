/*
 * builtins.h - shell'in kendi işleminde çalışan yerleşik komutlar.
 */
#ifndef TRSH_BUILTINS_H
#define TRSH_BUILTINS_H

/*
 * argv[0] yerleşik bir komutsa çalıştırır ve 1 döner.
 * Yerleşik değilse hiçbir şey yapmaz ve 0 döner.
 */
int builtin_run(int argc, char **argv);

#endif
