#!/usr/bin/env python3
"""Bridge-Theorem measurement protocol (BRIDGE-AND-HARDNESS.md Part I).
Reads a113028_prof survivor samples ("P m avail_hex rQ2 rQ3"), recomputes
per-node fibers |T(A')| and surviving-children counts via the joint
cyclotomic-layer class-DP, and reports the K distribution, the beta
profile (gamma, burn-in c), and predicted-vs-actual volume.
Usage: bridge_measure.py B Q2 logfile
"""
import sys
from functools import lru_cache

B = int(sys.argv[1]); Q = int(sys.argv[2]); LOG = sys.argv[3]
E = 2  # layer order (both testbeds have only the e=2 layer nontrivial)
# Guard rails (PEELING-SUPPORT.md 5.6) - the b39 modulus bug preventer:
import math as _m
assert _m.gcd(B, Q) == 1, f"layer modulus {Q} shares a factor with B={B}"
assert pow(B, E, Q) == 1 % Q, f"B^{E} != 1 mod {Q}: wrong layer order"
WTS = [1 % Q, B % Q]

@lru_cache(maxsize=None)
def img(avail, m):
    """Achievable suffix residues mod Q of arranging the digit set `avail`
    (bitmask) into positions 0..m-1, via class-partition DP (classes i%2)."""
    ks = [(m + 1) // 2, m // 2]
    digs = [d for d in range(1, B) if avail >> d & 1]
    assert len(digs) == m
    states = {(ks[0], ks[1], 0)}
    for d in digs:
        nxt = set()
        for k0, k1, s in states:
            if k0: nxt.add((k0 - 1, k1, (s + d * WTS[0]) % Q))
            if k1: nxt.add((k0, k1 - 1, (s + d * WTS[1]) % Q))
        states = nxt
    return frozenset(s for _, _, s in states)

nodes = {}
for line in open(LOG):
    p = line.split()
    if not p or p[0] != 'P': continue
    m, avail, r2 = int(p[1]), int(p[2], 16), int(p[3])
    t2 = (Q - (r2 * pow(B, m, Q)) % Q) % Q
    nodes.setdefault((m, avail, t2), 0)
    nodes[(m, avail, t2)] += 1

import math
per_level = {}
ratios = []
for (m, avail, t2), cnt in sorted(nodes.items()):
    if m < 2: continue
    fib = img(avail, m)
    digs = [d for d in range(1, B) if avail >> d & 1]
    g = 0
    for d in digs[1:]: g = math.gcd(g, d - digs[0])
    H = math.gcd(Q, (B - 1) * g) if g else Q
    surv = 0; pred = 0.0
    for d in digs:
        child_t = (t2 - d * pow(B, m - 1, Q)) % Q
        cimg = img(avail & ~(1 << d), m - 1)
        if child_t in cimg: surv += 1
        pred += len(cimg) / Q
    L = per_level.setdefault(m, dict(n=0, surv=0, pred=0.0, fmax=0, node_in=0,
                                     ggt1=0, hcoset=0.0))
    L['n'] += 1; L['surv'] += surv; L['pred'] += pred
    L['fmax'] = max(L['fmax'], len(fib))
    L['node_in'] += (t2 in fib)
    L['ggt1'] += (g > 1)
    L['hcoset'] += len(fib) * H / Q   # ~1 iff fiber = its full H-coset
    if pred > 0.05: ratios.append(surv / pred)

print(f"B={B} Q2={Q}  sampled distinct nodes={len(nodes)}")
print(f"{'m':>3} {'#nodes':>7} {'selfOK%':>7} {'mean-surv':>9} {'mean-pred':>9} "
      f"{'beta':>7} {'g>1%':>5} {'fib/Hcoset':>10}")
for m in sorted(per_level, reverse=True):
    L = per_level[m]
    beta = L['fmax'] * m / Q
    print(f"{m:>3} {L['n']:>7} {100*L['node_in']/L['n']:>6.1f}% "
          f"{L['surv']/L['n']:>9.2f} {L['pred']/L['n']:>9.2f} {beta:>7.2f} "
          f"{100*L['ggt1']/L['n']:>4.0f}% {L['hcoset']/L['n']:>10.2f}")
rat = sorted(ratios)
if rat:
    import statistics as st
    print(f"K distribution (surv/pred): median={rat[len(rat)//2]:.2f} "
          f"p90={rat[int(.9*len(rat))]:.2f} p99={rat[int(.99*len(rat))]:.2f} "
          f"max={rat[-1]:.2f} n={len(rat)}")
