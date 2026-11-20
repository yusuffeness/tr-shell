#!/usr/bin/env bash
#
# tr-shell behaviour tests.
#
# Each tests/cases/NAME.in is fed to trsh on stdin. Its stdout and stderr
# are merged, "[exit N]" is appended, and the result is compared with
# tests/cases/NAME.out.
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
# Each test gets a fresh, empty home directory.
TEST_HOME=/tmp/trsh-home

run_trsh() {
    rm -rf "$TEST_HOME" && mkdir -p "$TEST_HOME"
    env -u OLDPWD HOME="$TEST_HOME" LC_ALL=C NO_COLOR=1 timeout 10 "$TRSH"
}

pass=0 fail=0 updated=0
diff_file=$(mktemp)
trap 'rm -f "$diff_file"; rm -rf "$TEST_HOME"' EXIT
cd "$ROOT" || exit 2

for in_file in "$CASES"/*.in; do
    name=$(basename "$in_file" .in)
    [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]] && continue

    out_file="$CASES/$name.out"
    actual=$(run_trsh < "$in_file" 2>&1; echo "[exit $?]")

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
