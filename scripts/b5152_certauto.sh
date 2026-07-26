#!/bin/bash
# Second-method certauto runs for b51 (v4-scan value in hand) and b52 (v15
# candidate in hand). Self-check against the first-method values.
cd /home/jes/a113028
OUT=b5152_certauto_verdicts.txt
echo "=== certauto b51/b52 second-method runs started $(date -u +%FT%TZ) ===" >> "$OUT"
nice -n 19 timeout 7200 ./carrytrie_cert certauto 51 180145958793036691752603389680297418249529280174180771266050386968626019795153600 3000000 > b51_certauto.log 2>&1
echo "b51: rc=$? $(grep -oE 'CERTIFICATION.*|STRONG CANDIDATE.*' b51_certauto.log | tail -1)" >> "$OUT"
nice -n 19 timeout 7200 ./carrytrie_cert certauto 52 448735208793063714451606009674691709006633117645639135533102744646118644150575200 3000000 > b52_certauto.log 2>&1
echo "b52: rc=$? $(grep -oE 'CERTIFICATION.*|STRONG CANDIDATE.*' b52_certauto.log | tail -1)" >> "$OUT"
echo "=== finished $(date -u +%FT%TZ) ===" >> "$OUT"
