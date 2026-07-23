#!/bin/bash
# Climb continuation b60->b64 at CERTPOS=21 (decision 2026-07-23 ~15:45,
# boss msg 9269 left width to my call): the W=20 fast pass systematically
# undershoots on rough-B-1 bases past the b54 onset (3/3 WEAK verdicts were
# submaximal or non-monotone; every +1 probe found more), so W=21 IS the
# fast pass for the remaining bases. Same no-stop honest labeling.
cd /home/jes/a113028
OUT=climb_verdicts.txt
echo "=== climb resumed b60->b64 at CERTPOS=21 (width decision, see FRONTIER-STATUS) $(date -u +%FT%TZ) ===" >> "$OUT"
for b in 60 61 62 63 64; do
  log="b${b}_certauto_w21.log"
  s0=$(date +%s)
  ( ulimit -v 6000000; exec env CERTPOS=21 nice -n 19 timeout 5400 ./carrytrie_cert certauto "$b" - 3000000 ) > "$log" 2>&1
  rc=$?
  s1=$(date +%s)
  found=$(grep -o "FOUND with |D|=[0-9]*[^[]*" "$log" | tail -1 | tr -d '\n')
  val=$(grep -o "maximum value ([0-9]* digits): [0-9]*" "$log" | tail -1)
  if [ $rc -eq 0 ] && grep -q "CERTIFICATION (direct-verified" "$log"; then
    echo "b$b: CERTIFIED-CLEAN@W21 $((s1-s0))s $found $val" >> "$OUT"
  elif [ $rc -eq 4 ]; then
    echo "b$b: WEAK@W21(refute-descend) $((s1-s0))s $found $val" >> "$OUT"
  elif [ $rc -eq 124 ]; then
    echo "b$b: NO-VALUE (timeout >5400s @W21)" >> "$OUT"
  elif [ $rc -eq 3 ]; then
    echo "b$b: NO-VALUE (admission-declined @W21)" >> "$OUT"
  else
    echo "b$b: NO-VALUE (rc=$rc anomaly @W21) last: $(tail -1 "$log" | head -c 150)" >> "$OUT"
  fi
done
echo "=== climb reached b64 ceiling $(date -u +%FT%TZ) ===" >> "$OUT"
