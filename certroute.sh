#!/bin/bash
# 900s timeout -> Sol-router (jes tg#7603, boss #9307).
# Runs certauto under the 900s cap and classifies the outcome for routing:
#   rc=0   -> CERTIFIED/PASS (clean cert; no routing needed)
#   rc=4   -> WEAK (run COMPLETED with refute-descend result; band-depth route)
#   rc=3   -> DECLINED (admission planner; memory/plan route)
#   rc=124 -> TRIP. Classify from the log:
#             refute-descend already engaged (>1 subset scan) -> BAND-DEPTH route
#             still mid-first-subset -> ONE 2x rerun (1800s);
#               still stuck -> PLANNER-CALIBRATION route
# Partial verdicts always preserved: any FOUND value seen is recorded as the
# best-known lower bound; a tripped base is routed, never lost.
# Usage: certroute.sh <base> [expectedDecimal|-] [CERTPOS] [SUBSETENUM]
cd /home/jes/a113028
b=$1; exp=${2:--}; W=${3:-}; ENUM=${4:-}
OUT=routing_verdicts.txt
envs=()
[ -n "$W" ] && envs+=("CERTPOS=$W")
[ -n "$ENUM" ] && envs+=("SUBSETENUM=$ENUM")

run_once() { # $1=timeout $2=logsuffix
  local log="b${b}_route$2.log"
  ( ulimit -v 6000000; exec env "${envs[@]}" nice -n 19 timeout "$1" ./carrytrie_cert certauto "$b" "$exp" 3000000 ) > "$log" 2>&1
  echo $?
}

best_value() { grep -o "maximum value ([0-9]* digits): [0-9]*" "b${b}_route$1.log" 2>/dev/null | tail -1; }
scans_engaged() { [ "$(grep -c 'predicted-vs-actual' "b${b}_route$1.log" 2>/dev/null)" -gt 1 ]; }

rc=$(run_once 900 "")
case $rc in
  0) echo "b$b: CLEAN-CERT under 900s $(best_value '') [no route]" >> "$OUT" ;;
  4) echo "b$b: WEAK (completed, window-bounded) $(best_value '') [route: BAND-DEPTH deep-ladder]" >> "$OUT" ;;
  3) echo "b$b: DECLINED by admission planner [route: memory/plan]" >> "$OUT" ;;
  124)
    if scans_engaged ""; then
      echo "b$b: TRIP@900s, refute-descend engaged $(best_value '') [route: BAND-DEPTH deep-ladder]" >> "$OUT"
    else
      rc2=$(run_once 1800 "_2x")
      case $rc2 in
        0) echo "b$b: CLEAN-CERT on 2x rerun (900<t<=1800s) $(best_value '_2x') [no route; note: slow-legit]" >> "$OUT" ;;
        4) echo "b$b: WEAK on 2x rerun $(best_value '_2x') [route: BAND-DEPTH deep-ladder]" >> "$OUT" ;;
        *) echo "b$b: STUCK mid-first-subset at 2x (rc=$rc2) $(best_value '_2x') [route: PLANNER-CALIBRATION]" >> "$OUT" ;;
      esac
    fi ;;
  *) echo "b$b: rc=$rc anomaly $(best_value '') [route: investigate]" >> "$OUT" ;;
esac
