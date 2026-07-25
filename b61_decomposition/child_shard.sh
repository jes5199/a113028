#!/bin/bash
# Exact partition of b61's W=23 terminal into 23 W=22 children.
# Any hit ends everything: touch b61.FOUND, all shards see it and stop.
sh=$1; N=$2; n=0
while read PFX; do
  n=$((n+1)); [ $(( (n-1) % N )) -ne "$sh" ] && continue
  [ -f b61.FOUND ] && { echo "shard$sh stopping: another child found a completion"; exit 0; }
  ./runlog.sh "b61_child$n" "child$n.log" env CERTSET_W=22 CERTSET_PREFIX="$PFX" timeout 7200 nice -n 19 ./carrytrie_cert.new20 certset 61 "30" 3000000
  rc=$?
  V=$(grep -oE "FOUND:.*|maximum value \([0-9]+ digits\): [0-9]+|REFUTED at the heuristic prefix \(wall=[0-9.]+s\)|DECLINED.*" child$n.log | head -2 | tr '\n' ' ')
  echo "child$n rc=$rc $(date -u +%H:%M:%S) :: $V"
  grep -q "FOUND:" child$n.log && touch b61.FOUND
done < children.txt
echo "=== SHARD $sh COMPLETE $(date -u +%H:%M:%S) ==="
