#!/bin/bash
cd /home/jes/a113028
OUT=g1_gate.txt
for spec in "58 28,29" "60 5,10,15,20,24,25,30,35,40,45,50,55"; do
  set -- $spec
  b=$1; drops=$2
  s0=$(date +%s)
  ( ulimit -v 6000000; exec nice -n 19 timeout 5400 ./carrytrie_cert.new8 certbb "$b" "$drops" 3000000 ) > "b${b}_certbb.log" 2>&1
  rc=$?
  s1=$(date +%s)
  echo "b$b: rc=$rc wall=$((s1-s0))s $(grep -o 'DFS DONE.*' b${b}_certbb.log | tail -1) $(grep -o 'incumbent ([0-9]* decimal digits): [0-9]*' b${b}_certbb.log | tail -1) $(grep -oE 'CERTIFIED \(zero unfinished branches\)|INCOMPLETE.*' b${b}_certbb.log | tail -1)" >> "$OUT"
done
echo "=== g1 rest done $(date -u +%FT%TZ) ===" >> "$OUT"
