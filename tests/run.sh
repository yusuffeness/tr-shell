#!/usr/bin/env bash
#
# tr-shell behaviour tests.
#
# Each tests/cases/NAME.in is fed to trsh on stdin. Its stdout and stderr
# are merged, "[exit N]" is appended, and the result is compared with
# tests/cases/NAME.out.
#
# If tests/cases/NAME.sed exists, the output is passed through it first
# (sed -E), to blank out parts that change from run to run, such as timings.
#
#   bash tests/run.sh              run all tests
#   bash tests/run.sh cd           run tests whose name contains "cd"
#   UPDATE=1 bash tests/run.sh     rewrite the expected outputs
#   TRSH=./other bash tests/run.sh test another binary

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
CASES="$ROOT/tests/cases"
TRSH=${TRSH:-"$ROOT/trsh"}
FILTER=${1:-}

if [ ! -x "$TRSH" ]; then
    echo "trsh not found: $TRSH (run 'make' first)" >&2
    exit 2
fi

if [ -t 1 ]; then
    G=$'\033[32m' R=$'\033[31m' Y=$'\033[33m' N=$'\033[0m'
else
    G='' R='' Y='' N=''
fi

# A fixed environment keeps the output identical on every machine.
# Each test gets a fresh, empty home directory, and new files always get the
# same permissions (CI runners and desktops use different umasks).
TEST_HOME=/tmp/trsh-home
umask 022

run_trsh() {
    rm -rf "$TEST_HOME" && mkdir -p "$TEST_HOME"
    env -u OLDPWD HOME="$TEST_HOME" LC_ALL=C NO_COLOR=1 TRSH="$TRSH" timeout 10 "$TRSH"
}

pass=0 fail=0 updated=0
diff_file=$(mktemp)
trap 'rm -f "$diff_file"; rm -rf "$TEST_HOME"' EXIT
cd "$ROOT" || exit 2

for in_file in "$CASES"/*.in; do
    name=$(basename "$in_file" .in)
    [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]] && continue

    out_file="$CASES/$name.out"
    sed_file="$CASES/$name.sed"
    actual=$(run_trsh < "$in_file" 2>&1; echo "[exit $?]")
    if [ -f "$sed_file" ]; then
        actual=$(printf '%s\n' "$actual" | sed -E -f "$sed_file")
    fi

    if [ "${UPDATE:-0}" = 1 ]; then
        printf '%s\n' "$actual" > "$out_file"
        echo "${Y}UPDATED${N} $name"
        updated=$((updated + 1))
        continue
    fi

    if [ ! -f "$out_file" ]; then
        echo "${R}MISSING${N} $name (no $name.out)"
        fail=$((fail + 1))
    elif diff -u "$out_file" <(printf '%s\n' "$actual") > "$diff_file"; then
        echo "${G}PASS${N}    $name"
        pass=$((pass + 1))
    else
        echo "${R}FAIL${N}    $name"
        sed 's/^/        /' "$diff_file"
        # On GitHub Actions, also report the failure as an annotation.
        if [ -n "${GITHUB_ACTIONS:-}" ]; then
            body=$(head -n 30 "$diff_file" | sed -e 's/%/%25/g' | awk '{printf "%s%%0A", $0}')
            echo "::error file=tests/cases/$name.in,title=Test $name failed::$body"
        fi
        fail=$((fail + 1))
    fi
done

if [ "${UPDATE:-0}" = 1 ]; then
    echo "$updated expected outputs updated."
    exit 0
fi

echo
echo "$pass passed, $fail failed."
[ "$fail" -eq 0 ]
