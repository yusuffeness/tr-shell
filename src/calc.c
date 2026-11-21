/*
 * calc.c - `hesapla`: a small calculator.
 *
 * A recursive-descent parser, one function per precedence level:
 *
 *   expr    := term  (('+' | '-') term)*
 *   term    := unary (('*' | '/' | '%') unary)*
 *   unary   := ('-' | '+') unary | power
 *   power   := primary ('^' unary)?        right-associative: 2^3^2 = 2^9
 *   primary := NUMBER | '(' expr ')'
 *
 * so -2^2 is -(2^2) = -4, as in mathematics.
 */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc.h"
#include "messages.h"
#include "shell.h"
#include "ui.h"
#include "util.h"

struct parser {
    const char *p;
    const char *error;
};

static double expr(struct parser *ps);

static void skip_spaces(struct parser *ps)
{
    while (isspace((unsigned char)*ps->p))
        ps->p++;
}

static int accept(struct parser *ps, char c)
{
    skip_spaces(ps);
    if (*ps->p == c) {
        ps->p++;
        return 1;
    }
    return 0;
}

static void fail(struct parser *ps, const char *msg)
{
    if (ps->error == NULL)
        ps->error = msg;
}

static double number(struct parser *ps)
{
    char buf[64];
    size_t n = 0;
    int seen_point = 0;

    while (n + 1 < sizeof buf) {
        char c = *ps->p;
        if (isdigit((unsigned char)c)) {
            buf[n++] = c;
        } else if ((c == '.' || c == ',') && !seen_point) {
            buf[n++] = '.';     /* accept the Turkish decimal comma too */
            seen_point = 1;
        } else {
            break;
        }
        ps->p++;
    }
    buf[n] = '\0';
    if (n == 0 || strcmp(buf, ".") == 0) {
        fail(ps, MSG_CALC_EXPECTED);
        return 0;
    }
    return strtod(buf, NULL);
}

static double primary(struct parser *ps)
{
    skip_spaces(ps);
    if (accept(ps, '(')) {
        double v = expr(ps);
        if (!accept(ps, ')'))
            fail(ps, MSG_CALC_PAREN);
        return v;
    }
    if (isdigit((unsigned char)*ps->p) || *ps->p == '.' || *ps->p == ',')
        return number(ps);

    fail(ps, *ps->p ? MSG_CALC_BAD_CHAR : MSG_CALC_EXPECTED);
    return 0;
}

static double unary(struct parser *ps);

static double power(struct parser *ps)
{
    double base = primary(ps);
    if (accept(ps, '^'))
        return pow(base, unary(ps));
    return base;
}

static double unary(struct parser *ps)
{
    if (accept(ps, '-'))
        return -unary(ps);
    if (accept(ps, '+'))
        return unary(ps);
    return power(ps);
}

static double term(struct parser *ps)
{
    double v = unary(ps);

    for (;;) {
        if (accept(ps, '*')) {
            v *= unary(ps);
        } else if (accept(ps, '/')) {
            double d = unary(ps);
            if (d == 0)
                fail(ps, MSG_CALC_DIV_ZERO);
            else
                v /= d;
        } else if (accept(ps, '%')) {
            double d = unary(ps);
            if (d == 0)
                fail(ps, MSG_CALC_DIV_ZERO);
            else
                v = fmod(v, d);
        } else {
            return v;
        }
    }
}

static double expr(struct parser *ps)
{
    double v = term(ps);

    for (;;) {
        if (accept(ps, '+'))
            v += term(ps);
        else if (accept(ps, '-'))
            v -= term(ps);
        else
            return v;
    }
}

int calc_eval(const char *text, double *result, const char **error)
{
    struct parser ps = { text, NULL };
    double v = expr(&ps);

    skip_spaces(&ps);
    if (ps.error == NULL && *ps.p != '\0')
        ps.error = *ps.p == ')' ? MSG_CALC_PAREN : MSG_CALC_BAD_CHAR;
    if (ps.error == NULL && !isfinite(v))
        ps.error = MSG_CALC_RANGE;

    if (ps.error != NULL) {
        *error = ps.error;
        return -1;
    }
    *result = v;
    return 0;
}

/* Whole numbers without decimals; others with up to 10 significant digits. */
static void print_number(double v)
{
    char buf[64];

    if (fabs(v) < 1e15 && v == floor(v)) {
        printf("%.0f\n", v == 0 ? 0.0 : v);     /* no "-0" */
        return;
    }
    snprintf(buf, sizeof buf, "%.10g", v);
    for (char *c = buf; *c; c++)
        if (*c == '.')
            *c = ',';
    puts(buf);
}

int bi_hesapla(int argc, char **argv)
{
    char text[MAX_LINE];
    const char *error;
    double v;

    if (argc < 2) {
        ui_error(argv[0], "%s %s", MSG_HELP_USAGE, "hesapla İFADE  (ör. hesapla 3*(4+2))");
        return 1;
    }
    if (!join_args(text, sizeof text, argc - 1, argv + 1)) {
        ui_error(argv[0], MSG_LINE_TOO_LONG, MAX_LINE - 2);
        return 1;
    }
    if (calc_eval(text, &v, &error) != 0) {
        ui_error(argv[0], "%s", error);
        return 1;
    }
    print_number(v);
    return 0;
}
