#!/usr/bin/env python3
"""Verify FASTER-PROVISIONAL-MAXIMUM-VALIDATION.md section 8 forced-set cardinalities.
See FORCED-SET-CARDINALITY-AUDIT.md for the argument. Necessary conditions only."""
from math import gcd

def factorint(n):
    f = {}; d = 2
    while d * d <= n:
        while n % d == 0: f[d] = f.get(d, 0) + 1; n //= d
        d += 1
    if n > 1: f[n] = f.get(n, 0) + 1
    return f

def lcmv(ds):
    L = 1
    for d in ds: L = L // gcd(L, d) * d
    return L

def forced(B):
    """Max-cardinality D subset {1..B-1} with B does not divide lcm(D) and
    digitsum(D) == 0 mod gcd(lcm(D), B-1). Returns (drops, D, route)."""
    n = B - 1; full = list(range(1, B)); best = None
    for p, e in factorint(B).items():
        pe = p ** e
        drops = {d for d in full if d % pe == 0}
        D = [d for d in full if d not in drops]
        if lcmv(D) % B == 0: continue
        g = gcd(lcmv(D), n); s = sum(D); extra = []
        if g > 1 and s % g:
            need = s % g
            single = [d for d in D if d % g == need]
            if single: extra = [min(single)]
            else:
                for i in range(len(D)):
                    for j in range(i + 1, len(D)):
                        if (D[i] + D[j]) % g == need: extra = [D[i], D[j]]; break
                    if extra: break
                if not extra: continue
        D2 = [d for d in D if d not in extra]
        if lcmv(D2) % B == 0: continue
        g2 = gcd(lcmv(D2), n)
        if g2 > 1 and sum(D2) % g2: continue
        if best is None or len(D2) > len(best[1]):
            best = (sorted(drops | set(extra)), D2, pe)
    return best

if __name__ == "__main__":
    # self-validation: certified bases must reproduce exactly
    CERTIFIED = {56: 48, 58: 55, 60: 47, 63: 55}
    IN_USE = {54: 50, 59: 56, 61: 58, 62: 58, 64: 60}
    print("== self-validation against certified bases ==")
    ok = True
    for B, want in sorted(CERTIFIED.items()):
        drops, D, pe = forced(B)
        good = len(D) == want
        ok &= good
        print(f"  B={B:>3} route={pe:>3} |D|={len(D):>3} expected={want:>3} {'OK' if good else 'MISMATCH'}  drops={drops}")
    print(f"  -> {'4/4 PASS' if ok else 'FAILED'}")
    print("== section 8 claims ==")
    for B, cur in sorted(IN_USE.items()):
        drops, D, pe = forced(B)
        print(f"  B={B:>3} route={pe:>3} |D|={len(D):>3} in-use={cur:>3} gain={len(D)-cur:+d}  drops={drops}")
