/*
 * config.h - settings and aliases, kept in ~/.trsh/config.
 *
 *   tema = mor                  color theme
 *   isim = Yusuf Enes Kütük     name shown in the prompt
 *   karsilama = acik            welcome screen: acik, sade, kapali
 *   gecmis_boyutu = 1000        how many history lines to keep
 *
 *   [takmaad]
 *   ll = listele -d             user-defined commands
 *
 * The file is optional. Lines it cannot understand are reported and
 * skipped, so a typo never stops the shell from starting.
 */
#ifndef TRSH_CONFIG_H
#define TRSH_CONFIG_H

#include <stddef.h>

#include "strvec.h"

enum banner_mode {
    BANNER_FULL,        /* acik: the box with the logo */
    BANNER_PLAIN,       /* sade: one line */
    BANNER_OFF,         /* kapali */
};

struct settings {
    char name[128];
    enum banner_mode banner;
    int history_size;
    struct strvec alias_names;
    struct strvec alias_values;
};

extern struct settings g_settings;

/*
 * Writes ~/.trsh/<file> into buf, or ~/.trsh itself when file is NULL.
 * Returns 1 on success, 0 if HOME is unset or the path does not fit.
 */
int config_path(char *buf, size_t size, const char *file);

/* Creates ~/.trsh if needed. Returns 0 on success; prints an error as who. */
int config_ensure_dir(const char *who);

/* Sets the defaults, then reads the config file if there is one. */
void config_load(void);

/* Writes the current settings and aliases. Returns 0, or 1 (reported as who). */
int config_save(const char *who);

/* Aliases (the [takmaad] section). Changes are saved by config_save(). */
int alias_name_ok(const char *name);
const char *alias_get(const char *name);
int alias_define(const char *name, const char *value);
int alias_remove(const char *name);

/* `ayar`: show settings, `ayar ANAHTAR DEĞER` to change one, `ayar duzenle`. */
int bi_ayar(int argc, char **argv);

#endif
