# tr-shell

A small Unix shell written in C, with Turkish commands and a colorful prompt.

tr-shell started as an Operating Systems course assignment and is being grown into a
complete, everyday-usable shell. It is a hands-on project around the classic process
APIs: `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, signals, process groups and
`tcsetpgrp`.

> **Status:** v0.8 — scripting. See [ROADMAP.md](ROADMAP.md) for what comes next.

```text
╭────────────────────────────────────────────────────────────────╮
│                                                                │
│   ████████╗██████╗ ███████╗██╗  ██╗                            │
│   ╚══██╔══╝██╔══██╗██╔════╝██║  ██║   tr-shell 0.8.0           │
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

- **A real shell language:** quotes, pipes, redirection, `&&` / `||` / `;`, variables,
  file name patterns and job control — see [Shell syntax](#shell-syntax)
- **Scripts in Turkish:** `eger` / `degilse`, `dongu`, `her`, `$(...)` and script
  arguments; run files with `trsh betik.trsh` — see [Scripting](#scripting)
- **Its own screen.** Opens on the terminal's alternate screen like `vim` or `less`;
  your previous terminal output comes back when you leave
- **Welcome box** with logo, version, directory and date (a plain header on narrow
  terminals)
- **Two-line prompt** with directory, git branch and time; ✓ or ✗ with the exit code
  after each command, and the duration of commands that take a second or more
- **Themes:** `tema mor` switches the accent color and remembers it
- **Your own commands:** `takmaad ll=listele -d`, kept between sessions
- **A config file** for the theme, the name in the prompt, the welcome screen and
  history size — see [Configuration](#configuration)
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
| | `bul "PATTERN"` | Find files by name below the current directory | `find . -name` |
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
| | `takmaad [NAME=COMMAND]` | Define your own command, or list them | builtin |
| | `takmaadsil NAME...` | Remove an alias | builtin |
| | `ayar [KEY VALUE \| duzenle \| sifirla]` | Show or change settings | builtin |
| | `isler` | List background and stopped jobs | builtin |
| | `on [N]` | Bring a job to the foreground (`fg`) | builtin |
| | `arka [N]` | Continue a stopped job in the background (`bg`) | builtin |
| | `degisken [NAME=VALUE... \| sil NAME]` | List, set or remove environment variables | builtin |
| | `tema [NAME]` | List themes or switch: `mavi`, `yesil`, `mor`, `turuncu`, `sade` | builtin |
| | `temizle` | Clear the screen | builtin |
| | `hakkinda` | Version and author | builtin |
| | `kaynak FILE` | Run a script in the current shell (`source`) | builtin |
| | `kir` / `devam` | Leave a loop / go to its next turn (`break` / `continue`) | builtin |
| | `cikis [CODE]` / `exit` | Leave the shell, optionally with an exit code | builtin |
| | `cd DIR` | Classic change directory | builtin |

For commands that run a standard tool, extra arguments are passed through:
`kopyala -v a b` runs `cp -r -v a b`.

## Shell syntax

```bash
echo "iki   boşluk" 'tek $HOME' a\ b      # quotes and escapes
oku notlar.txt | grep sınav | wc -l     # pipelines
listele > liste.txt 2> hatalar.txt     # redirection: < > >> 2> 2>> 2>&1
make && ./trsh || echo derlenemedi     # run the next one on success / failure
klasor yeni; gec yeni                  # one after the other
AD=Enes; echo "Merhaba $AD" ~         # variables: $AD ${AD} $? $$ ~
DEBUG=1 ./program                      # variable for one command only
oku *.c                                # * ? [...] match file names
sleep 100 &                            # background job
echo "bugün $(tarih)"                  # command substitution
# a comment
```

**Job control:** Ctrl+Z stops the running job, `isler` lists jobs, `on` brings one back to
the foreground and `arka` lets it continue in the background. Ctrl+C stops only the
foreground job, never the shell.

Not supported: here-documents, functions and word splitting of unquoted variables
(`$(...)` and `$@` are split, as in bash).

## Scripting

Control flow uses Turkish keywords. Blocks can be written on one line or on several;
in the interactive shell, an open block shows a `┆` prompt until its `son`.

```bash
eger test -f notlar.txt ise         # if ... then
    echo "notlar var"
degilse                             # else
    echo "notlar yok"
son                                 # fi

dongu test $i -lt 3 yap echo $i; degisken i=$(expr $i + 1); son   # while

her dosya icin *.c yap              # for ... in ... do
    echo "derleniyor: $dosya"
son
```

| Keyword | bash | |
|---|---|---|
| `eger KOMUT ise ... [degilse ...] son` | `if; then; else; fi` | `ise` may follow the condition without a `;` |
| `dongu KOMUT yap ... son` | `while; do; done` | |
| `her AD icin KELİMELER yap ... son` | `for in; do; done` | words are globbed and expanded |
| `kir`, `devam` | `break`, `continue` | |

Keywords count only where a command starts and when unquoted: `echo son` prints
`son`, and `"ise"` is an ordinary word.

**Scripts.** Put commands in a file and run it with `trsh betik.trsh arg1 arg2`, or
make it executable with a `#!/usr/bin/env trsh` first line. Inside, `$0` is the script
name, `$1` … `$9` the arguments, `$#` their count and `$@` all of them (`"$@"` keeps
each argument whole). `cikis 3` ends the script with exit code 3.

```bash
trsh ornekler/selam.trsh Ayşe Mehmet     # run a script with arguments
trsh -c 'echo $0 $1' ad birinci          # run one command string
kaynak ayarlarim.trsh                    # run a file in this shell (keeps variables)
```

**Startup file.** If `~/.trsh/baslangic` exists, the interactive shell runs it before
showing the prompt — a place for variables and aliases, like `~/.bashrc`.

## Configuration

Settings and aliases live in `~/.trsh/config`. You can edit it by hand (`ayar duzenle`
opens it in `$EDITOR`) or change it from the shell:

```bash
ayar isim Yusuf Enes Kütük      # name shown in the prompt (default: your account name)
ayar tema mor                   # mavi, yesil, mor, turuncu, sade
ayar karsilama sade             # welcome screen: acik, sade, kapali
ayar gecmis_boyutu 5000         # history lines to keep
takmaad ll=listele -d           # your own command
takmaad guncelle='git pull && make'
```

```ini
tema = mor
isim = Yusuf Enes Kütük
karsilama = acik
gecmis_boyutu = 1000

[takmaad]
ll = listele -d
guncelle = git pull && make
```

A line the shell cannot understand is reported with its line number and skipped.

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

```text
trsh                          interactive shell
trsh FILE [ARG...]            run a script
trsh -c 'COMMAND' [NAME ARG...]   run one command string
trsh --surum | --yardim       version | usage
```

For the box and logo to line up, use a terminal font with box-drawing characters
(Windows Terminal with Cascadia Code, iTerm2, GNOME Terminal and most modern
terminals work out of the box).

## Project layout

```text
src/
  main.c        options (trsh FILE, -c) and the interactive loop
  run.c         running text, files, $(...) capture; kaynak
  shell.h       shared constants and shell state
  commands.c    command table, dispatch and help
  builtins.c    commands that run inside the shell (gec, geri, tarih ...)
  lexer.c       tokens: words, quotes, operators
  parser.c      syntax tree: lists, and/or, pipelines, redirections, eger/dongu/her
  expand.c      variables, $(...), $1 $# $@, ~, quote removal, file name patterns
  executor.c    pipes, redirections, builtins in the shell process, loops
  jobs.c        job control: process groups, terminal, isler/on/arka
  strvec.c      growable string arrays
  input.c       reading a line (readline or plain)
  history.c     history, `gecmis` and !-expansion
  completion.c  Tab completion
  signals.c     Ctrl+C, Ctrl+\ and Ctrl+Z handling
  suggest.c     "did you mean" suggestions
  config.c      ~/.trsh/config: settings, aliases, `ayar`
  alias.c       takmaad / takmaadsil and alias expansion
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
ornekler/       example scripts
tests/
  run.sh        test runner
  helpers/      scripts and files used by the tests
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
