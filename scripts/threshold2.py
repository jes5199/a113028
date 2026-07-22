import itertools, sympy

def ordq(B, q):
    x, e = B % q, 1
    while x != 1:
        x = x * B % q; e += 1
    return e

fails = []; closest_ok = []
for q in [q for q in range(50, 2600) if sympy.isprime(q)]:
    for m in range(4, 10):
        fact = 1
        for i in range(2, m + 1): fact *= i
        if not (0.5 <= fact / q <= 40): continue
        for B in range(m + 1, 60):          # legal radix: B > m
            if B % q == 0 or (B - 1) % q == 0: continue
            e = ordq(B, q)
            if e < m: continue
            wts = [pow(B, i, q) for i in range(m)]
            seen = set()
            for perm in itertools.permutations(range(m)):
                seen.add(sum((d + 1) * wts[p]
                             for d, p in zip(range(m), perm)) % q)
                if len(seen) == q: break
            (fails if len(seen) < q else closest_ok).append(
                (fact / q, B, q, m, len(seen)))
            break
print("failures:", len(fails))
print("max m!/q with failure:",
      max((r[0] for r in fails), default=None))
print("min m!/q with full coverage:",
      min((r[0] for r in closest_ok), default=None))
