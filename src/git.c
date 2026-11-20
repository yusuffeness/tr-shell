/*
 * git.c - git branch of the current directory, for the prompt.
 */
#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "git.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define REF_PREFIX    "ref: refs/heads/"
#define GITDIR_PREFIX "gitdir: "
#define SHORT_HASH    7

/* Reads the first line of a file without the newline. Returns 0 on failure. */
static int read_line(const char *path, char *buf, size_t size)
{
    FILE *f = fopen(path, "r");

    if (f == NULL)
        return 0;
    int ok = fgets(buf, (int)size, f) != NULL;
    fclose(f);
    if (ok)
        buf[strcspn(buf, "\r\n")] = '\0';
    return ok;
}

/* Turns the contents of HEAD into a branch name or a short hash. */
static const char *parse_head(const char *head)
{
    static char name[256];
    size_t plen = strlen(REF_PREFIX);

    if (strncmp(head, REF_PREFIX, plen) == 0)
        snprintf(name, sizeof name, "%s", head + plen);
    else
        snprintf(name, sizeof name, "%.*s", SHORT_HASH, head);
    return name[0] ? name : NULL;
}

/* snprintf that reports truncation; a truncated path is treated as absent. */
#define FITS(n, buf) ((n) >= 0 && (size_t)(n) < sizeof (buf))

/*
 * Looks for a repository in dir: either a .git directory, or a .git file
 * pointing elsewhere (worktrees and submodules).
 */
static const char *branch_in(const char *dir)
{
    char path[PATH_MAX];
    char line[PATH_MAX];
    int n;

    n = snprintf(path, sizeof path, "%s/.git/HEAD", dir);
    if (FITS(n, path) && read_line(path, line, sizeof line))
        return parse_head(line);

    n = snprintf(path, sizeof path, "%s/.git", dir);
    if (!FITS(n, path) || !read_line(path, line, sizeof line) ||
        strncmp(line, GITDIR_PREFIX, strlen(GITDIR_PREFIX)) != 0)
        return NULL;

    const char *gitdir = line + strlen(GITDIR_PREFIX);
    char head[PATH_MAX];

    if (gitdir[0] == '/')
        n = snprintf(path, sizeof path, "%s/HEAD", gitdir);
    else
        n = snprintf(path, sizeof path, "%s/%s/HEAD", dir, gitdir);
    if (FITS(n, path) && read_line(path, head, sizeof head))
        return parse_head(head);
    return NULL;
}

const char *git_branch(void)
{
    char dir[PATH_MAX];

    if (getcwd(dir, sizeof dir) == NULL)
        return NULL;

    /* Walk up towards / until a repository is found. */
    for (;;) {
        const char *branch = branch_in(dir);
        if (branch != NULL)
            return branch;

        char *slash = strrchr(dir, '/');
        if (slash == NULL || (slash == dir && dir[1] == '\0'))
            return NULL;
        if (slash == dir)
            dir[1] = '\0';      /* parent is the root */
        else
            *slash = '\0';
    }
}
