#!/bin/bash
# Bucket-first, scan-fallback sweep b41-b49 (jes tg#7544 via boss-clod #9204).
# Per base: try carrytrie cert (bucket join) under ulimit -v 6GB + 1800s cap;
# on ANY bucket failure (memory abort, ulimit kill, timeout), fall back to the
# proven low-memory v4 scan engine (3600s cap). Every scorecard row is labeled
# by the method that ACTUALLY produced the value — never a disguised swap.
# b41: bucket already honestly failed (recorded); scan-only backfill here.
cd /home/jes/a113028
SC=bucket_scorecard.txt
declare -A KNOWN=(
  [41]=791222981626154999235100158499550255615325307057668641337271200
  [42]=650828754201915243697436482327806321775960031977171416000
  [43]=9374765438594117074250580509957460813483601689280434597219126021600
  [44]=12428520662963836843722648681332672663852331283150932087854716800
  [45]=29858202121833974366127520253547500517971464443016809773917504800
  [46]=315044747190120671695735975284412123404260147529994283460952247723479200
  [47]=1754681573582232514378787438934811607312193893426436545338224933544695955200
  [48]=94237804886307950779486130179671488973571078333724597158459950718126090200
  [49]=27480664153312064994836939532520844560984511658005210290838348871641700981823200
)
SCAN_ONLY=" 41 "   # bucket verdict already recorded for these; go straight to scan

scan_fallback() {  # $1=base $2=reason-label
  local b=$1 reason=$2 slog="scan_b${b}.log"
  local s0=$(date +%s.%N)
  nice -n 19 timeout 3600 ./a113028_v4 "$b" "$b" 1 > "$slog" 2>&1
  local src=$?
  local s1=$(date +%s.%N)
  local swall=$(echo "$s1 $s0" | awk '{printf "%.1f", $1-$2}')
  local val=$(awk -v b="$b" '$1==b {print $2}' "$slog" | tail -1)
  if [ "$src" -eq 124 ]; then
    echo "b$b: scan fallback TIMEOUT >3600s ($reason)" >> "$SC"
  elif [ "$val" = "${KNOWN[$b]}" ]; then
    echo "b$b: scan fallback ${swall}s MATCH ($reason)" >> "$SC"
  else
    echo "b$b: scan fallback ${swall}s MISMATCH-OR-FAIL rc=$src val=${val:-none} ($reason)" >> "$SC"
  fi
}

echo "=== sweep v2 (bucket-first + scan-fallback) started $(date -u +%FT%TZ) ===" >> "$SC"
for b in 41 42 43 44 45 46 47 48 49; do
  if [[ "$SCAN_ONLY" == *" $b "* ]]; then
    scan_fallback "$b" "bucket OOM'd, recorded earlier"
    continue
  fi
  out="bucket_b${b}.log"
  start=$(date +%s.%N)
  ( ulimit -v 6000000; exec nice -n 19 timeout 1800 ./carrytrie_cert cert "$b" "${KNOWN[$b]}" 3000000 ) > "$out" 2>&1
  rc=$?
  end=$(date +%s.%N)
  wall=$(echo "$end $start" | awk '{printf "%.2f", $1-$2}')
  if grep -q "CERTIFICATION (matches known target): PASS" "$out"; then
    engwall=$(grep -o "total autonomous-search wall=[0-9.]*s" "$out" | tail -1)
    echo "b$b: bucket ${engwall#total autonomous-search wall=} CERTIFIED PASS (script-wall ${wall}s)" >> "$SC"
  else
    if [ "$rc" -eq 124 ]; then reason="bucket TIMEOUT >1800s"
    elif grep -q "exceeded budget" "$out"; then reason="bucket memory self-abort @${wall}s"
    elif [ "$rc" -ge 128 ] || grep -qi "bad_alloc\|std::bad_alloc" "$out"; then reason="bucket ulimit/alloc kill rc=$rc @${wall}s"
    else reason="bucket FAIL rc=$rc @${wall}s"
    fi
    scan_fallback "$b" "$reason"
  fi
done
echo "=== sweep v2 finished $(date -u +%FT%TZ) ===" >> "$SC"
