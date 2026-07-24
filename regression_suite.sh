#!/bin/bash
# BAND-DEPTH-CERTIFICATION.md Phase 1B regression harness.
#
# Runs the 6 known-good CERTIFIED bases in their CERTIFIED configurations,
# each nice -n 19 with a generous timeout. Pass criteria per base (review
# note C1, NOT the doc body's byte-identical-counts): exit 0 + CERTIFICATION
# line + char-exact decimal value. subsetsScanned/subsetsChecked and wall
# time are recorded as INFORMATIONAL columns only (may legitimately change
# across code revisions per C1) in regression_baseline.txt.
#
# Usage: ./regression_suite.sh [binary] [label]
#   binary : carrytrie binary to test (default ./carrytrie_cert.new5)
#   label  : tag written into regression_baseline.txt rows (default "run")
#
# Everything here is read-only w.r.t. the binary; never touches
# ./carrytrie_cert. All runs sequential (no parallel builds/runs), nice -19.

set -u
cd "$(dirname "$0")"

BIN="${1:-./carrytrie_cert.new5}"
LABEL="${2:-run}"
OUTDIR="regression_logs"
mkdir -p "$OUTDIR"
BASELINE_FILE="regression_baseline.txt"

if [ ! -x "$BIN" ]; then
    echo "FATAL: binary $BIN not found or not executable" >&2
    exit 1
fi

# base : certpos_env : subsetenum_env : timeout_seconds : expected_decimal
BASES_50=50; EXP_50="71024679959360285134972854735006247396917902186058529912810323948019000719382560"
BASES_51=51; EXP_51="180145958793036691752603389680297418249529280174180771266050386968626019795153600"
BASES_52=52; EXP_52="448735208793063714451606009674691709006633117645639135533102744646118644150575200"
BASES_56=56; EXP_56="818241795167031571216125942108524047248252714836991418093245122512858064867506776900"
BASES_58=58; EXP_58="9736569208044009047215982390641931224449891752961186859894784030413187509184301962164357195502400"
BASES_60=60; EXP_60="374096795553866901593729675694543662806191148614834420420657978999648347496776816352"

PASS_COUNT=0
FAIL_COUNT=0
TS=$(date -u +%FT%TZ)

echo "=== regression_suite.sh run label=$LABEL binary=$BIN started $TS ===" | tee -a "$BASELINE_FILE"

run_one() {
    local base="$1" certpos="$2" subsetenum="$3" tmo="$4" expected="$5"
    local logf="$OUTDIR/b${base}_${LABEL}.log"
    local env_prefix=""
    [ -n "$certpos" ] && env_prefix="CERTPOS=$certpos "
    [ -n "$subsetenum" ] && env_prefix="${env_prefix}SUBSETENUM=$subsetenum "

    echo "--- b${base}: ${env_prefix}nice -19 timeout ${tmo}s $BIN certauto $base <expected> 3000000 ---"
    local t0 t1 wall
    t0=$(date +%s.%N)
    if [ -n "$certpos" ] && [ -n "$subsetenum" ]; then
        nice -n 19 timeout "$tmo" env CERTPOS="$certpos" SUBSETENUM="$subsetenum" "$BIN" certauto "$base" "$expected" 3000000 > "$logf" 2>&1
    elif [ -n "$certpos" ]; then
        nice -n 19 timeout "$tmo" env CERTPOS="$certpos" "$BIN" certauto "$base" "$expected" 3000000 > "$logf" 2>&1
    else
        nice -n 19 timeout "$tmo" "$BIN" certauto "$base" "$expected" 3000000 > "$logf" 2>&1
    fi
    local rc=$?
    t1=$(date +%s.%N)
    wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f", b-a}')

    local certline value subsetsInfo
    certline=$(grep -oE 'CERTIFICATION.*' "$logf" | tail -1)
    value=$(grep -oE 'maximum value \([0-9]+ digits\): [0-9]+' "$logf" | tail -1 | awk '{print $NF}')
    subsetsInfo=$(grep -oE '\[subsetsChecked=[0-9]+ subsetsScanned=[0-9]+\]' "$logf" | tail -1)

    local status
    if [ "$rc" -eq 0 ] && [ -n "$certline" ] && [ "$value" = "$expected" ]; then
        status="PASS"
        PASS_COUNT=$((PASS_COUNT+1))
    else
        status="FAIL"
        FAIL_COUNT=$((FAIL_COUNT+1))
    fi

    echo "b${base}: label=$LABEL status=$status rc=$rc wall=${wall}s value_match=$([ "$value" = "$expected" ] && echo yes || echo no) $subsetsInfo certline=\"$certline\" log=$logf" | tee -a "$BASELINE_FILE"
}

run_one "$BASES_50" "" "" 1800 "$EXP_50"
run_one "$BASES_51" "" "" 1800 "$EXP_51"
run_one "$BASES_52" "" "" 1800 "$EXP_52"
run_one "$BASES_56" "21" "" 3600 "$EXP_56"
run_one "$BASES_58" "21" "" 3600 "$EXP_58"
run_one "$BASES_60" "21" "new" 3600 "$EXP_60"

TS2=$(date -u +%FT%TZ)
echo "=== regression_suite.sh run label=$LABEL finished $TS2: PASS=$PASS_COUNT FAIL=$FAIL_COUNT ===" | tee -a "$BASELINE_FILE"

if [ "$FAIL_COUNT" -ne 0 ]; then
    exit 1
fi
exit 0
