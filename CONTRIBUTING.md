# Contributing to tr-shell

Thanks for taking the time to improve tr-shell. Bug reports, new commands, better
messages and documentation fixes are all welcome.

## Reporting a bug

Open an issue with the **Bug report** template. The most useful report has:

- the smallest input that shows the problem — a `trsh -c '...'` line is ideal,
- what you expected and what happened instead,
- the output of `trsh --surum` and your system (for example *Ubuntu 24.04 on WSL 2*).

## Setting up

```bash
sudo apt install build-essential libreadline-dev   # Debian, Ubuntu, WSL
git clone https://github.com/yusuffeness/tr-shell.git
cd tr-shell
make && make test
```

Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for a tour of the code: how a line
travels from the prompt to `execvp`, and which file does what.

## Making a change

1. Create a branch: `git checkout -b fix-pipe-exit-status`.
2. Make the change and add a test for it (see below).
3. Run the checks that CI runs:

   ```bash
   make clean && make test                  # gcc, with readline
   make clean && make READLINE=0 test       # without readline
   make clean && make CC=clang test         # clang
   make debug && bash tests/run.sh          # AddressSanitizer + UBSan
   ```

4. Open a pull request. Describe what changed and how you tested it; the template
   has a short checklist.

## Code style

- C11, POSIX.1-2008. The build uses `-Wall -Wextra -Werror -pedantic`, so a warning
  is a build failure.
- Four-space indentation, braces on the same line except for function bodies,
  `snake_case` names. Keep functions short and give each file one job.
- Comments and identifiers are in English. Everything the user sees is in Turkish and
  lives in `src/messages.h`, so the wording can be reviewed in one place.
- Check every system call and report failures with `ui_error()`. Free what you
  allocate; the sanitizer run catches leaks.
- Commit messages are one short line in the imperative mood:
  `Add agac depth option`, `Fix exit status of a stopped pipeline`.

## Adding a command

Every Turkish command is one entry in the table in `src/commands.c`. Help output,
Tab completion and "did you mean" suggestions are generated from it.

An **alias** runs an existing program with fixed arguments:

```c
ALIAS("kopyala", RUN("cp", "-r"), G_FILES,
      "kopyala KAYNAK HEDEF", "Dosya ya da klasör kopyalar"),
```

A **builtin** is a C function that runs inside the shell, with the usual
`int fn(int argc, char **argv)` signature and the exit status as its return value:

```c
BUILTIN("tarih", bi_tarih, G_SYSTEM,
        "tarih", "Tarihi ve saati Türkçe gösterir"),
```

Builtins that are more than a few lines get their own file (`files.c`, `calc.c`,
`trash.c` ...). Builtin output goes through `stdout`, so redirections and pipes work
without extra code.

## Writing tests

A test is a pair of files in `tests/cases/`:

- `NAME.in` — lines typed into the shell,
- `NAME.out` — the expected output (stdout and stderr together) followed by
  `[exit N]`.

Each test runs in a fresh, empty `$HOME` with `LC_ALL=C` and `NO_COLOR=1`. If part of
the output changes from run to run (a time, a size), add `NAME.sed` with `sed -E`
expressions that replace it with a fixed placeholder. Scripts and data files that
tests use go in `tests/helpers/`; the binary under test is available as `$TRSH`.

```bash
bash tests/run.sh             # all tests
bash tests/run.sh kaynak      # tests whose name contains "kaynak"
UPDATE=1 bash tests/run.sh 45 # write the .out file for a new test, then review it
```

Always read a generated `.out` file before committing it: it becomes the definition
of correct behaviour.

## Releasing

1. Update `TRSH_VERSION` in `src/shell.h`, the man page header and `CHANGELOG.md`.
2. Commit, then tag: `git tag v1.1 && git push && git push --tags`.
3. The release workflow builds, tests and publishes the archive. Its notes come from
   the matching section of `CHANGELOG.md`.

## License

By contributing you agree that your work is released under the [MIT License](LICENSE).
