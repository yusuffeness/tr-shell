# tr-shell

A small Unix shell written in C, with Turkish commands and a colorful prompt.

tr-shell started as an Operating Systems course assignment and is being grown into a
complete, everyday-usable shell. It is a hands-on project around the classic process
APIs: `fork`, `execvp`, `waitpid`, `pipe`, `dup2` and signals.

> **Status:** v0.2 — Turkish commands. See [ROADMAP.md](ROADMAP.md) for what comes next.

## Features

- Turkish commands next to the usual ones — `ls`, `git`, `gcc` and everything else
  in your `PATH` still work
- Built-in help: `yardim` lists every command, `yardim KOMUT` explains one
- Runs external programs in a child process (`fork` + `execvp` + `waitpid`)
- Turkish messages, including common system errors
- Colored prompt with the current directory (`~` for home) and the last exit
  status when it is non-zero
- Reports children killed by a signal (e.g. `Killed`)
- Non-interactive mode when input comes from a file or a pipe
- Respects `NO_COLOR`

## Commands

| Group | Command | What it does | Runs |
| --- | --- | --- | --- |
| Files | `listele [DIR]` | List files | `ls` |
| | `oku FILE...` | Show file contents | `cat` |
| | `kopyala SRC DST` | Copy files or folders | `cp -r` |
| | `tasi SRC DST` | Move or rename | `mv` |
| | `klasor NAME...` | Create folders, with parents | `mkdir -p` |
| | `olustur FILE...` | Create empty files | `touch` |
| | `bul PATTERN` | Find files by name below the current directory | `find . -name` |
| | `ara WORD [PATH]` | Search text inside files | `grep -rn` |
| | `boyut [PATH]` | Show size | `du -sh` |
| Directories | `gec [DIR]` | Change directory; home if no argument | builtin |
| | `geri` | Go back to the previous directory | builtin |
| | `ev` | Go to the home directory | builtin |
| | `nerdeyim` | Print the current directory | builtin |
| System | `surecler` | List processes by CPU usage | `ps` |
| | `oldur PID...` | Terminate a process | `kill` |
| | `disk` | Disk usage | `df -h` |
| | `bellek` | Memory usage | `free -h` |
| | `kimim` | Current user | `whoami` |
| | `tarih` | Date and time in Turkish | builtin |
| Shell | `yardim [CMD]` | Help | builtin |
| | `hakkinda` | Version and author | builtin |
| | `cikis` / `exit` | Leave the shell | builtin |
| | `cd DIR` | Classic change directory | builtin |

Extra arguments are passed through, so `listele -la` runs `ls --color=auto -la`.

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
tr-shell 0.2.0 - Yusuf Enes Kütük
Çıkmak için 'cikis' yazın ya da Ctrl+D'ye basın. Komutlar için 'yardim'.

Yusuf Enes Kütük ~> gec /tmp
Yusuf Enes Kütük /tmp> geri
/home/enes
Yusuf Enes Kütük ~> oku yok.txt
cat: yok.txt: No such file or directory
Yusuf Enes Kütük ~ [1]> cikis
Görüşmek üzere!
```

## Project layout

```text
src/
  main.c        main loop
  shell.h       shared constants and shell state
  commands.c    command table, dispatch and help
  builtins.c    commands that run inside the shell (gec, geri, tarih ...)
  executor.c    fork / exec / wait
  parser.c      splitting a line into arguments
  input.c       reading a line
  ui.c          colors, banner, prompt, error output
  messages.c/h  all user-facing text
tests/
  run.sh        test runner
  cases/        *.in inputs and *.out expected outputs
```

## Adding a command

Add one line to the table in `src/commands.c`. An alias needs only the program it
runs:

```c
ALIAS("kopyala", RUN("cp", "-r"), G_FILES,
      "kopyala KAYNAK HEDEF", "Dosya ya da klasör kopyalar"),
```

A builtin points to a C function in `src/builtins.c`. Help output is generated from
the table automatically.

## Tests

Each `tests/cases/NAME.in` is fed to the shell; its output and exit code are compared
with `NAME.out`.

```bash
bash tests/run.sh          # all tests
bash tests/run.sh help     # only tests whose name contains "help"
UPDATE=1 bash tests/run.sh # regenerate expected outputs after an intended change
```

## License

[MIT](LICENSE) © 2026 Yusuf Enes Kütük
