#!/bin/bash
# Bucket-engine (carrytrie cert mode) sweep b41-b49, each self-checked
# against its known answer. Sequential, nice 19, per-base 1800s external cap
# (engine also has internal 1800s warn / 5400s abort + 3GB RSS budget).
# One-line verdicts appended to bucket_scorecard.txt as each base lands.
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
echo "=== bucket sweep (re)started $(date -u +%FT%TZ) engine=carrytrie_cert (cert mode) ulimit-v=6GB backstop ===" >> "$SC"
for b in 42 43 44 45 46 47 48 49; do
  out="bucket_b${b}.log"
  start=$(date +%s.%N)
  # ulimit -v backstop: the engine's 3GB RSS self-abort fires only at
  # checkpoints (b41 ballooned to 8.9GB before the post-generation check);
  # the address-space cap makes allocation fail instead of pressuring hermes.
  ( ulimit -v 6000000; exec nice -n 19 timeout 1800 ./carrytrie_cert cert "$b" "${KNOWN[$b]}" 3000000 ) > "$out" 2>&1
  rc=$?
  end=$(date +%s.%N)
  wall=$(echo "$end $start" | awk '{printf "%.2f", $1-$2}')
  verdict=$(grep -o "CERTIFICATION.*" "$out" | tail -1)
  found=$(grep -o "total autonomous-search wall=[0-9.]*s" "$out" | tail -1)
  if [ $rc -eq 124 ]; then
    echo "b$b: TIMEOUT >1800s (external cap) rc=124" >> "$SC"
  elif [ -n "$verdict" ]; then
    echo "b$b: $verdict | $found | script-wall=${wall}s rc=$rc" >> "$SC"
  else
    lastline=$(tail -2 "$out" | tr '\n' ' ')
    echo "b$b: NO-CERT rc=$rc wall=${wall}s last: $lastline" >> "$SC"
  fi
done
echo "=== bucket sweep finished $(date -u +%FT%TZ) ===" >> "$SC"
