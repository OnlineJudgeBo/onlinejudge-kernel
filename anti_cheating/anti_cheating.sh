#!/bin/bash
set -euo pipefail

if [ $# -lt 5 ] || [ $# -gt 6 ]; then
    echo "Use: $0 <WORK_DIR> <SOLUTION_ID> <CONTEST_ID> <LANG> <PROBLEM_ID> [OWN_SOLUTION_IDS]"
    exit 1
fi

WORK_DIR=$1
SOLUTION_ID=$2
CONTEST_ID=$3
LANG=$4
PROBLEM_ID=$5
# Comma separated solution ids of the same user: a submission is never compared with its author's own code.
OWN_IDS=",${6:-},"

TMP_DIR=$(mktemp -d /tmp/patito-anticheat.XXXXXX)
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

PROBLEM_TMP_DIR="$TMP_DIR/problem/$PROBLEM_ID"
mkdir -p "$PROBLEM_TMP_DIR"

SOURCE_DIR="$WORK_DIR/data/contests/$CONTEST_ID/problem/$PROBLEM_ID"
shopt -s nullglob
files=()
for file in "$SOURCE_DIR"/*"$LANG"; do
    id=$(basename "$file" "$LANG")
    if [ "$id" = "$SOLUTION_ID" ] || [[ "$OWN_IDS" != *",$id,"* ]]; then
        files+=("$file")
    fi
done

if [ "${#files[@]}" -le 1 ]; then
    echo "0.xx,0.xx,0"
    exit 0
fi

cp -- "${files[@]}" "$PROBLEM_TMP_DIR/"
cd "$PROBLEM_TMP_DIR"

if command -v dolos >/dev/null 2>&1; then
    timeout 120s dolos run *"$LANG" > salida.txt || true
else
    docker run --user root -v "$PWD:/dolos" --rm --entrypoint "/bin/sh" ghcr.io/dodona-edu/dolos-cli:2.7.1 -c "cd /dolos && dolos *$LANG" > salida.txt || true
fi

# Best match among the rows that pair exactly this solution with another one.
awk -v self="$SOLUTION_ID$LANG" '
    ($1 == self || $2 == self) && $3 ~ /^[0-9.]+$/ && $3 + 0 > best {
        best = $3 + 0
        other = ($1 == self) ? $2 : $1
    }
    END {
        if (best > 0) printf "%s,%s,%s\n", self, other, best
        else print "0.xx,0.xx,0"
    }' salida.txt
