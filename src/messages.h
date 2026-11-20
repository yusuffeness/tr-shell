/*
 * messages.h - every user-facing message in one place.
 *
 * The shell talks to the user in Turkish; keeping the strings here makes
 * them easy to review and keeps the rest of the code free of literals.
 */
#ifndef TRSH_MESSAGES_H
#define TRSH_MESSAGES_H

#define MSG_BANNER_HINT     "Çıkmak için 'cikis' yazın ya da Ctrl+D'ye basın. Komutlar için 'yardim'."
#define MSG_GOODBYE         "Görüşmek üzere!"

#define MSG_LINE_TOO_LONG   "satır çok uzun (en fazla %d karakter)"
#define MSG_TOO_MANY_ARGS   "çok fazla argüman (en fazla %d)"
#define MSG_NOT_FOUND       "komut bulunamadı"
#define MSG_MISSING_DIR     "dizin argümanı eksik"
#define MSG_EXTRA_ARGS      "çok fazla argüman"
#define MSG_NO_OLDPWD       "önceki dizin yok"
#define MSG_NO_HOME         "ev dizini (HOME) tanımlı değil"
#define MSG_UNKNOWN_HELP    "böyle bir komut yok: %s"

#define MSG_HELP_TITLE      "tr-shell komutları"
#define MSG_HELP_DETAIL     "Ayrıntı için: yardim KOMUT"
#define MSG_HELP_LINUX      "Linux komutları (ls, git, gcc ...) da aynen çalışır."
#define MSG_HELP_USAGE      "Kullanım:"
#define MSG_HELP_RUNS       "Karşılığı:"

#define MSG_ABOUT_AUTHOR    "Yazar:"
#define MSG_ABOUT_LICENSE   "Lisans:"

/* Turkish text for common errno values; falls back to strerror(). */
const char *msg_error(int errnum);

#endif
