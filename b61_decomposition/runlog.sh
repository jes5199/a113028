#!/bin/bash
# usage: runlog.sh <tag> <logfile> <command...>
TAG="$1"; LOG="$2"; shift 2
LEDGER="$(dirname "$LOG")/RUNS.jsonl"
printf '{"ev":"START","tag":"%s","pid":%d,"t":"%s","cmd":"%s"}\n' "$TAG" "$$" "$(date -u +%FT%TZ)" "$*" >> "$LEDGER"
"$@" > "$LOG" 2>&1
rc=$?
printf '{"ev":"END","tag":"%s","pid":%d,"t":"%s","rc":%d}\n' "$TAG" "$$" "$(date -u +%FT%TZ)" "$rc" >> "$LEDGER"
