/*
 * shell.h - tr-shell genel tanımları ve paylaşılan durum.
 */
#ifndef TRSH_SHELL_H
#define TRSH_SHELL_H

#define TRSH_NAME    "tr-shell"
#define TRSH_VERSION "0.1.0"
#define TRSH_USER    "Yusuf Enes Kütük"

#define MAX_LINE 1024   /* bir satırın en fazla bayt sayısı (\n ve \0 dahil) */
#define MAX_ARGS 64     /* argv dizisinin boyutu (NULL sonlandırıcı dahil) */

struct shell {
    int last_status;    /* son komutun çıkış kodu (0 = başarılı) */
    int use_color;      /* çıktı terminale gidiyorsa 1 */
    int running;        /* 0 olunca ana döngü biter */
};

extern struct shell g_shell;

#endif
