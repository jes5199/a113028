#!/bin/bash
# Phase C escalating-window probe of b50's forced 47-digit subset
# (boss-endorsed plan, msg 9236/9237). Each width re-runs certauto 50 with a
# wider divergence window; a refutation at width W covers m* slack up to
# (W-20). Any |D|=47 find = SUPERSEDE, stop immediately.
cd /home/jes/a113028
OUT=b50_certpos_escalation.txt
echo "=== b50 CERTPOS escalation started $(date -u +%FT%TZ) ===" >> "$OUT"
for W in 21 22 23; do
  case $W in 21) cap=3600;; 22) cap=7200;; 23) cap=14400;; esac
  log="b50_certpos${W}.log"
  s0=$(date +%s)
  CERTPOS=$W nice -n 19 timeout $cap ./carrytrie_cert certauto 50 - 3000000 > "$log" 2>&1
  rc=$?
  s1=$(date +%s)
  found=$(grep -o "FOUND with |D|=[0-9]*" "$log" | tail -1)
  val=$(grep -o "maximum value.*" "$log" | tail -1)
  echo "W=$W rc=$rc wall=$((s1-s0))s $found $val" >> "$OUT"
  if grep -q "FOUND with |D|=47" "$log"; then
    echo "W=$W: *** 47-DIGIT COMPLETION FOUND — SUPERSEDES the 46-digit candidate; stopping escalation ***" >> "$OUT"
    break
  fi
done
echo "=== escalation finished $(date -u +%FT%TZ) ===" >> "$OUT"
