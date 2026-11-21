# Architecture

This document follows one line of input from the prompt to the processes it starts,
then describes the modules around that path. It is meant for anyone who wants to read
or change the code.

## The life of a command line

```text
 keyboard ──► input.c ──► lexer.c ──► alias.c ──► parser.c ──► executor.c ──► fork / execvp
              readline     tokens     takmaad     syntax tree   expand.c       jobs.c
                                      expansion                 per command    waitpid
```

Take `her f icin *.c yap oku $f | wc -l; son > sayim.txt` as an example.

1. **Reading** — `main.c` shows the prompt (`ui.c`) and reads a line through
   `input.c`, which uses GNU readline's callback interface when it is available and a
   plain `fgets` loop otherwise. If the parser reports that a block is still open, the
   loop shows the `┆` continuation prompt and appends the next line.

2. **Tokens** — `lexer.c` splits the text into words and operators (`|`, `&&`, `;`,
   `>`, new line ...). Quotes are kept in the word text at this stage, because
   expansion later needs to know what was quoted. `$(...)` is skipped as one unit, so
   the `|` inside `$(oku a | wc -l)` is not seen as a pipe.

3. **Aliases** — `alias.c` replaces a `takmaad` name in command position with its
   tokens. An alias may contain pipes and `&&`; self-reference is stopped.

4. **Syntax tree** — `parser.c` is a recursive-descent parser:

   ```text
   script   := item (sep item)*            sep: ; & or new line
   item     := and_or ['&']
   and_or   := pipeline (('&&' | '||') pipeline)*
   pipeline := command ('|' command)*
   command  := simple | compound, followed by redirections
   compound := 'eger' script 'ise' script ['degilse' script] 'son'
             | 'dongu' script 'yap' script 'son'
             | 'her' NAME 'icin' WORD* 'yap' script 'son'
   ```

   Keywords count only in command position and only when unquoted. When the input ends
   inside a block, `parse` returns `PARSE_INCOMPLETE` instead of an error; that is how
   the interactive loop and `run_file` know to read more lines.

5. **Expansion** — just before a command runs, `expand.c` turns each raw word into
   zero or more arguments: variables, `$1`…`$#`/`$@`, `~`, `$(...)` (run by
   `run_capture` in a forked copy of the shell), quote removal, then file name patterns
   with `glob(3)`. Expanding late means `her` and `dongu` see fresh values on every
   turn.

6. **Running** — `executor.c` walks the tree:
   - a single **builtin**, a **compound command** or a bare assignment runs inside the
     shell process, with its redirections applied to saved-and-restored file
     descriptors, so `gec` and `degisken` change the shell itself;
   - anything else is **launched**: one `fork` per command, connected with `pipe` and
     `dup2`, each child calling `signals_child()` and then `execvp` (or the builtin
     function, when a builtin sits inside a pipeline);
   - `&&`, `||` and `;` are decided from exit statuses; `kir`, `devam`, `cikis` and
     Ctrl+C set flags that stop the rest of the list.

7. **Jobs** — `jobs.c` puts each pipeline in its own process group, hands the terminal
   to it with `tcsetpgrp`, and waits with `WUNTRACED`. A job stopped with Ctrl+Z is
   remembered with its terminal settings, so `on` can restore both. Finished
   background jobs are reported before the next prompt.

## Modules

| File | Responsibility |
| --- | --- |
| `main.c` | Command-line options, interactive loop, multi-line blocks, startup file |
| `run.c` | Running text, files and `$(...)` captures; `kaynak` |
| `lexer.c` | Tokens, quotes, comments, unclosed-quote detection |
| `parser.c` | Syntax tree and syntax errors |
| `expand.c` | Variables, parameters, `$(...)`, `~`, quote removal, globbing |
| `executor.c` | Pipelines, redirections, builtins in the shell, loops and conditionals |
| `jobs.c` | Process groups, terminal ownership, `isler` / `on` / `arka` |
| `signals.c` | Ctrl+C, Ctrl+\ and Ctrl+Z for the shell and its children |
| `commands.c` | The command table, dispatch and `yardim` |
| `builtins.c` | Small builtins: directories, variables, `cikis`, `kir`, `devam` ... |
| `alias.c` | `takmaad`, `takmaadsil` and alias expansion |
| `config.c` | `~/.trsh/config` and `ayar` |
| `input.c`, `history.c`, `completion.c` | Line editing, history and `!` expansion, Tab completion |
| `ui.c`, `theme.c`, `git.c` | Alternate screen, welcome box, prompt, colors, git branch |
| `files.c`, `sysinfo.c`, `trash.c`, `calc.c`, `notes.c` | `listele`/`agac`, `sistem`, `sil`/`copkutusu`, `hesapla`, `not`/`notlar` |
| `suggest.c` | "Did you mean …?" by edit distance |
| `messages.c/h` | Every user-facing string, and Turkish text for `errno` values |
| `strvec.c`, `util.c` | Growable string arrays, sizes, dates, yes/no prompts |

## Design decisions

**Builtins run in the shell, unless they are in a pipeline.** `gec src` must change
the shell's own directory, so a lone builtin is called directly. In `listele | wc -l`
it runs in a forked child like any program, which keeps pipes simple and correct.

**Compound commands run in the shell for the same reason.** Variables set inside
`her` or `eger` are visible afterwards. Inside a pipeline or with `&`, the whole block
runs in a forked copy of the shell instead.

**One table describes every command.** `commands.c` holds the name, kind, group,
usage and description of each command. Help, Tab completion and suggestions are
generated from it, so a new command cannot be forgotten in one of them.

**All user-facing text is in one header.** `messages.h` keeps the Turkish wording in
one place; the code stays in English and is easy to grep.

**The shell survives Ctrl+C.** The shell catches SIGINT and only sets a flag;
children get the default handlers back before `exec`. A loop or a list checks the
flag between commands, so Ctrl+C stops `dongu true yap sleep 1; son` as a whole.

**No unchecked limits.** Lines are limited to `MAX_LINE` and commands to `MAX_ARGS`;
longer input is reported, not truncated silently. Growing buffers are checked on
every allocation.

## Testing

`tests/run.sh` feeds each `tests/cases/*.in` to the shell and compares the output and
exit status with the `.out` file. The suite covers parsing, expansion, redirection,
jobs, every builtin, configuration and scripting. CI runs it with gcc and clang, with
and without readline, and once more under AddressSanitizer and UBSan, which also
catches memory leaks.

Interactive behaviour — the prompt, readline, Ctrl+C and Ctrl+Z — needs a real
terminal and is checked by hand before each release.
