#!/usr/bin/env python3
"""Faithful Python port of make_subset() from 2006-04-finite.c (staged
alongside this file; to be vendored under evidence/ together).

ATTRIBUTION — there were TWO 2006 programs, and this ports only one of
them: the APRIL C rewrite (finite.c, GMP). The February 2006 run log was
produced by a different program, a Ruby solver (the log's own "YARV
interpreter" line), whose selection behaviour this script cannot speak
for — notably, February's Ruby solved b6 while the April C fails it (see
the exception note below). Every result printed here is a statement about
the April C's selection, not about "2006" generally.

Purpose: make the claim "the 2006 digit-set selection picks the true answer
set at every solved base tested" reproducible rather than asserted.

Port semantics, matched to finite.c:
  - candidate sets are built descending-lex, with active[0] pinned to
    base-1 (finite.c:118 — the 2006 code never considers a set omitting
    the top digit);
  - "ten rule" (finite.c:147): a prefix whose lcm is divisible by the base
    kills the whole subtree (the C code's backup/slack walk is an
    iterative encoding of exactly this pruned DFS);
  - "nine rule" (finite.c:151): at the final position, digit sum must be
    ≡ 0 mod gcd(base-1, lcm) — note this is CRT-equivalent to the modern
    per-prime-power digit-sum gates (carrytrie.cpp:4512);
  - if no set of length L survives, recurse to L-1 (finite.c:131) —
    maximal-cardinality-first.

Validation gate (runs first; everything else is void if it fails): the
port must reproduce the b36 answer set from jes's 2006-02-20 run log
(zyxwvutsqponmlkjf586a4e2b13d7hc → drops {9,16,18,27}).

Then, for every solved base 36..64 present in b113028.txt, the port's pick
is compared against the digit set of the true answer a(B).

Usage: python3 make_subset_2006_port.py [path/to/b113028.txt]
       (default: ./b113028.txt — run from the repo root)
Exit 0 iff the b36 gate and every comparison pass.

Historical note this script demonstrates (do not over-read it): in 36..64,
selection was never the 2006 gap — the pick is correct at all 29 solved
bases, including b63 (seven drops) and b64 (full alphabet), and the prefix
pruning settles b60's selection in ~1.5M steps where the 2026
pre-churn-fix enumerator churned ~6e10 complete subsets
(B60-DISCOVERY-CHURN-FIX.md). What 2006 lacked was fallback past the
first set, the cardinality-domination strategy, arrangement-aware set
rejection, and a standard of proof.

ONE KNOWN EXCEPTION, below this script's range: at base 6 the first
rule-passing set is {5,4,1}, which has NO valid arrangement — the true
a(6)=412 uses {4,2,1}, the next set at the same cardinality. The April
2006 C artifact demonstrably fails there (prints nulls); the February
2006 Ruby run solved b6, so the two 2006 programs differed. b6 is the
unique base in 2..64 where the no-fallback gap actually fires.
"""
import sys
from math import lcm, gcd

sys.setrecursionlimit(100000)


def make_subset_2006(base, step_cap=200_000_000):
    """First descending-lex digit set at the largest feasible cardinality,
    top digit pinned to base-1, exactly as finite.c's make_subset."""
    steps = [0]

    def dfs(chosen, cur_lcm, cur_sum, length):
        steps[0] += 1
        if steps[0] > step_cap:
            raise RuntimeError("step cap exceeded")
        i = len(chosen)
        if i == length:
            if cur_sum % gcd(base - 1, cur_lcm) == 0:  # nine rule
                return chosen
            return None
        lo = length - i  # must leave room for length-i more distinct digits
        for d in range(chosen[-1] - 1, lo - 1, -1):
            nl = lcm(cur_lcm, d)
            if nl % base == 0:  # ten rule: prune the whole subtree
                continue
            r = dfs(chosen + [d], nl, cur_sum + d, length)
            if r:
                return r
        return None

    for length in range(base - 1, 0, -1):
        r = dfs([base - 1], base - 1, base - 1, length)
        if r:
            return r, steps[0]
    return None, steps[0]


def digit_set(n, b):
    s = set()
    while n:
        s.add(n % b)
        n //= b
    return s


def main():
    bfile = sys.argv[1] if len(sys.argv) > 1 else "b113028.txt"
    canon = {}
    with open(bfile) as f:
        for line in f:
            n, a = line.split()
            canon[int(n)] = int(a)

    # Gate: reproduce the 2006 log's own b36 answer set.
    pick, steps = make_subset_2006(36)
    want36 = set(range(1, 36)) - {9, 16, 18, 27}
    if set(pick) != want36:
        print("GATE FAILED: b36 pick does not match the 2006 run log")
        return 1
    print(f"gate: b36 pick == 2006 run-log answer set ({steps} steps)")

    fails = 0
    for B in sorted(b for b in canon if 36 <= b <= 64):
        pick, steps = make_subset_2006(B)
        true = digit_set(canon[B], B)
        ok = set(pick) == true
        drops = sorted(set(range(1, B)) - set(pick))
        tag = "MATCH" if ok else "MISMATCH"
        print(f"b{B}: |D|={len(pick)} drops={drops} {tag} "
              f"({steps:,} steps)")
        if not ok:
            fails += 1
            print(f"   true answer drops: "
                  f"{sorted(set(range(1, B)) - true)}")
    if fails:
        print(f"{fails} MISMATCH(ES)")
        return 1
    print("ALL BASES: 2006 selection == true answer digit set")
    return 0


if __name__ == "__main__":
    sys.exit(main())
