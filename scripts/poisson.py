import itertools, math
from collections import Counter

def hist(B, q, m):
    wts = [pow(B, i, q) for i in range(m)]
    cnt = Counter()
    for perm in itertools.permutations(range(m)):
        cnt[sum((d + 1) * wts[p]
                for d, p in zip(range(m), perm)) % q] += 1
    lam = math.factorial(m) / q
    mean = lam
    var = sum(cnt[t] ** 2 for t in range(q)) / q - mean ** 2
    print(f"B={B} q={q} m={m} lambda={lam:.2f} "
          f"var/mean={var / mean:.3f}")

hist(8, 593, 7)    # complement-closed: over-dispersed, involution
hist(8, 997, 7)
hist(10, 719, 7)   # generic: under-dispersed
