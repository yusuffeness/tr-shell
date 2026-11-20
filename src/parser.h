/*
 * parser.h - splitting a command line into arguments.
 */
#ifndef TRSH_PARSER_H
#define TRSH_PARSER_H

/*
 * Splits line on spaces and tabs and NULL-terminates argv.
 * line is modified in place; argv entries point into it.
 * Returns the argument count, or -1 if max_args would be exceeded.
 */
int parse_line(char *line, char **argv, int max_args);

#endif
