#!/bin/bash
# Absence is checkable: any START without a matching END is unfinished.
L="$(dirname "$0")/RUNS.jsonl"; [ -f "$L" ] || { echo "no ledger"; exit 0; }
python3 - "$L" << 'PY'
import json,sys,collections
st={};en={}
for line in open(sys.argv[1]):
    line=line.strip()
    if not line: continue
    r=json.loads(line)
    (st if r["ev"]=="START" else en)[r["tag"]]=r
open_=[t for t in st if t not in en]
print(f"runs started={len(st)} ended={len(en)} UNFINISHED={len(open_)}")
for t in open_: print("  OPEN:",t,st[t]["t"],st[t]["cmd"][:60])
for t,r in en.items():
    if r["rc"]!=0: print(f"  ended rc={r['rc']}: {t}")
PY
