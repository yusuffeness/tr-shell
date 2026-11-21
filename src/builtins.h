/*
 * builtins.h - commands that run inside the shell process.
 *
 * Each builtin takes the usual (argc, argv) and returns an exit status.
 * They are registered in the command table in commands.c.
 */
#ifndef TRSH_BUILTINS_H
#define TRSH_BUILTINS_H

int bi_cd(int argc, char **argv);        /* cd DIR                 */
int bi_gec(int argc, char **argv);       /* gec [DIR], home if none */
int bi_geri(int argc, char **argv);      /* back to $OLDPWD         */
int bi_ev(int argc, char **argv);        /* go to $HOME             */
int bi_nerdeyim(int argc, char **argv);  /* print working directory */
int bi_exit(int argc, char **argv);      /* exit / cikis            */
int bi_tarih(int argc, char **argv);     /* date in Turkish         */
int bi_temizle(int argc, char **argv);   /* clear screen, banner    */
int bi_hakkinda(int argc, char **argv);  /* version and author      */
int bi_degisken(int argc, char **argv);  /* list / set / unset env  */

#endif
