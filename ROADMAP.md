# tr-shell Roadmap

The project is built in nine phases. Each phase ends with one commit and a version tag.

| Phase | Topic | Version | Status |
| --- | --- | --- | --- |
| 1 | Infrastructure | v0.1 | ✅ Done |
| 2 | Turkish commands | v0.2 | ✅ Done |
| 3 | Look and feel | v0.3 | ✅ Done |
| 4 | Usability | v0.4 | ✅ Done |
| 5 | Custom commands | v0.5 | ✅ Done |
| 6 | Shell features | v0.6 | ✅ Done |
| 7 | Aliases and config | v0.7 | ✅ Done |
| 8 | Scripting | v0.8 | ✅ Done |
| 9 | Release | v1.0 | ⏳ Next |

---

## Phase 1: Infrastructure → v0.1 ✅

- [x] Repository and first commit
- [x] Split the code into modules under `src/` (ui, input, parser, builtins, executor)
- [x] Makefile (`make`, `run`, `test`, `debug`, `install`, `clean`)
- [x] Test runner (`tests/run.sh`) and non-interactive mode
- [x] README and LICENSE

## Phase 2: Turkish commands → v0.2 ✅

- [x] Command table: every command has a name, group, usage and description
- [x] Alias commands mapped to standard tools: `listele`, `oku`, `kopyala`, `tasi`, `klasor`, `olustur`, `bul`, `ara`, `boyut`, `surecler`, `oldur`, `disk`, `bellek`, `kimim`
- [x] Builtins: `gec`, `geri`, `ev`, `nerdeyim`, `cikis`
- [x] `yardim`, `hakkinda`, `tarih`
- [x] All messages in Turkish, collected in `src/messages.h`; Turkish text for common system errors
- [x] Tests for the new commands

## Phase 3: Look and feel → v0.3 ✅

- [x] Alternate screen: previous terminal output is hidden and comes back on exit, also when the shell is killed
- [x] Welcome box with logo, version, directory and date; plain header on terminals narrower than 66 columns
- [x] Two-line prompt with time, ✓ / ✗ and exit code, duration of commands that take a second or more
- [x] Git branch in the prompt, read from `.git/HEAD` without starting a process
- [x] Themes (`tema`): mavi, yesil, mor, turuncu, sade; saved in `~/.trsh/theme`
- [x] `temizle` (clear screen)

## Phase 4: Usability → v0.4 ✅

- [x] Line editing with GNU readline (detected at build time; plain input without it)
- [x] History on the arrow keys and Ctrl+R, saved in `~/.trsh/history`; `gecmis`, `!!`, `!N`, `!-N`, `!PREFIX`
- [x] Tab completion: commands and programs in `PATH` for the first word, file names after it
- [x] Ctrl+C drops the current line at the prompt and stops a running command, not the shell; Ctrl+\ and Ctrl+Z no longer kill or suspend the shell
- [x] "Did you mean …?" suggestions for mistyped commands (edit distance)
- [x] `zamanla`: wall-clock, user and system time of a command

## Phase 5: Custom commands → v0.5 ✅

- [x] `listele` written in C: directories first, natural order, colors, columns on a terminal, `-a` hidden files, `-d` details with human-readable sizes
- [x] `agac`: directory tree with depth limit, without following links
- [x] `sistem`: OS, kernel, CPU, memory and swap, disk, uptime, load and process count, with usage bars
- [x] `sil`: asks first and moves to `~/.trsh/cop` instead of deleting; refuses `.`, `..` and `/`
- [x] `copkutusu`: list, restore by number or name, empty after confirmation
- [x] `hesapla`: recursive-descent calculator with `+ - * / % ^`, parentheses and decimal commas
- [x] `not` / `notlar`: quick notes in `~/.trsh/notlar`

## Phase 6: Shell features → v0.6 ✅

- [x] Lexer: single and double quotes, backslash escapes, `#` comments
- [x] Recursive-descent parser building a syntax tree (lists, and/or, pipelines, commands, redirections) with clear syntax errors
- [x] Redirection: `<`, `>`, `>>`, `2>`, `2>>`, `2>&1`; also for builtins, which run in the shell with their descriptors saved and restored
- [x] Pipelines of any length with `pipe` and `dup2`
- [x] `&&`, `||`, `;`
- [x] Variables: `$NAME`, `${NAME}`, `$?`, `$$`, `~`; `NAME=value` and `NAME=value command`; `degisken`
- [x] File name patterns: `*`, `?`, `[...]`
- [x] Job control: `&`, Ctrl+Z, `isler`, `on`, `arka`; each job in its own process group, terminal handed over with `tcsetpgrp`, terminal settings kept per job

## Phase 7: Aliases and config → v0.7 ✅

- [x] `takmaad` / `takmaadsil`: user-defined commands, expanded in command position (also after `|`, `&&`, `;`), may contain pipes and `&&`, protected against self-reference loops, completed with Tab
- [x] Config file `~/.trsh/config`: theme, name in the prompt, welcome screen (`acik` / `sade` / `kapali`), history size, aliases
- [x] Bad lines are reported with their line number and skipped; the shell always starts
- [x] `ayar`: show, change (`ayar isim ...`), edit in `$EDITOR` (`ayar duzenle`), reset (`ayar sifirla`)
- [x] The old `~/.trsh/theme` file from v0.3–v0.6 is still honored

## Phase 8: Scripting → v0.8 ✅

- [x] Script mode (`trsh script.trsh ARG...`), `trsh -c`, `#!/usr/bin/env trsh`, `--surum` / `--yardim`
- [x] `kaynak` (source), with a nesting limit; startup file `~/.trsh/baslangic`
- [x] `eger ... ise ... degilse ... son`, `dongu ... yap ... son`, `her X icin ... yap ... son`, `kir`, `devam`; blocks nest, may span lines, take redirections and run in pipelines or in the background
- [x] `$(...)` command substitution, nested and quoted; unquoted output is split into words
- [x] Script arguments `$0`–`$9`, `$#`, `$@` / `"$@"`; `cikis [CODE]`
- [x] `#` comments; multi-line input with a `┆` continuation prompt, also after a trailing `|`, `&&` or `||`; Ctrl+C cancels an open block or stops a loop

## Phase 9: Release → v1.0

- [ ] Complete test suite and GitHub Actions
- [ ] Final README with screenshots, v1.0
