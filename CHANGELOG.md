# Changelog

All notable changes to tr-shell are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-11-21

First stable release.

### Added
- Continuous integration on GitHub Actions: Ubuntu 24.04 (GNU coreutils) and the
  latest Ubuntu (Rust coreutils), gcc and clang, with and without readline, an
  AddressSanitizer + UBSan run of the whole test suite, and an install check.
- Failing tests are reported as annotations on GitHub.
- Release workflow: every `v*` tag is built, tested and published with a Linux binary
  archive and its SHA-256 checksum.
- Manual page (`man trsh`), installed by `make install`.
- `CONTRIBUTING.md`, `docs/ARCHITECTURE.md`, issue and pull request templates.
- Example scripts in `examples/`: `greet.trsh`, `count-c-files.trsh`, `backup.trsh`.
- README with terminal screenshots.

### Changed
- `make install` also installs the man page; `make uninstall` removes it.
- Tests run with a fixed umask and no longer depend on the coreutils implementation
  (message order, wording and broken-pipe behaviour) or on the machine having swap.
- The example scripts folder is now `examples/` (was `ornekler/`).

## [0.8.0] - 2026-11-21

### Added
- Script mode: `trsh FILE [ARG...]`, `trsh -c 'COMMAND' [NAME ARG...]`,
  `#!/usr/bin/env trsh`, `--surum` and `--yardim`.
- Control flow: `eger ... ise ... degilse ... son`, `dongu ... yap ... son`,
  `her NAME icin WORDS yap ... son`, `kir` and `devam`. Blocks nest, may span several
  lines, and can be redirected, piped or run in the background.
- `$(...)` command substitution, nested and inside double quotes.
- Script arguments: `$0`–`$9`, `$#`, `$@` and `"$@"`.
- `kaynak FILE` (source) and the startup file `~/.trsh/baslangic`.
- `cikis [CODE]` and `exit [CODE]`.
- Multi-line input with a `┆` continuation prompt, also after a trailing `|`, `&&`
  or `||`. Ctrl+C cancels an open block or stops a running loop.

### Changed
- A word made only of empty, unquoted variables is dropped, as in bash.
- `$?` reflects the last `$(...)` that ran.

## [0.7.0] - 2026-11-21

### Added
- `takmaad` and `takmaadsil`: user-defined commands that may contain pipes and
  `&&`, expanded in command position and protected against self-reference.
- Config file `~/.trsh/config` for the theme, the name in the prompt, the welcome
  screen and the history size, plus a `[takmaad]` section.
- `ayar` to show, change, edit (`ayar duzenle`) or reset (`ayar sifirla`) settings.

### Changed
- A line the config parser does not understand is reported with its line number and
  skipped; the shell always starts.

## [0.6.0] - 2026-11-21

### Added
- Recursive-descent parser building a syntax tree, with clear syntax errors.
- Single and double quotes, backslash escapes and `#` comments.
- Pipelines of any length; redirection with `<`, `>`, `>>`, `2>`, `2>>` and `2>&1`,
  also for builtins.
- `&&`, `||` and `;`.
- Variables (`$NAME`, `${NAME}`, `$?`, `$$`, `~`), `NAME=value` and
  `NAME=value command`, and `degisken`.
- File name patterns `*`, `?` and `[...]`.
- Job control: `&`, Ctrl+Z, `isler`, `on` and `arka`, with one process group per job.

## [0.5.0] - 2026-11-21

### Added
- `listele` written in C: folders first, natural order, colors, columns, `-a`, `-d`.
- `agac` directory tree, `sistem` overview, `hesapla` calculator, `not` / `notlar`.
- `sil` moves files to a trash in `~/.trsh/cop`; `copkutusu` lists, restores and
  empties it.

## [0.4.0] - 2026-11-21

### Added
- Line editing with GNU readline (detected at build time), history on the arrow
  keys and Ctrl+R, saved between sessions; `gecmis`, `!!`, `!N`, `!-N`, `!PREFIX`.
- Tab completion for commands and file names.
- "Did you mean …?" suggestions for mistyped commands.
- `zamanla` to time a command.

### Changed
- Ctrl+C discards the current line or stops the running command, never the shell.

## [0.3.0] - 2026-11-20

### Added
- Alternate screen: the previous terminal output comes back on exit.
- Welcome box with logo, version, directory and date.
- Two-line prompt with git branch, time, ✓ / ✗ with the exit code and the duration
  of slow commands.
- Color themes with `tema`; `temizle`.

## [0.2.0] - 2026-11-20

### Added
- Turkish commands with a command table, `yardim` and `hakkinda`.
- Turkish messages, including common system errors.

## [0.1.0] - 2026-11-20

### Added
- Project structure in `src/`, Makefile, test runner and non-interactive mode.

[1.0.0]: https://github.com/yusuffeness/tr-shell/compare/v0.8...v1.0
[0.8.0]: https://github.com/yusuffeness/tr-shell/compare/v0.7...v0.8
[0.7.0]: https://github.com/yusuffeness/tr-shell/compare/v0.6...v0.7
[0.6.0]: https://github.com/yusuffeness/tr-shell/compare/v0.5...v0.6
[0.5.0]: https://github.com/yusuffeness/tr-shell/compare/v0.4...v0.5
[0.4.0]: https://github.com/yusuffeness/tr-shell/compare/v0.3...v0.4
[0.3.0]: https://github.com/yusuffeness/tr-shell/compare/v0.2...v0.3
[0.2.0]: https://github.com/yusuffeness/tr-shell/compare/v0.1...v0.2
[0.1.0]: https://github.com/yusuffeness/tr-shell/releases/tag/v0.1
