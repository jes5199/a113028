#!/bin/bash
# Auto-widen certify (jes tg#7579): rerun certauto at increasing CERTPOS
# until it completes CLEAN (exit 0 = winning answer inside the window, no
# refute-and-descend) => genuinely CERTIFIED. exit 4 (window-bounded
# refute-descend) => widen +1 and retry. Engine caps CERTPOS at 24.
# COST CHECKPOINT (boss 9260): each +1 is ~9x; per-width cap 7200s — a
# width that times out (or W hitting 24 uncertified) STOPS this base with a
# "needs decision" line instead of grinding.
# Usage: autowiden.sh <base> [startW]   (default startW=21)
cd /home/jes/a113028
b=$1; W=${2:-21}
OUT=autowiden_verdicts.txt
while [ "$W" -le 24 ]; do
  log="b${b}_certpos${W}.log"
  s0=$(date +%s)
  ( ulimit -v 6000000; exec env CERTPOS=$W nice -n 19 timeout 7200 ./carrytrie_cert certauto "$b" - 3000000 ) > "$log" 2>&1
  rc=$?
  s1=$(date +%s)
  val=$(grep -o "maximum value ([0-9]* digits): [0-9]*" "$log" | tail -1)
  if [ $rc -eq 0 ] && grep -q "CERTIFICATION (direct-verified" "$log"; then
    echo "b$b: CERTIFIED-CLEAN at W=$W $((s1-s0))s $val" >> "$OUT"; exit 0
  elif [ $rc -eq 4 ]; then
    echo "b$b: W=$W still window-bounded ($((s1-s0))s) -> widening" >> "$OUT"
    W=$((W+1))
  elif [ $rc -eq 124 ]; then
    echo "b$b: NEEDS-DECISION — W=$W timed out >7200s (next width ~9x more); paused" >> "$OUT"; exit 0
  else
    echo "b$b: NEEDS-DECISION — W=$W rc=$rc (decline/anomaly); paused" >> "$OUT"; exit 0
  fi
done
echo "b$b: NEEDS-DECISION — uncertified at engine cap W=24; paused" >> "$OUT"
