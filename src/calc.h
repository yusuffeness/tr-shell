/*
 * calc.h - `hesapla`: a small calculator.
 *
 *   hesapla 3*(4+2)^2      ->  108
 *   hesapla 10 / 4         ->  2,5
 *
 * Supports + - * / % ^, parentheses, unary minus and decimals written with
 * either a comma or a dot.
 */
#ifndef TRSH_CALC_H
#define TRSH_CALC_H

/*
 * Evaluates expr. Returns 0 and stores the value in *result, or returns -1
 * and points *error at a Turkish message.
 */
int calc_eval(const char *expr, double *result, const char **error);

int bi_hesapla(int argc, char **argv);

#endif
