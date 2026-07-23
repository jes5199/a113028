#!/bin/bash
# Fast-mode no-stop climb b55->b64 (jes tg#7572): quick certauto verdict per
# base, honest label, NO halting at hard bases, NO deep window escalation.
cd /home/jes/a113028
OUT=climb_verdicts.txt
echo "=== fast-mode no-stop climb b55->b64 started $(date -u +%FT%TZ) ===" >> "$OUT"
for b in 55 56 57 58 59 60 61 62 63 64; do
  log="b${b}_certauto.log"
  s0=$(date +%s)
  ( ulimit -v 6000000; exec nice -n 19 timeout 3600 ./carrytrie_cert certauto "$b" - 3000000 ) > "$log" 2>&1
  rc=$?
  s1=$(date +%s)
  found=$(grep -o "FOUND with |D|=[0-9]*[^[]*" "$log" | tail -1 | tr -d '\n')
  val=$(grep -o "maximum value ([0-9]* digits): [0-9]*" "$log" | tail -1)
  if [ $rc -eq 0 ] && grep -q "CERTIFICATION (direct-verified" "$log"; then
    echo "b$b: STRONG(single-exhaustive) $((s1-s0))s $found $val" >> "$OUT"
  elif [ $rc -eq 4 ]; then
    echo "b$b: WEAK(refute-descend, window-bounded) $((s1-s0))s $found $val" >> "$OUT"
  elif [ $rc -eq 124 ]; then
    echo "b$b: NO-VALUE (timeout >3600s)" >> "$OUT"
  elif [ $rc -eq 3 ]; then
    echo "b$b: NO-VALUE (admission-declined)" >> "$OUT"
  else
    echo "b$b: NO-VALUE (rc=$rc anomaly) last: $(tail -1 "$log" | head -c 150)" >> "$OUT"
  fi
done
echo "=== fast-mode climb reached b64 ceiling $(date -u +%FT%TZ) ===" >> "$OUT"
