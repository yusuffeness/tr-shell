# tr-shell

A small Unix shell written in C, with Turkish commands and a colorful prompt.

tr-shell started as an Operating Systems course assignment and is being grown into a
complete, everyday-usable shell. It is a hands-on project around the classic process
APIs: `fork`, `execvp`, `waitpid`, `pipe`, `dup2` and signals.

> **Status:** v0.5 — custom commands. See [ROADMAP.md](ROADMAP.md) for what comes next.

```text
╭────────────────────────────────────────────────────────────────╮
│                                                                │
│   ████████╗██████╗ ███████╗██╗  ██╗                            │
│   ╚══██╔══╝██╔══██╗██╔════╝██║  ██║   tr-shell 0.5.0           │
│      ██║   ██████╔╝███████╗███████║   Yusuf Enes Kütük         │
│      ██║   ██╔══██╗╚════██║██╔══██║                            │
│      ██║   ██║  ██║███████║██║  ██║   ~/projects/tr-shell      │
│      ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝   20 Kasım 2026 · 13:01    │
│                                                                │
│   ✦ Komutları görmek için     yardim                           │
│   ✦ Temayı değiştirmek için   tema mor                         │
│   ✦ Çıkmak için               cikis                            │
│                                                                │
╰────────────────────────────────────────────────────────────────╯

╭─ Yusuf Enes Kütük · ~/projects/tr-shell · ⎇ main · 13:01
╰─❯ sleep 2

╭─ Yusuf Enes Kütük · ~/projects/tr-shell · ⎇ main · 13:01  ✓ 2,0 sn
╰─❯ listel
listel: komut bulunamadı. Bunu mu demek istediniz: listele?

╭─ Yusuf Enes Kütük · ~/projects/tr-shell · ⎇ main · 13:01  ✗ 127
╰─❯
```

## Features

- **Its own screen.** Opens on the terminal's alternate screen like `vim` or `less`;
  your previous terminal output comes back when you leave
- **Welcome box** with logo, version, directory and date (a plain header on narrow
  terminals)
- **Two-line prompt** with directory, git branch and time; ✓ or ✗ with the exit code
  after each command, and the duration of commands that take a second or more
- **Themes:** `tema mor` switches the accent color and remembers it
- **Line editing** with readline: history on the arrow keys and Ctrl+R, kept between
  sessions; Tab completes commands and file names; `gecmis`, `!!`, `!N`
- **Ctrl+C** drops the line you are typing or stops the running command — never the
  shell itself
- **"Did you mean …?"** when a command name has a typo
- **Commands written from scratch in C:** `listele`, `agac`, `sistem`, a calculator,
  quick notes, and a `sil` that moves files to a trash you can restore from
- **Turkish commands** next to the usual ones — `ls`, `git`, `gcc` and everything else
  in your `PATH` still work
- **Built-in help:** `yardim` lists every command, `yardim KOMUT` explains one
- Turkish messages, including common system errors
- Runs external programs in a child process (`fork` + `execvp` + `waitpid`) and
  reports children killed by a signal
- Non-interactive mode when input comes from a file or a pipe; respects `NO_COLOR`

## Commands

| Group | Command | What it does | Runs |
| --- | --- | --- | --- |
| Files | `listele [-a] [-d] [PATH...]` | List files: folders first, colors, `-a` hidden, `-d` details | builtin |
| | `agac [-a] [PATH] [DEPTH]` | Directory tree (depth 3 by default) | builtin |
| | `oku FILE...` | Show file contents | `cat` |
| | `kopyala SRC DST` | Copy files or folders | `cp -r` |
| | `tasi SRC DST` | Move or rename | `mv` |
| | `klasor NAME...` | Create folders, with parents | `mkdir -p` |
| | `olustur FILE...` | Create empty files | `touch` |
| | `bul PATTERN` | Find files by name below the current directory | `find . -name` |
| | `ara WORD [PATH]` | Search text inside files | `grep -rn` |
| | `boyut [PATH]` | Show size | `du -sh` |
| | `sil [-e] PATH...` | Ask, then move to the trash (`-e`: don't ask) | builtin |
| | `copkutusu` | List the trash; `geri NO\|NAME` restores, `bosalt` empties | builtin |
| Directories | `gec [DIR]` | Change directory; home if no argument | builtin |
| | `geri` | Go back to the previous directory | builtin |
| | `ev` | Go to the home directory | builtin |
| | `nerdeyim` | Print the current directory | builtin |
| System | `sistem` | OS, CPU, memory, disk, uptime and load at a glance | builtin |
| | `surecler` | List processes by CPU usage | `ps` |
| | `oldur PID...` | Terminate a process | `kill` |
| | `disk` | Disk usage | `df -h` |
| | `bellek` | Memory usage | `free -h` |
| | `kimim` | Current user | `whoami` |
| | `tarih` | Date and time in Turkish | builtin |
| | `zamanla CMD...` | Run a command and show how long it took | builtin |
| Tools | `hesapla EXPR` | Calculator: `+ - * / % ^`, parentheses, `1,5` or `1.5` | builtin |
| | `not TEXT...` | Save a quick note | builtin |
| | `notlar [sil N]` | List notes or delete one | builtin |
| Shell | `yardim [CMD]` | Help | builtin |
| | `gecmis [N]` | Command history; rerun with `!N`, `!!` or `!PREFIX` | builtin |
| | `tema [NAME]` | List themes or switch: `mavi`, `yesil`, `mor`, `turuncu`, `sade` | builtin |
| | `temizle` | Clear the screen | builtin |
| | `hakkinda` | Version and author | builtin |
| | `cikis` / `exit` | Leave the shell | builtin |
| | `cd DIR` | Classic change directory | builtin |

For commands that run a standard tool, extra arguments are passed through:
`kopyala -v a b` runs `cp -r -v a b`.

## Requirements

Linux, macOS or WSL with a C11 compiler (gcc or clang) and `make`.

GNU readline is optional but recommended: it adds line editing, history on the arrow
keys and Tab completion. The Makefile uses it automatically when it is installed.

```bash
sudo apt install build-essential libreadline-dev   # Debian, Ubuntu, WSL
```

`hakkinda` shows which mode the shell was built in; `make READLINE=0` builds without it.

## Build and run

```bash
make          # builds ./trsh
make run      # builds and starts the shell
make test     # runs the test suite
make debug    # builds with AddressSanitizer and UBSan
make install  # installs to ~/.local/bin (override with PREFIX=...)
```

For the box and logo to line up, use a terminal font with box-drawing characters
(Windows Terminal with Cascadia Code, iTerm2, GNOME Terminal and most modern
terminals work out of the box).

## Project layout

```text
src/
  main.c        main loop
  shell.h       shared constants and shell state
  commands.c    command table, dispatch and help
  builtins.c    commands that run inside the shell (gec, geri, tarih ...)
  executor.c    fork / exec / wait
  parser.c      splitting a line into arguments
  input.c       reading a line (readline or plain)
  history.c     history, `gecmis` and !-expansion
  completion.c  Tab completion
  signals.c     Ctrl+C, Ctrl+\ and Ctrl+Z handling
  suggest.c     "did you mean" suggestions
  config.c      paths under ~/.trsh
  files.c       listele and agac
  sysinfo.c     sistem
  trash.c       sil and copkutusu
  calc.c        hesapla (recursive-descent parser)
  notes.c       not and notlar
  util.c        sizes, dates, yes/no questions
  ui.c          alternate screen, banner, prompt, error output
  theme.c       color themes and the `tema` command
  git.c         git branch for the prompt
  messages.c/h  all user-facing text
tests/
  run.sh        test runner
  cases/        *.in inputs, *.out expected outputs, optional *.sed filters
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
with `NAME.out`. An optional `NAME.sed` filter blanks out parts that change between
runs, such as timings.

```bash
bash tests/run.sh          # all tests
bash tests/run.sh help     # only tests whose name contains "help"
UPDATE=1 bash tests/run.sh # regenerate expected outputs after an intended change
```

## License

[MIT](LICENSE) © 2026 Yusuf Enes Kütük
