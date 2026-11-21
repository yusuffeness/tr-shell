/*
 * files.h - `listele` and `agac`, written in C instead of calling ls/tree.
 */
#ifndef TRSH_FILES_H
#define TRSH_FILES_H

/*
 * listele [-a] [-d] [YOL...]
 *   Directories first, then files, each group sorted by name.
 *   -a  include hidden entries   -d / -l  detailed listing
 */
int bi_listele(int argc, char **argv);

/*
 * agac [-a] [YOL] [DERİNLİK]
 *   Draws the directory tree (default depth 3) without following links.
 */
int bi_agac(int argc, char **argv);

#endif
