# tr-shell

A small Unix shell written in C, with a colorful prompt and (soon) Turkish commands.

tr-shell started as an Operating Systems course assignment and is being grown into a
complete, everyday-usable shell. It is a hands-on project around the classic process
APIs: `fork`, `execvp`, `waitpid`, `pipe`, `dup2` and signals.

> **Status:** v0.1 — project infrastructure. See [ASAMALAR.md](ASAMALAR.md) for the roadmap.

## Features

Available now:

- Runs external programs in a child process (`fork` + `execvp` + `waitpid`)
- Built-in `cd` and `exit`
- Colored prompt showing the user, the current directory (`~` for home) and the
  last exit status when it is non-zero
- Reports children killed by a signal (e.g. `Killed`)
- Safe limits on line length and argument count
- Non-interactive mode: no prompt when input comes from a file or a pipe
- Respects `NO_COLOR`

Planned:

- Turkish commands (`listele`, `gec`, `oku`, `yardim` …) alongside the usual ones
- Full-screen look with a welcome box, two-line prompt and themes
- Line editing, persistent history and Tab completion
- Quotes, redirection, pipes, `&&` / `||` / `;`, variables and background jobs
- Aliases and a config file

## Requirements

Linux, macOS or WSL with a C11 compiler (gcc or clang) and `make`.

## Build and run

```bash
make          # builds ./trsh
make run      # builds and starts the shell
make test     # runs the test suite
make debug    # builds with AddressSanitizer and UBSan
make install  # installs to ~/.local/bin (override with PREFIX=...)
```

```text
$ ./trsh
tr-shell 0.1.0 - Yusuf Enes Kütük

Yusuf Enes Kütük ~/projects> echo hello
hello
Yusuf Enes Kütük ~/projects> cd /nowhere
cd: /nowhere: No such file or directory
Yusuf Enes Kütük ~/projects [1]> exit
```

## Project layout

```text
src/
  main.c        main loop
  shell.h       shared constants and shell state
  ui.c          colors, banner, prompt
  input.c       reading a line
  parser.c      splitting a line into arguments
  builtins.c    built-in commands (cd, exit)
  executor.c    fork / exec / wait
tests/
  run.sh        test runner
  cases/        *.in inputs and *.out expected outputs
```

## Tests

Each `tests/cases/NAME.in` is fed to the shell; its output and exit code are compared
with `NAME.out`.

```bash
bash tests/run.sh          # all tests
bash tests/run.sh cd       # only tests whose name contains "cd"
UPDATE=1 bash tests/run.sh # regenerate expected outputs after an intended change
```

## Türkçe

tr-shell, C ile yazılmış küçük bir Unix kabuğudur. İşletim Sistemleri dersi ödevi
olarak başladı ve Türkçe komutları, renkli arayüzü ve gerçek bir shell'in temel
özellikleriyle tam bir projeye dönüşüyor. Yol haritası için
[ASAMALAR.md](ASAMALAR.md) dosyasına bakın.

## License

[MIT](LICENSE) © 2026 Yusuf Enes Kütük
