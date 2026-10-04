#!/bin/bash
# Self-check for anti_cheating.sh and contest_similarity.sh. Needs dolos in PATH: ./test.sh
set -euo pipefail

script="$(cd "$(dirname "$0")" && pwd)/anti_cheating.sh"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
dir="$work/data/contests/7/problem/1000"
mkdir -p "$dir"

cat > "$dir/100.cc" <<'SRC'
#include <iostream>
#include <vector>
using namespace std;
long long solve(vector<int>& a){ long long s=0; for(size_t i=0;i<a.size();i++){ if(a[i]%2==0) s+=a[i]; else s-=a[i]; } return s; }
int main(){ int n; cin>>n; vector<int> a(n); for(int i=0;i<n;i++) cin>>a[i]; cout<<solve(a)<<endl; int best=0; for(int i=0;i<n;i++) for(int j=i+1;j<n;j++) if(a[i]+a[j]>best) best=a[i]+a[j]; cout<<best<<endl; return 0; }
SRC
cp "$dir/100.cc" "$dir/1001.cc"                                # same author resubmits
sed 's/solve/calc/g; s/best/mx/g' "$dir/100.cc" > "$dir/200.cc" # another user copies it
cat > "$dir/300.cc" <<'SRC'
#include <cstdio>
#include <map>
#include <string>
int main(){ std::map<std::string,int> m; char b[64]; while(scanf("%63s",b)==1){ m[b]++; } for(auto&p:m){ printf("%s %d\n",p.first.c_str(),p.second);} int t=0; for(auto&p:m) t+=p.second*2; printf("%d\n",t); return 0; }
SRC

check() {
    local expected=$1 actual
    shift
    actual=$("$script" "$work" "$@")
    [ "$actual" = "$expected" ] || { echo "FAIL [$*]: expected '$expected', got '$actual'"; exit 1; }
}

check "100.cc,200.cc,1" 100 7 .cc 1000 "1001,"   # own resubmission ignored, copy by another user found
check "0.xx,0.xx,0"     100 7 .cc 1000 "1001,200," # nobody else left to compare with
check "0.xx,0.xx,0"     300 7 .cc 1000 ""        # original code, id 300 must not match by substring
check "1001.cc,200.cc,1" 1001 7 .cc 1000 "100,"

# Whole contest run, with mysql replaced by a stub that serves the accepted list and logs the writes.
cp "$dir/100.cc" "$dir/400.cc" # no longer accepted: must be left out
mkdir -p "$work/etc" "$work/bin"
printf 'OJ_HOST_NAME=db\nOJ_USER_NAME=u\nOJ_PASSWORD=p\nOJ_DB_NAME=jol\nOJ_PORT_NUMBER=3306\n' > "$work/etc/judge.conf"
printf '100\talice\t1000\n1001\talice\t1000\n200\tbob\t1000\n300\tcarol\t1000\n' > "$work/accepted.tsv"
cat > "$work/bin/mysql" <<STUB
#!/bin/bash
query="\${@: -1}"
case "\$query" in SELECT*) cat "$work/accepted.tsv" ;; *) echo "\$query" >> "$work/sql.log" ;; esac
STUB
chmod +x "$work/bin/mysql"
touch "$work/data/contests/7/similarity.request"
PATH="$work/bin:$PATH" "$(dirname "$script")/contest_similarity.sh" "$work"

[ ! -e "$work/data/contests/7/similarity.request" ] || { echo "FAIL: request not removed"; exit 1; }
grep -q '^DELETE sc FROM similar_code .*contest_id=7$' "$work/sql.log" || { echo "FAIL: previous rows not deleted"; exit 1; }
actual=$(grep -o '([0-9,]*)' "$work/sql.log" | sort | tr '\n' ' ')
expected="(100,200,100) (1001,200,100) (200,100,100) "
[ "$actual" = "$expected" ] || { echo "FAIL contest run: expected '$expected', got '$actual'"; exit 1; }
echo OK
