/*
 * messages.h - every user-facing message in one place.
 *
 * The shell talks to the user in Turkish; keeping the strings here makes
 * them easy to review and keeps the rest of the code free of literals.
 */
#ifndef TRSH_MESSAGES_H
#define TRSH_MESSAGES_H

#define MSG_BANNER_HINT     "Komutlar için 'yardim', çıkmak için 'cikis' yazın."
#define MSG_TIP_HELP        "Komutları görmek için"
#define MSG_TIP_THEME       "Temayı değiştirmek için"
#define MSG_TIP_EXIT        "Çıkmak için"
#define MSG_GOODBYE         "Görüşmek üzere!"
#define MSG_SECONDS         "sn"

#define MSG_LINE_TOO_LONG   "satır çok uzun (en fazla %d karakter)"
#define MSG_TOO_MANY_ARGS   "çok fazla argüman (en fazla %d)"
#define MSG_NOT_FOUND       "komut bulunamadı"
#define MSG_MISSING_DIR     "dizin argümanı eksik"
#define MSG_EXTRA_ARGS      "çok fazla argüman"
#define MSG_NO_OLDPWD       "önceki dizin yok"
#define MSG_NO_HOME         "ev dizini (HOME) tanımlı değil"
#define MSG_UNKNOWN_HELP    "böyle bir komut yok: %s"
#define MSG_UNKNOWN_THEME   "böyle bir tema yok: %s ('tema' ile listeleyin)"
#define MSG_THEME_SET       "Tema: %s"
#define MSG_DID_YOU_MEAN    "Bunu mu demek istediniz: %s?"
#define MSG_HIST_NO_MATCH   "geçmişte böyle bir komut yok"
#define MSG_HIST_NOT_FOUND  "geçmişte bulunamadı: %s"
#define MSG_BAD_NUMBER      "geçersiz sayı: %s"

#define MSG_UNKNOWN_OPTION  "bilinmeyen seçenek: -%c"
#define MSG_UNKNOWN_SUBCMD  "bilinmeyen alt komut: %s"
#define MSG_CANCELLED       "Vazgeçildi."

#define MSG_LIST_TOTAL      "%d klasör, %d dosya, toplam %s"
#define MSG_TREE_TOTAL      "%d klasör, %d dosya"

#define MSG_SYS_OS          "İşletim sistemi"
#define MSG_SYS_KERNEL      "Çekirdek"
#define MSG_SYS_HOST        "Bilgisayar"
#define MSG_SYS_CPU         "İşlemci"
#define MSG_SYS_CORES       "%ld çekirdek"
#define MSG_SYS_MEMORY      "Bellek"
#define MSG_SYS_SWAP        "Takas alanı"
#define MSG_SYS_DISK        "Disk (/)"
#define MSG_SYS_UPTIME      "Açık kalma süresi"
#define MSG_SYS_DAYS        "gün"
#define MSG_SYS_HOURS       "saat"
#define MSG_SYS_MINUTES     "dakika"
#define MSG_SYS_LOAD        "Yük"
#define MSG_SYS_PROCS       "Süreç sayısı"

#define MSG_TRASH_ASK       "'%s' çöp kutusuna taşınsın mı? [e/H] "
#define MSG_TRASH_ASK_EMPTY "Çöp kutusundaki %zu öğe kalıcı olarak silinsin mi? [e/H] "
#define MSG_TRASH_MOVED     "Çöp kutusuna taşındı: %s"
#define MSG_TRASH_RESTORED  "Geri alındı: %s"
#define MSG_TRASH_EMPTIED   "%zu öğe kalıcı olarak silindi."
#define MSG_TRASH_EMPTY     "Çöp kutusu boş."
#define MSG_TRASH_HINT      "Geri almak için: copkutusu geri NO   ·   Boşaltmak için: copkutusu bosalt"
#define MSG_TRASH_REFUSE    "bu silinemez: %s"
#define MSG_TRASH_ALREADY   "zaten çöp kutusunda: %s"
#define MSG_TRASH_NO_ITEM   "çöp kutusunda böyle bir öğe yok: %s"
#define MSG_TRASH_LOG_FAIL  "çöp kutusu kaydı okunamadı ya da yazılamadı"

#define MSG_CALC_EXPECTED   "sayı bekleniyordu"
#define MSG_CALC_PAREN      "parantezler eşleşmiyor"
#define MSG_CALC_BAD_CHAR   "anlaşılmayan karakter"
#define MSG_CALC_DIV_ZERO   "sıfıra bölme"
#define MSG_CALC_RANGE      "sonuç çok büyük ya da tanımsız"

#define MSG_NOTE_ADDED      "Not eklendi (%zu)."
#define MSG_NOTE_DELETED    "Not %ld silindi."
#define MSG_NOTE_NO_SUCH    "böyle bir not yok: %ld"
#define MSG_NOTES_EMPTY     "Henüz not yok. Eklemek için: not METİN"
#define MSG_NOTES_READ_FAIL "notlar okunamadı"

#define MSG_SYNTAX          "sözdizimi hatası: beklenmeyen '%s'"
#define MSG_SYNTAX_END      "sözdizimi hatası: satır beklenmedik şekilde bitti"
#define MSG_UNCLOSED_QUOTE  "kapanmamış tırnak: %c"
#define MSG_NO_MEMORY       "bellek yetersiz"
#define MSG_BAD_ASSIGNMENT  "geçersiz atama: %s (AD=DEĞER biçiminde olmalı)"

#define MSG_JOB_RUNNING     "Çalışıyor"
#define MSG_JOB_STOPPED     "Durduruldu"
#define MSG_JOB_DONE        "Bitti"
#define MSG_JOB_FAILED      "Çıkış"
#define MSG_NO_JOBS         "iş yok"
#define MSG_NO_SUCH_JOB     "böyle bir iş yok: %s"
#define MSG_JOB_NOT_STOPPED "%d numaralı iş zaten çalışıyor"

#define MSG_TIME_REAL       "gerçek"
#define MSG_TIME_USER       "kullanıcı"
#define MSG_TIME_SYS        "sistem"

#define MSG_HELP_TITLE      "tr-shell komutları"
#define MSG_HELP_DETAIL     "Ayrıntı için: yardim KOMUT"
#define MSG_HELP_LINUX      "Linux komutları (ls, git, gcc ...) da aynen çalışır."
#define MSG_HELP_USAGE      "Kullanım:"
#define MSG_HELP_RUNS       "Karşılığı:"

#define MSG_ABOUT_AUTHOR    "Yazar:"
#define MSG_ABOUT_LICENSE   "Lisans:"
#define MSG_ABOUT_EDITING   "Satır düzenleme:"
#define MSG_EDITING_RL      "readline (oklar, Tab, Ctrl+R)"
#define MSG_EDITING_BASIC   "basit (readline olmadan derlendi)"

/* Turkish text for common errno values; falls back to strerror(). */
const char *msg_error(int errnum);

#endif
