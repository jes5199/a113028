#!/bin/bash
cd /home/jes/a113028
OUT=g34_probes.txt
for b in 54 61; do
  s0=$(date +%s)
  ( ulimit -v 6000000; exec env GROUPDP=active CERTPOS=21 nice -n 19 timeout 2400 ./carrytrie_cert.new6 certauto "$b" - 3000000 ) > "b${b}_groupdp_probe.log" 2>&1
  rc=$?
  s1=$(date +%s)
  echo "b$b: rc=$rc wall=$((s1-s0))s $(grep -o 'FOUND with [^[]*' b${b}_groupdp_probe.log | tail -1 | head -c 120) $(grep -cE 'GROUPDP-CONTRADICTION' b${b}_groupdp_probe.log) contradictions $(grep -o 'site1_active_filtered=[0-9]*' b${b}_groupdp_probe.log | tail -1)" >> "$OUT"
done
echo "=== g34 done $(date -u +%FT%TZ) ===" >> "$OUT"
