/*
 * git.h - git branch of the current directory, for the prompt.
 */
#ifndef TRSH_GIT_H
#define TRSH_GIT_H

/*
 * Returns the branch name (or a short commit hash when HEAD is detached)
 * of the repository containing the current directory, or NULL if there
 * is none. Reads .git/HEAD directly; no git process is started.
 * The result points to a static buffer.
 */
const char *git_branch(void);

#endif
