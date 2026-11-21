/*
 * trash.h - `sil` and `copkutusu`: deleting that can be undone.
 *
 * sil never removes anything. It asks first, then moves the file or folder
 * to ~/.trsh/cop and records where it came from in ~/.trsh/cop.log.
 * copkutusu lists, restores or (after confirmation) empties the trash.
 */
#ifndef TRSH_TRASH_H
#define TRSH_TRASH_H

/* sil [-e] YOL...   (-e: do not ask) */
int bi_sil(int argc, char **argv);

/* copkutusu | copkutusu geri NO|AD | copkutusu bosalt [-e] */
int bi_copkutusu(int argc, char **argv);

#endif
