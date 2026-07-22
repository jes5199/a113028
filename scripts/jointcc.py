import itertools, math

def joint(B, M, qs):
    A = list(range(1, M + 1)); m = len(A)
    Q = math.prod(qs)
    wts = [pow(B, i, Q) for i in range(m)]
    seen = set()
    for perm in itertools.permutations(range(m)):
        seen.add(sum(d * wts[p] for d, p in zip(A, perm)) % Q)
    lam = math.factorial(m) / Q
    print(f"qs={qs} Q={Q} lambda={lam:.2f} "
          f"empty={Q - len(seen)} poisson={Q * math.exp(-lam):.1f}")

B, M = 10, 9
joint(B, M, [11, 37, 41])
joint(B, M, [7, 11, 13, 41])
joint(B, M, [7, 13, 37, 11])   # E=6 anomaly: use P(m,E), not m!
joint(B, M, [7, 13, 41, 37])
joint(B, M, [11, 13, 37, 41])
# corrected count for the anomaly:
# P(9,6) = 9!/(2!2!2!) = 45360; lambda_eff = 45360/37037 = 1.22
# predicted empty = 37037*exp(-1.22) = 10883; observed = 11767
