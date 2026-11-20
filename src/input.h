/*
 * input.h - standart girdiden satır okuma.
 */
#ifndef TRSH_INPUT_H
#define TRSH_INPUT_H

#include <stddef.h>

/*
 * Bir satır okur.
 * Dönüş: 1 = satır okundu, 0 = girdi sonu (EOF) ya da okuma hatası,
 *        -1 = satır çok uzundu (satırın geri kalanı atıldı).
 */
int input_read_line(char *buf, size_t size);

#endif
