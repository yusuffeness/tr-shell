# tr-shell Roadmap

The project is built in nine phases. Each phase ends with one commit and a version tag.

| Phase | Topic | Version | Status |
| --- | --- | --- | --- |
| 1 | Infrastructure | v0.1 | ✅ Done |
| 2 | Turkish commands | v0.2 | ✅ Done |
| 3 | Look and feel | v0.3 | ⏳ Next |
| 4 | Usability | v0.4 | |
| 5 | Custom commands | v0.5 | |
| 6 | Shell features | v0.6 | |
| 7 | Aliases and config | v0.7 | |
| 8 | Scripting (optional) | v0.8 | |
| 9 | Release | v1.0 | |

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

## Phase 3: Look and feel → v0.3

- [ ] Alternate screen (previous terminal output is hidden) and a welcome box
- [ ] Two-line prompt, ✓ / ✗ status indicator, duration of slow commands
- [ ] Git branch in the prompt
- [ ] Themes (`tema`) and adapting to the terminal width

## Phase 4: Usability → v0.4

- [ ] Line editing with readline and persistent history (`gecmis`, `!N`)
- [ ] Tab completion
- [ ] Ctrl+C stops the running command, not the shell
- [ ] "Did you mean …?" suggestions and `zamanla` (timing)

## Phase 5: Custom commands → v0.5

- [ ] `listele` rewritten in C (colors, human-readable sizes)
- [ ] `agac` (directory tree)
- [ ] `sistem` (CPU, memory, uptime)
- [ ] `sil` (asks for confirmation, moves to trash) and `copkutusu`
- [ ] `hesapla` (calculator) and `not` / `notlar` (quick notes)

## Phase 6: Shell features → v0.6

- [ ] New lexer: quotes and escape characters
- [ ] Parser that builds a syntax tree
- [ ] Redirection: `>`, `>>`, `<`, `2>`
- [ ] Pipes: `|`
- [ ] `&&`, `||`, `;`
- [ ] Variables: `$HOME`, `$?`, `~`, `degisken`
- [ ] Background jobs: `&`, `isler`, `on`, `arka`

## Phase 7: Aliases and config → v0.7

- [ ] `takmaad` and `takmaadsil` (user-defined aliases)
- [ ] Config file (`~/.trsh/config`)

## Phase 8: Scripting (optional) → v0.8

- [ ] Script mode (`trsh script.trsh`) and `kaynak` (source)
- [ ] `eger` (if) and `dongu` (loop)
- [ ] `$(...)` command substitution

## Phase 9: Release → v1.0

- [ ] Complete test suite and GitHub Actions
- [ ] Final README with screenshots, v1.0
