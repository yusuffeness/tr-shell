/*
 * jobs.h - jobs and job control.
 *
 * A job is one pipeline (or a background `a && b` list) and the processes
 * running it. In an interactive shell each job gets its own process group,
 * and the group in the foreground owns the terminal, so Ctrl+C and Ctrl+Z
 * reach the job and not the shell:
 *
 *   komut &      start in the background
 *   Ctrl+Z       stop the foreground job
 *   isler        list jobs
 *   on [N]       bring job N to the foreground (fg)
 *   arka [N]     let stopped job N continue in the background (bg)
 */
#ifndef TRSH_JOBS_H
#define TRSH_JOBS_H

#include <sys/types.h>

struct job;

/* Takes control of the terminal in an interactive shell. */
void jobs_init(void);

/* 1 if job control is active (interactive shell on a terminal). */
int jobs_control(void);

/* Turns job control off (used in forked subshells). */
void jobs_disable(void);

/* The process group a new job's processes should join (0: start a new one). */
pid_t jobs_shell_pgid(void);

/* Registers started processes as a job. pids is copied. */
struct job *jobs_add(pid_t pgid, const pid_t *pids, size_t n, const char *text);

/* Waits for a foreground job; returns its exit status. */
int jobs_wait_foreground(struct job *j);

/* Prints "[N] PID" for a job started in the background. */
void jobs_announce_background(const struct job *j);

/* Reaps finished background jobs; prints "Bitti" lines if notify is set. */
void jobs_poll(int notify);

/* On exit: stopped jobs would wait forever, so hang them up. */
void jobs_cleanup(void);

int bi_isler(int argc, char **argv);
int bi_on(int argc, char **argv);
int bi_arka(int argc, char **argv);

#endif
