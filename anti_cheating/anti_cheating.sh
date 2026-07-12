#!/bin/bash
set -euo pipefail

if [ $# -ne 5 ]; then
    echo "Use: $0 <WORK_DIR> <SOLUTION_ID> <CONTEST_ID> <LANG> <PROBLEM_ID>"
    exit 1
fi

WORK_DIR=$1
SOLUTION_ID=$2
CONTEST_ID=$3
LANG=$4
PROBLEM_ID=$5

TMP_DIR=$(mktemp -d /tmp/patito-anticheat.XXXXXX)
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

PROBLEM_TMP_DIR="$TMP_DIR/problem/$PROBLEM_ID"
mkdir -p "$PROBLEM_TMP_DIR"

SOURCE_DIR="$WORK_DIR/data/contests/$CONTEST_ID/problem/$PROBLEM_ID"
shopt -s nullglob
files=("$SOURCE_DIR"/*"$LANG")

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

output=$(grep -- "$SOLUTION_ID" salida.txt | head -n 1 | awk '{print $1,$2,$3}' || true)
IFS=' ' read -r first_param second_param third_param <<< "$output"

if [[ "${first_param:-}" == *"$SOLUTION_ID"* ]]; then
    found_param=$first_param
    other_param=${second_param:-0}
else
    found_param=${second_param:-0}
    other_param=${first_param:-0}
fi

if [ -z "${third_param:-}" ]; then
    third_param=$(grep -o 'Similarity score: [0-9]*' salida.txt | awk '{print $3}' | head -n 1 || true)
fi

third_param=${third_param:-0}
printf '%s,%s,%s\n' "$found_param" "$other_param" "$third_param"
