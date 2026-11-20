/*
 * parser.h - komut satırını argümanlara ayırma.
 */
#ifndef TRSH_PARSER_H
#define TRSH_PARSER_H

/*
 * Satırı boşluk ve sekmelere göre böler, argv'yi NULL ile sonlandırır.
 * line yerinde değiştirilir; argv elemanları line içine işaret eder.
 * Dönüş: argüman sayısı, ya da sınır aşıldıysa -1.
 */
int parse_line(char *line, char **argv, int max_args);

#endif
