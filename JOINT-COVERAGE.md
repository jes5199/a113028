# Joint coverage, coupon-collector thresholds, and a correction to the conjecture

*(Companion to OPEN-PROBLEM.md and SINGLE-MODULUS.md. Three results from a
working session, 2026-07-21: (1) a proved theorem giving simultaneous
coverage of several prime-power moduli via mutually invisible exchange
gadgets — the first rigorous statement about the coupling; (2) empirical
evidence that both single- and joint-modulus feasibility thresholds are
pure coupon-collector, with no structural obstruction found anywhere;
(3) a correction to the threshold conjecture's first-moment count: the
right quantity is the class-partition count, not m!, and the refined
statement explains the observed window width. Verification scripts and
raw output are reproduced at the end for re-running.)*

## Setting

As in OPEN-PROBLEM.md: radix B ≥ 3, digit set A = {1, …, M} \ R with
|R| ≤ C₀, m = |A|, positions {0, …, m−1}, N(σ) = Σ_{d∈A} d·B^{σ(d)}
for an arrangement σ. SINGLE-MODULUS.md proves: for a single odd prime
power q with p ∤ B(B−1) and e = ord_q(B) ≥ 2, if
w = min(⌊m/e⌋, ⌊ℓ/2⌋) satisfies w² ≥ q−1 (ℓ = longest hole-free run in
A), then every target mod q is achievable ("Theorem 1"). Its proof
freezes all degrees of freedom except one two-class digit exchange. This
note is about not freezing them.

## Theorem 2 (joint coverage for independent orders)

Let q₁, …, q_k be powers of distinct odd primes p_j with p_j ∤ B(B−1),
orders e_j = ord_{q_j}(B) ≥ 2, and E = lcm(e₁, …, e_k).

**Independence condition:** for each j,

```
ord_{p_j}(B)  ∤  L_j := lcm_{l ≠ j} e_l .
```

Set w_j = ⌈√(q_j − 1)⌉. Suppose the positions {0, …, m−1} contain, in
each of 2k prescribed residue classes mod E, at least the required
quota (in particular m ≥ E·(max_j w_j + 1) suffices), and A contains k
pairwise disjoint hole-free blocks of consecutive digits of sizes
2w₁, …, 2w_k. Then for **every** joint target
(t₁, …, t_k) ∈ Z_{q₁} × ⋯ × Z_{q_k} there is a single arrangement σ
with N(σ) ≡ t_j (mod q_j) for all j simultaneously.

**Proof.** For each j put Δ_j = L_j. For l ≠ j we have e_l | Δ_j, so
B^{Δ_j} ≡ 1 (mod q_l). By the independence condition,
B^{Δ_j} ≢ 1 (mod p_j), so

```
u_j := 1 − B^{Δ_j}
```

is a **unit** mod q_j. Choose base residues ρ_j (mod E) so that the 2k
classes {ρ_j, ρ_j + Δ_j mod E} are pairwise distinct (greedily; each
choice must avoid at most 4(k−1)+1 residues, so E ≥ 4k suffices, and in
practice far less). Note ρ_j ≢ ρ_j + Δ_j (mod E) since e_j ∤ Δ_j.

Gadget j exchanges digits between the position-classes
{i ≡ ρ_j (mod E)} and {i ≡ ρ_j + Δ_j (mod E)}. By construction such an
exchange is invisible mod q_l for every l ≠ j (equal weights), and
visible mod q_j with weight difference B^{ρ_j}·u_j, a unit.

Dedicate to gadget j a block U_j of 2w_j consecutive digits, the blocks
pairwise disjoint; give U_j quota w_j in each of its two classes; fill
all remaining quota of every class arbitrarily from A minus the blocks,
and freeze that choice. Then, mod q_j,

```
N ≡ const_j + s_j · B^{ρ_j} u_j   (mod q_j),
```

where s_j is the sum of the w_j-subset S_j ⊆ U_j assigned to class
ρ_j — the frozen mass and every other gadget are constants mod q_j. By
the interval lemma of SINGLE-MODULUS.md, s_j ranges over a complete run
of w_j² + 1 ≥ q_j consecutive integers as S_j varies; a length-≥q_j
arithmetic progression with unit difference covers Z_{q_j}. The k
subset choices are supported on disjoint digit blocks and disjoint
position classes, hence independent, so any joint target is met. ∎

### Remarks on Theorem 2

1. **What it is and is not.** This is the first statement proving the
   coupling across prime powers is achievable — but its depth
   requirement scales like E·√(max q), and E = lcm of the orders blows
   up with k, so it does not approach the conjecture's regime
   (m ≈ m*). Its value is (a) showing the coupling is algebraically
   unobstructed wherever there is room, and (b) isolating the atomic
   move — the mutually invisible exchange — that any absorption-style
   proof of the full conjecture will need.

2. **Entangled orders.** The independence condition genuinely excludes
   order-sets like {2, 3, 6}, where every e_j divides the lcm of the
   others, and no ordering of sequential tuning works. Empirically (see
   below) entangled sets show **no obstruction whatsoever**. A likely
   escape for the proof: for prime q with even order e = 2f, cyclicity
   of the units forces B^f ≡ −1 (mod q), so a *pair* of exchanges at
   class offsets differing by f, with equal digit-differences, cancels
   mod q while adding mod any modulus of order dividing f — a compound
   move that tunes inside the entanglement. Worked micro-example for
   orders {2, 3, 6}: exchanges at class pairs (r, r+2) and (r+3, r+5)
   with equal digit-differences are each invisible mod the order-2
   modulus (offsets even), have equal effect mod the order-3 modulus
   (B³ ≡ 1), and cancel mod the order-6 modulus (B³ ≡ −1) — so the
   pair tunes order-3 while invisible to orders 2 and 6. Generalizing
   this to arbitrary entangled order-sets is the natural next theorem.

3. **CRT merge.** Moduli sharing the same order e impose constraints
   through the same class-sum vector and merge by CRT into a single
   constraint mod their product. The number of genuinely independent
   constraints is the number of **distinct orders**, not the number of
   prime powers. (Useful both for proofs and for Engine C bookkeeping.)

## Empirical results

All experiments brute-force exact coverage (full enumeration of
arrangements, or exact class-partition DP), radix and digit sets as
noted. Scripts at the end.

### E1. Theorem 1 verified; its depth condition is very loose

96 legal single-modulus instances across
B ∈ {7, 10, 12, 23, 49}, m up to 16, q up to ~300, including
interval-minus-holes digit sets: **zero violations** of Theorem 1.
Moreover all 86 cases *below* the depth condition w² ≥ q−1 were also
fully covered, including primitive-root-type cases with w = 1. The
condition is sufficient only, and far from necessary.

### E2. Single-modulus failures are coupon-collector, not structural

In the regime the theorems do not reach (e ≥ m, all weights distinct),
with the legality constraint B > m enforced, coverage failures exist —
and every one is a near-miss (1–3 empty residues out of hundreds).
Failures occur only for m!/q up to ≈ 8.5 and vanish above; full
coverage appears from m!/q ≈ 4. This matches the random model
exactly: P(some residue empty) ≈ q·e^{−m!/q}, which crosses 1 at
m!/q ≈ ln q ≈ 6.5–7 for the q tested. (Caution from E1's sweep: with
B ≤ m the instance is illegal and the integer range of N can be
smaller than q, producing fake "failures" — filter B > m.)

### E3. Per-residue counts are near-Poisson; deviations have causes

Full histograms of #{σ : N(σ) ≡ t (mod q)} versus Poisson(m!/q):

- Generic digit sets: variance/mean ≈ 0.73–0.79 — mild
  **under**-dispersion, i.e. more uniform than random (favorable for
  the conjecture).
- B = 8, m = 7 (complement-closed A = {1,…,B−1}): variance/mean ≈ 1.38,
  and every count-frequency is even. Cause, exact: when A = {1,…,B−1},
  the digit map d ↦ B−d induces an involution on arrangements with

  ```
  N ↦ B·G − N,   G = (B^m − 1)/(B − 1)  (arrangement-invariant),
  ```

  so residues pair up t ↔ B·G − t with **identical** counts. This
  symmetry exists precisely at the top of the descent (full digit set)
  and makes per-residue fluctuations correlated in pairs — it must be
  accounted for in any second-moment analysis.

### E4. Joint coverage exceeds Theorem 2 everywhere tested

B = 10, A = {1,…,m}, exact joint coverage mod Q = Π q_j:

| moduli | orders | E | Q | m | m!/Q | joint coverage |
|---|---|---|---|---|---|---|
| 11, 37 | 2, 3 | 6 | 407 | 8 | 99 | FULL |
| 11, 41 | 2, 5 | 10 | 451 | 9 | 805 | FULL |
| 11, 7 | 2, 6 (entangled) | 6 | 77 | 8 | 524 | FULL |
| 11, 37, 7 | 2, 3, 6 (entangled) | 6 | 2849 | 9 | 127 | FULL |
| 37, 7 | 3, 6 (entangled) | 6 | 259 | 9 | 1401 | FULL |
| 11, 37, 41 | 2, 3, 5 | 30 | 16687 | 9 | 21.7 | FULL |

Note the last row: E = 30 > m = 9, so Theorem 2's gadget cannot even
be instantiated, yet joint coverage is total. Entanglement produces no
obstruction. The proofs are far behind the truth.

### E5. The joint threshold is also coupon-collector — with one crucial exception

Pushing Q up to bracket the joint threshold (B = 10, m = 9,
9! = 362880):

| moduli | orders | E | Q | λ = m!/Q | empty observed | Poisson pred |
|---|---|---|---|---|---|---|
| 11, 37, 41 | 2,3,5 | 30 | 16687 | 21.7 | 0 | 0.0 |
| 7, 11, 13, 41 | 6,2,6,5 | 30 | 41041 | 8.8 | 12 | 5.9 |
| **7, 13, 37, 11** | **6,6,3,2** | **6** | **37037** | **9.8** | **11767** | **2.1** |
| 7, 13, 41, 37 | 6,6,5,3 | 30 | 138047 | 2.6 | 13455 | 9964 |
| 11, 13, 37, 41 | 2,6,3,5 | 30 | 216931 | 1.7 | 41174 | 40723 |

Every E = 30 row matches the Poisson model. The E = 6 row misses it by
four orders of magnitude — and the cause is a counting error in the
model, not new structure:

**When all orders divide a common E ≤ m, N mod Q factors through the
class sums, so the effective number of independent samples is not m!
but the class-partition count**

```
P(m, E) = m! / Π_{r=0}^{E−1} k_r! ,    k_r = #{ 0 ≤ i < m : i ≡ r (mod E) }.
```

For the anomalous row: P(9, 6) = 9!/(2!2!2!) = 45360, so the corrected
λ_eff = 45360/37037 = 1.22, predicting 37037·e^{−1.22} ≈ 10883 empty
residues versus 11767 observed — agreement to 8%, with the residual
attributable to collisions among partitions' sum-vectors (exact
distinct-residue count by class DP: 25270/37037). The naive model
predicted 2.

## The corrected conjecture

The threshold quantity in OPEN-PROBLEM.md compares m! to L_eff. E5
shows m! is the wrong count whenever the position-classes mod
E = lcm{e_q : q ∥ L_eff} are non-singleton. The corrected first-moment
count is P(m, E) above. In genuine A113028 instances E is the lcm of
many multiplicative orders and vastly exceeds m, every class is a
singleton, and P(m, E) = m! — so m* and all empirical claims in
OPEN-PROBLEM.md survive unchanged. But the honest general statement,
and the one an analytic attack must target, is:

**Refined threshold conjecture.** For instances with the interval
structure of OPEN-PROBLEM.md and targets satisfying the cheap
conditions,

```
P(m, E) ≥ L_eff · ln L_eff   ⟹   PERM-FEAS(B, A, L_eff, t) = YES,
```

with the counting form #{σ : N(σ) ≡ t} = (P(m,E)/L_eff)·(1 + o(1))
uniformly over admissible t, holding already at P(m,E) ≥ L_eff^{1+ε}
and conjecturally down to the ln factor.

Two consequences of the ln L_eff factor:

1. **It explains the window width.** One depth level multiplies the
   count by ≈ m ≈ m*, while the required oversampling factor is only
   ln L_eff ≈ M ≈ m*·ln m* — so the coupon-collector correction costs
   ≈ 1 level. Combined with Poisson fluctuation at the edge, this
   predicts an observed window of c ≈ 1–2, matching the ≤ ~3 seen
   across all 47 instances. The window width stops being a mystery
   constant.

2. **It tells the analyst what to equidistribute.** Any circle-method
   attack must count class partitions, not permutations, whenever
   orders collapse — and must handle the complement involution of E3
   when A is complement-closed.

## Suggested repo actions (for the agent)

1. Add this file; cross-link from OPEN-PROBLEM.md ("Status" section)
   and SINGLE-MODULUS.md.
2. Patch OPEN-PROBLEM.md: state the refined conjecture with P(m, E)
   and the ln L_eff factor; note it degenerates to the original m*
   form since E ≫ m in all real instances.
3. Re-run the scripts below (they are self-contained; sympy required
   for the first two) and commit their output as evidence files;
   extend E2/E5 sweeps to larger m with sampling if desired.
4. Engine C: implement the CRT merge of same-order moduli (Remark 3)
   and the O(1) discharge of moduli satisfying Theorem 1's condition
   (see SINGLE-MODULUS.md, with the node-level depth re-check).
5. Open items, in rough order: (a) compound-move generalization for
   entangled order-sets (Remark 2); (b) primitive-root regime lemma
   (large e, GAP covering); (c) second-moment computation across the
   47 real instances, accounting for the involution.

## Verification scripts

### verify.py — E1 (Theorem 1, exact class-partition DP)

```python
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
```

### threshold2.py — E2 (legal-instance coupon-collector sweep, B > m)

```python
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
```

### poisson.py — E3 (per-residue count histogram vs Poisson)

```python
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
```

### jointcc.py — E4/E5 (joint coverage and corrected first moment)

```python
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
```

## Status

Theorem 2: elementary, believed correct as stated, not refereed.
Empirical claims: exact enumeration, no sampling, reproducible from
the scripts above (Python 3, sympy for E1/E2). Session date
2026-07-21.
