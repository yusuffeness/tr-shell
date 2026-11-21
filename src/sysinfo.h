/*
 * sysinfo.h - the `sistem` command: a one-screen system summary.
 */
#ifndef TRSH_SYSINFO_H
#define TRSH_SYSINFO_H

/*
 * Shows the OS, kernel, CPU, memory, swap, disk, uptime and load.
 * Reads /proc on Linux; on other systems the parts that need /proc are
 * left out.
 */
int bi_sistem(int argc, char **argv);

#endif
