#!/bin/bash
cd /home/jes/a113028
OUT=resume_gate_results.txt
echo "=== resume gates started $(date -u +%FT%TZ) binary=new9 ===" >> "$OUT"
# G1: b60 fresh regression
( ulimit -v 6000000; exec nice -n 19 timeout 900 ./carrytrie_cert.new9 certbb 60 5,10,15,20,24,25,30,35,40,45,50,55 3000000 ) > g1_b60_fresh.log 2>&1
echo "G1 b60 fresh: rc=$? $(grep -oE 'CERTIFIED.*|INCOMPLETE.*' g1_b60_fresh.log | tail -1) val=$(grep -o 'incumbent ([0-9]* decimal digits): [0-9]*' g1_b60_fresh.log | tail -1)" >> "$OUT"
# G2 session 1: b56 forced interrupt (cap 120)
( ulimit -v 6000000; exec env CERTBB_CAP_S=120 nice -n 19 timeout 900 ./carrytrie_cert.new9 certbb 56 8,16,24,32,40,48,52 3000000 ) > g2_s1.log 2>&1
echo "G2 s1 (cap120): rc=$? $(grep -oE 'INCOMPLETE.*|CERTIFIED.*' g2_s1.log | tail -1) manifest=$(wc -l < certbb_56_manifest.jsonl 2>/dev/null)" >> "$OUT"
# G2 session 2: resume to completion
( ulimit -v 6000000; exec nice -n 19 timeout 3600 ./carrytrie_cert.new9 certbb 56 8,16,24,32,40,48,52 3000000 resume ) > g2_s2.log 2>&1
echo "G2 s2 (resume): rc=$? $(grep -o 'resume: loaded.*' g2_s2.log | tail -1)" >> "$OUT"
echo "G2 s2 verdict: $(grep -oE 'CERTIFIED.*|INCOMPLETE.*' g2_s2.log | tail -1) val=$(grep -o 'incumbent ([0-9]* decimal digits): [0-9]*' g2_s2.log | tail -1)" >> "$OUT"
# G3: b63 smoke 600s resume on v1 manifest
( ulimit -v 6000000; exec env CERTBB_CAP_S=600 nice -n 19 timeout 1200 ./carrytrie_cert.new9 certbb 63 9,18,27,28,36,45,54 3000000 resume ) > g3_b63_smoke.log 2>&1
echo "G3 b63 smoke: rc=$? $(grep -o 'resume: loaded.*' g3_b63_smoke.log | tail -1)" >> "$OUT"
echo "G3 verdict: $(grep -oE 'CERTIFIED.*|INCOMPLETE.*' g3_b63_smoke.log | tail -1) $(grep -o 'DFS DONE.*' g3_b63_smoke.log | tail -1)" >> "$OUT"
echo "=== resume gates done $(date -u +%FT%TZ) ===" >> "$OUT"
