#!/bin/bash
# Recomputes code similarity for whole contests. judged runs it when the admin API leaves
# data/contests/<contest_id>/similarity.request: the previous similar_code rows of that contest
# are deleted and every accepted solution is compared with the other users' ones through Dolos.
# A failed run leaves similarity.error next to the request (time and reason) for the admin API;
# a clean run removes it.
# Use: contest_similarity.sh [OJ_HOME]
set -uo pipefail
shopt -s nullglob

OJ_HOME=${1:-/home/judge}
CONF="$OJ_HOME/etc/judge.conf"
THRESHOLD=0.7 # same cut as judge_client

conf() {
    sed -n "s/^$1[[:space:]]*=[[:space:]]*//p" "$CONF" | head -n 1 | sed 's/[[:space:]]*$//'
}

sql() {
    MYSQL_PWD=$(conf OJ_PASSWORD) mysql -N -B -h "$(conf OJ_HOST_NAME)" -P "$(conf OJ_PORT_NUMBER)" \
        -u "$(conf OJ_USER_NAME)" "$(conf OJ_DB_NAME)" -e "$1"
}

# Reasons a run failed, one per line; empty means it went fine.
fail() {
    echo "$1" >> "$tmp/errors"
}

run_contest() {
    local contest_id=$1 tmp=$2 problem_dir problem_id ext work id values

    command -v dolos > /dev/null || { fail "dolos no está instalado en el juez"; return 1; }
    sql "SELECT solution_id, user_id, problem_id FROM solution WHERE contest_id=$contest_id AND result=4" \
        > "$tmp/accepted.tsv" || { fail "no se pudieron leer los envíos aceptados"; return 1; }
    sql "DELETE sc FROM similar_code sc JOIN solution s ON s.solution_id=sc.solution_id WHERE s.contest_id=$contest_id" \
        || { fail "no se pudieron borrar las similitudes anteriores"; return 1; }

    # ponytail: only solutions whose source judge_client saved while OJ_SIM_ENABLE was on;
    # read source_code from the database if older contests must be covered.
    for problem_dir in "$OJ_HOME/data/contests/$contest_id/problem"/*/; do
        problem_id=$(basename "$problem_dir")
        for ext in $(ls "$problem_dir" | sed -n 's/^[0-9]*\.//p' | sort -u); do
            work="$tmp/$problem_id.$ext"
            mkdir -p "$work"
            # Files of solutions that are still accepted in this contest problem.
            while read -r id; do
                [ -f "$problem_dir$id.$ext" ] && cp "$problem_dir$id.$ext" "$work/"
            done < <(awk -F'\t' -v problem="$problem_id" '$3 == problem { print $1 }' "$tmp/accepted.tsv")
            [ "$(ls "$work" | wc -l)" -gt 1 ] || continue
            (cd "$work" && timeout 600s dolos run *."$ext") >> "$tmp/dolos.txt" \
                || fail "Dolos falló en el problema $problem_id (.$ext)"
        done
    done
    [ -s "$tmp/dolos.txt" ] || return 0

    # Best match of every solution against a different user.
    values=$(awk -v threshold="$THRESHOLD" '
        NR == FNR { split($0, field, "\t"); user[field[1]] = field[2]; next }
        $3 ~ /^[0-9.]+$/ {
            a = $1; b = $2
            sub(/\..*$/, "", a); sub(/\..*$/, "", b)
            if (!(a in user) || !(b in user) || user[a] == user[b]) next
            if ($3 + 0 > best[a]) { best[a] = $3 + 0; other[a] = b }
            if ($3 + 0 > best[b]) { best[b] = $3 + 0; other[b] = a }
        }
        END {
            for (id in best)
                if (best[id] >= threshold)
                    printf "%s(%d,%d,%d)", (count++ ? "," : ""), id, other[id], int(best[id] * 100 + 0.5)
        }' "$tmp/accepted.tsv" "$tmp/dolos.txt")
    [ -n "$values" ] || return 0
    sql "INSERT INTO similar_code (solution_id, similar_s_id, percentage) VALUES $values ON DUPLICATE KEY UPDATE percentage=VALUES(percentage)" \
        || fail "no se pudieron guardar las similitudes"
}

for request in "$OJ_HOME"/data/contests/*/similarity.request; do
    contest_id=$(basename "$(dirname "$request")")
    if [[ "$contest_id" =~ ^[0-9]+$ ]]; then
        tmp=$(mktemp -d /tmp/patito-similarity.XXXXXX)
        error_file="$(dirname "$request")/similarity.error"
        run_contest "$contest_id" "$tmp"
        if [ -s "$tmp/errors" ]; then
            { date '+%Y-%m-%d %H:%M:%S'; cat "$tmp/errors"; } > "$error_file"
            echo "contest similarity failed for contest $contest_id: $(paste -sd ';' "$tmp/errors")" >&2
        else
            rm -f "$error_file"
        fi
        rm -rf "$tmp"
    fi
    rm -f "$request"
done
