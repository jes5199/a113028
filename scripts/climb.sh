#!/bin/bash
# Frontier climb (jes tg#7568): b53 upward, one base at a time, STOP at the
# first base that isn't a clean sub-hour certauto PASS — the hardness onset
# is the deliverable. Verdicts appended per base to climb_verdicts.txt.
# rc=0+PASS → continue; rc=4 refute-and-descend, rc=3 declined, rc=124
# timeout, anything else → STOP and record why.
cd /home/jes/a113028
OUT=climb_verdicts.txt
echo "=== climb started $(date -u +%FT%TZ) engine=carrytrie_cert (certauto) ===" >> "$OUT"
for b in 53 54 55 56 57 58 59 60 61 62 63 64; do
  log="b${b}_certauto.log"
  s0=$(date +%s)
  ( ulimit -v 6000000; exec nice -n 19 timeout 3600 ./carrytrie_cert certauto "$b" - 3000000 ) > "$log" 2>&1
  rc=$?
  s1=$(date +%s)
  found=$(grep -o "FOUND with |D|=[0-9]*[^[]*" "$log" | tail -1 | tr -d '\n')
  val=$(grep -o "maximum value ([0-9]* digits): [0-9]*" "$log" | tail -1)
  if [ $rc -eq 0 ] && grep -q "CERTIFICATION (direct-verified" "$log"; then
    echo "b$b: PASS $((s1-s0))s $found $val" >> "$OUT"
  else
    case $rc in
      124) why="TIMEOUT >3600s — HARDNESS ONSET (wall)";;
      4)   why="STRONG-CANDIDATE exit (refute-and-descend engaged) — soundness-sensitive, needs review";;
      3)   why="DECLINED by admission planner (memory/plan infeasible)";;
      *)   why="rc=$rc anomaly";;
    esac
    echo "b$b: STOP $((s1-s0))s $why $val" >> "$OUT"
    echo "=== climb STOPPED at b$b $(date -u +%FT%TZ) ===" >> "$OUT"
    exit 0
  fi
done
echo "=== climb finished b64 (mask ceiling) $(date -u +%FT%TZ) ===" >> "$OUT"
