#!/bin/bash

if [ $# -ne 5 ]; then
    echo "Use: $0 <WORK_DIR> <SOLUTION_ID> <CONTEST_ID> <LANG> <PROBLEM_ID>"
    exit 1
fi

WORK_DIR=$1
SOLUTION_ID=$2
CONTEST_ID=$3
LANG=$4
PROBLEM_ID=$5

TMP_DIR="/tmp/$RANDOM"
ORIGIN_SOLUTION_PATH="$TMP_DIR/"

mkdir -p "$TMP_DIR/problem/$PROBLEM_ID"

cp $WORK_DIR/data/contests/$CONTEST_ID/problem/$PROBLEM_ID/*$LANG $TMP_DIR/problem/$PROBLEM_ID/

cd $TMP_DIR/problem/$PROBLEM_ID
file_count=$(ls -1 * | wc -l)

if [ "$file_count" -eq 1 ]; then
    echo "0.xx,0.xx,0"
    rm -rf $TMP_DIR
    exit 0
fi

docker run --user root -v "$PWD:/dolos" --rm --entrypoint "/bin/sh" ghcr.io/dodona-edu/dolos-cli:2.7.1 -c "cd /dolos && dolos *$LANG" > $TMP_DIR/problem/$PROBLEM_ID/salida.txt

output=$(cat $TMP_DIR/problem/$PROBLEM_ID/salida.txt | grep $SOLUTION_ID | head -n 1 | awk '{print $1,$2,$3}')
IFS=' ' read -r first_param second_param third_param <<< "$output"

if [[ "$first_param" == *"$SOLUTION_ID"* ]]; then
    found_param="$first_param"
    other_param="$second_param"
else
    found_param="$second_param"
    other_param="$first_param"
fi

if [ -z "$third_param" ]; then
    similarity_score=$(grep -o 'Similarity score: [0-9]*' "$TMP_DIR/problem/$PROBLEM_ID/salida.txt" | awk '{print $3}')
    third_param=$similarity_score
fi

result="$found_param,$other_param,$third_param"
echo $result
rm -rf $TMP_DIR
exit 0
