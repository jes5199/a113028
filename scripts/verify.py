import itertools, sympy

def ordq(B, q):
    if sympy.gcd(B, q) != 1: return None
    x, e = B % q, 1
    while x != 1:
        x = x * B % q; e += 1
        if e > q: return None
    return e

def longest_run(A):
    A = sorted(A); best = cur = 1
    for a, b in zip(A, A[1:]):
        cur = cur + 1 if b == a + 1 else 1
        best = max(best, cur)
    return best

def achievable_residues(A, B, q, e):
    m = len(A)
    ks = [sum(1 for i in range(m) if i % e == r) for r in range(e)]
    wts = [pow(B, r, q) for r in range(e)]
    states = {tuple(ks): {0}}
    for d in sorted(A):
        nxt = {}
        for caps, residues in states.items():
            for r in range(e):
                if caps[r] == 0: continue
                nc = list(caps); nc[r] -= 1; nc = tuple(nc)
                add = d * wts[r] % q
                tgt = nxt.setdefault(nc, set())
                tgt.update((x + add) % q for x in residues)
        states = nxt
    (final,) = states.values()
    return final

def check(B, M, R, qmax=400):
    A = [d for d in range(1, M + 1) if d not in R]
    m = len(A); ell = longest_run(A)
    for q in range(3, qmax):
        f = sympy.factorint(q)
        if len(f) != 1: continue
        p = next(iter(f))
        if p == 2 or B % p == 0 or (B - 1) % p == 0: continue
        e = ordq(B, q)
        if e is None or e < 2 or e > m: continue
        w = min(m // e, ell // 2)
        pred = w * w >= q - 1
        got = achievable_residues(A, B, q, e)
        full = len(got) == q
        flag = "OK" if (not pred or full) else "**COUNTEREXAMPLE**"
        print(f"B={B} m={m} q={q} e={e} w={w} pred={pred} covered {len(got)}/{q} {flag}")

for B, M, R in [(10, 9, set()), (10, 12, {5}), (7, 10, set()),
                (12, 14, {7, 11}), (10, 15, set()), (23, 12, {4}),
                (49, 13, set()), (10, 18, {9, 13})]:
    check(B, M, R)
