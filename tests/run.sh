#!/usr/bin/env bash
#
# tr-shell davranış testleri.
#
# Her tests/cases/NAME.in dosyası trsh'ye girdi olarak verilir. Standart
# çıktı ve hata birleştirilip sonuna "[exit N]" eklenir ve
# tests/cases/NAME.out ile karşılaştırılır.
#
#   bash tests/run.sh             tüm testleri çalıştır
#   bash tests/run.sh cd          adında "cd" geçen testleri çalıştır
#   UPDATE=1 bash tests/run.sh    beklenen çıktıları yeniden yaz
#   TRSH=./trsh bash tests/run.sh başka bir ikili dosyayı test et

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
CASES="$ROOT/tests/cases"
TRSH=${TRSH:-"$ROOT/trsh"}
FILTER=${1:-}

if [ ! -x "$TRSH" ]; then
    echo "trsh bulunamadi: $TRSH (once 'make' calistirin)" >&2
    exit 2
fi

if [ -t 1 ]; then
    G=$'\033[32m' R=$'\033[31m' Y=$'\033[33m' N=$'\033[0m'
else
    G='' R='' Y='' N=''
fi

pass=0 fail=0 updated=0
cd "$ROOT" || exit 2

for in_file in "$CASES"/*.in; do
    name=$(basename "$in_file" .in)
    [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]] && continue

    out_file="$CASES/$name.out"
    actual=$(LC_ALL=C timeout 10 "$TRSH" < "$in_file" 2>&1; echo "[exit $?]")

    if [ "${UPDATE:-0}" = 1 ]; then
        printf '%s\n' "$actual" > "$out_file"
        echo "${Y}guncellendi${N} $name"
        updated=$((updated + 1))
        continue
    fi

    if [ ! -f "$out_file" ]; then
        echo "${R}EKSIK${N}  $name (beklenen cikti yok: $name.out)"
        fail=$((fail + 1))
    elif diff -u "$out_file" <(printf '%s\n' "$actual") > /tmp/trsh_diff.$$; then
        echo "${G}GECTI${N}  $name"
        pass=$((pass + 1))
    else
        echo "${R}KALDI${N}  $name"
        sed 's/^/        /' /tmp/trsh_diff.$$
        fail=$((fail + 1))
    fi
done
rm -f /tmp/trsh_diff.$$

if [ "${UPDATE:-0}" = 1 ]; then
    echo "$updated test guncellendi."
    exit 0
fi

echo
echo "$pass gecti, $fail kaldi."
[ "$fail" -eq 0 ]
