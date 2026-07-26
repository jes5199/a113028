#!/usr/bin/env python3
"""Fail if a base's status disagrees between README.md and FRONTIER-STATUS.md.

Built after three instances of the same failure in two days: a correction
reached the rows it was about and missed the neighbours (LESSONS 13d). Each
time the surviving stale claim sat in a TABLE, which is scanned for values
rather than re-read for claims -- and twice it was the strongest possible form
of the retracted statement ("no such width exists", "no value known").

Run: python3 check_ladder_consistency.py   (exit 1 on any disagreement)
"""
import re, sys

def statuses(path, pat):
    out = {}
    for line in open(path, encoding='utf-8'):
        m = pat.match(line)
        if not m:
            continue
        b = int(m.group(1))
        rest = m.group(2)
        lab = None
        low = rest.lower()
        # Strip negated forms first: "NOT CERTIFIED" must not read as CERTIFIED.
        # This bit the checker on its own first run -- see LESSONS 13d.
        for neg in ("not certified", "no second engine", "not strong"):
            low = low.replace(neg, "")
        for key in ("CERTIFIED", "STRONG", "WEAK", "NO-VALUE", "VERIFIED LOWER BOUND",
                    "INCONCLUSIVE", "UNREACHABLE", "NO VALUE KNOWN"):
            if key.lower() in low:
                lab = key
                break
        out[b] = lab
    return out

row = re.compile(r'^\|\s*(\d{1,3})\s*\|(.*)$')
rd = statuses('README.md', row)
fr = statuses('FRONTIER-STATUS.md', row)

bad = []
for b in sorted(set(rd) & set(fr)):
    if not (2 <= b <= 89):
        continue
    if rd[b] and fr[b] and rd[b] != fr[b]:
        bad.append((b, rd[b], fr[b]))

if bad:
    print("LADDER INCONSISTENCY -- README and FRONTIER-STATUS disagree:")
    for b, a, c in bad:
        print(f"  base {b:>3}:  README={a:<22} FRONTIER={c}")
    print("\nA reader will cite whichever table they land on. Reconcile before committing.")
    sys.exit(1)
print(f"ladder consistent: {len(set(rd) & set(fr))} bases cross-checked, no disagreements")
