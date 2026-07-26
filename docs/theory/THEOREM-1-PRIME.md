# Theorem 1′: exact cosets per cyclotomic layer

> **ERRATA APPLIED (2026-07-22):** this file incorporates corrections
> E1–E5 from an external review ("Sol"), adopted and verified in
> THEOREM-2-DOUBLE-PRIME.md Part I — see there for the counterexamples
> that motivated each fix. Theorem 2″ in that file also supersedes the
> layer hypothesis here (arbitrary Λ coprime to B, no layer needed).

*(Companion to OPEN-PROBLEM.md, SINGLE-MODULUS.md, and JOINT-COVERAGE.md.
This note supersedes SINGLE-MODULUS.md's Theorem 1 and closes its
Remark 1: one statement now handles all prime powers coprime to B —
including p | B−1 and p = 2 — with no case analysis, gives the exact
achievable coset for an entire cyclotomic layer at once, and yields
closed-form "cheap conditions" that strengthen the joint-layer DP
proposal. Session 2026-07-21; verification script and results at the
end.)*

## Setting

As in OPEN-PROBLEM.md: radix B ≥ 3, digit set A = {1, …, M} \ R with
|R| ≤ C₀, m = |A|, positions {0, …, m−1},
N(σ) = Σ_{d∈A} d·B^{σ(d)} for an arrangement σ. ℓ denotes the length
of the longest hole-free run of consecutive integers in A. The
interval lemma (SINGLE-MODULUS.md): fixed-size subset sums of a run
of consecutive integers form a complete interval; a-subsets of an
n-run give width a(n−a).

A **cyclotomic layer** is a collection of pairwise coprime prime
powers q₁, …, q_k, each coprime to B, whose multiplicative orders
e_i = ord_{q_i}(B) all divide a common e ≥ 2. (In particular all
prime-power components of B^e − 1 that are coprime to B qualify.)
Write Q = ∏ q_i.

## Theorem 1′

Define the **invariant modulus**

```
V = ∏_i p_i^{v_i},     v_i = min(a_i, v_{p_i}(B − 1))     [E1: capped]
  = gcd(Q, B − 1),
```

where q_i = p_i^{a_i} and v_p denotes p-adic valuation. (E1: the cap
is required — order-1 components slip through "orders dividing e"
since 1 | e, and the uncapped product can fail to divide Q, e.g.
B = 10, q = 3.) Let w = min(⌊m/e⌋, ⌊ℓ/2⌋).

**(a) (Unconditional.)** Every arrangement satisfies

```
N(σ) ≡ Σ_{d ∈ A} d   (mod V),
```

and the set {N(σ) mod Q} lies in a single coset of V·Z_Q. **If
ℓ ≥ 2** [E2], the differences N(σ) − N(σ′) generate V·Z_Q exactly —
no finer congruence holds across all arrangements. (E2 counterexample
at ℓ = 1: B = 3, Q = 8, A = {1,3} has image exactly {2, 6}, whose
differences generate only 4·Z₈.)

**(b) (Depth condition.)** If w² ≥ Q/V − 1, then the achievable set
is **exactly** that coset: all Q/V residues t ≡ Σ_{d∈A} d (mod V)
are attained.

### Proof

**(a)** By definition of v_i, B ≡ 1 (mod p_i^{v_i}), hence every
weight B^j ≡ 1 (mod p_i^{v_i}) and N(σ) ≡ Σd (mod p_i^{v_i}) for
every σ; CRT combines these to mod V. Any two arrangements are
connected by transpositions, and transposing digits d, d′ at
positions j < j′ changes N by

```
(d − d′) · B^j · (B^{j′−j} − 1)  ∈  (B−1)·Z_Q  ⊆  V·Z_Q
```

(the containment because (B^Δ − 1) is divisible by (B − 1) as
integers), so the full set lies in one coset of V·Z_Q. For
exactness: transposing *consecutive* digits (d′ = d+1, available
since ℓ ≥ 2) at *adjacent* positions changes N by ±B^j(B−1), and
v_{p_i}(B−1) = v_i exactly for every i, so B−1 = V·u with u a unit
mod Q; these single differences already generate V·Z_Q.

**(b)** Freezing construction as in SINGLE-MODULUS.md, but with the
exchange always between the adjacent position-classes 0 and 1
(mod e) — legitimate for every component simultaneously, because
e_i | e means positions congruent mod e are congruent mod e_i, so
class-0 positions all weigh B^0 ≡ 1 and class-1 positions all weigh
B^1 ≡ B modulo every q_i.

Take a hole-free run J ⊆ A, let U be its first 2w elements, reserve
quota w in class 0 and w in class 1 (class sizes are ≥ ⌊m/e⌋ ≥ w),
freeze all other digits arbitrarily into the remaining quotas. For
S₀ ⊆ U with |S₀| = w sent to class 0 (complement to class 1), with
s = sum(S₀) and T = sum(U):

```
N ≡ (const + T·B) + s·(1 − B)   (mod Q).
```

The single decisive point [E1 phrasing]: **gcd(1 − B, Q) = V**, so
δ = 1 − B has additive order Q/V in Z_Q. Choosing adjacent classes
(Δ = 1) avoids every lifting-the-exponent subtlety that plagues
B^Δ − 1 for Δ > 1 (in particular the p = 2 anomaly at even Δ). By
the interval lemma, s sweeps a complete run of w² + 1 ≥ Q/V
consecutive integers, so const + s·δ covers the full coset
const + V·Z_Q. By (a) that is the coset containing Σd. ∎

### Corollaries and remarks

1. **Theorem 1 is subsumed.** The case k = 1, p ∤ B−1 gives V = 1
   and full coverage under w² ≥ q − 1 — SINGLE-MODULUS.md's Theorem
   1 — and the old hypotheses "p odd, p ∤ B−1" are now theorems'
   conclusions, not assumptions. SINGLE-MODULUS.md's Remark 1
   (deferred LTE bookkeeping) is closed.

2. **The depth condition weakens dramatically when V is large.** For
   a single q = p^j with p | B−1 (p odd), Q/V = p^{max(0, j−v)} [E5],
   which equals ord_{p^j}(B); so w² ≥ Q/V − 1 is trivial in practice.

3. **Closed-form cheap conditions — corrected per E3.** The invariant
   congruence t ≡ Σ_{d∈A} d (mod V) is a ROOT-ONLY check: it is
   preserved at every descendant of the search (placing d at position
   j changes the residual target by d·B^j ≡ d (mod V) and the
   remaining sum by d), so it cannot prune individual branches. The
   per-node value of this theorem is the *skip* direction: at a node
   whose **local** m, ℓ satisfy w² ≥ Q/V − 1, the layer is guaranteed
   feasible for every coset target and drops out of the joint search
   on that subtree.

4. **Relation to the joint-layer DP proposal** (reviewer's point 1).
   The proposal to run joint DPs mod Q_e is correct and strictly
   stronger than per-prime tests; this theorem explains *why* and
   caps *how much*: the joint image is exactly the V-coset once the
   sum-vector polytope is large enough, and any deficit below that
   is a depth artifact of the polytope, not a hidden congruence.

5. **What remains open.** Same as before, but sharper: the gap
   between the proved fill depth (w² ≥ Q/V) and the observed one
   (polytope/coupon-collector scale, m ≈ 10–14 in the tests below)
   is the quantitative problem; and the coupling *across* layers —
   distinct e — is untouched here and remains the content of the
   threshold conjecture (see JOINT-COVERAGE.md, Theorem 2 and the
   refined conjecture).

## Verification (exact class-partition DP, A = {1,…,m})

All rows computed by the script below. "Contained" = image ⊆
invariant coset (part (a), predicted unconditionally); "EXACT" =
image equals the full coset of size Q/V.

### Base 16, layer e = 3: Q = 9·7·13 = 819, V = 3, coset 273

| m | image | contained | exact |
|---|---|---|---|
| 9 | 253/273 | yes | no |
| 10 | 273/273 | yes | **yes** |
| 11–13 | 273/273 | yes | **yes** |

This closes the base-16 case from the review: the separate-DP count
273 is the coset (3 residues mod 9 — the invariant t ≡ Σd (mod 3) —
times full 7 and 13); the joint-DP count 253 at m = 9 is reproduced
exactly and is a depth artifact that fills to precisely 273 at
m = 10 and freezes.

### Base 49, layer e = 2: Q = 32·25 = 800, V = 16, coset 50

| m | image | contained | exact |
|---|---|---|---|
| 6 | 10/50 | yes | no |
| 7 | 13/50 | yes | no |
| 8 | 17/50 | yes | no |
| 9 | 21/50 | yes | no |
| 10 | 26/50 | yes | no |
| 11 | 31/50 | yes | no |
| 12 | 37/50 | yes | no |
| 13 | 43/50 | yes | no |
| 14 | 50/50 | yes | **yes** |

Consequence for Engine C at base 49 — **corrected per E3/E4**: the
mod-16 invariant is automatic below the root (E3), so it yields no
per-node pruning; and the "complete for all m ≥ 14" fact was computed
for the *interval* {1,…,m} and does NOT transfer to holey digit sets
(E4 counterexample: the 14 odd digits {1,…,27} achieve only 25 of the
50 coset residues — a digit-stride sub-invariant, gcd(800, 2·48) =
32). The valid per-node consequence is the skip rule with the LOCAL
run-length ℓ: where w = min(⌊m/2⌋, ⌊ℓ/2⌋) satisfies w² ≥ 49, the
Q₂ = 800 layer provably cannot prune and is dropped from that
subtree; elsewhere the layer DP retains (smaller-image) pruning
power. Fill depths must never be cached across hole patterns.

### 2-power and mixed layers (no special-casing)

| case | Q | V | coset | result |
|---|---|---|---|---|
| B=7, q=32, e=4 | 32 | 2 | 16 | exact for all m ∈ [6,12] |
| B=7, layer {32,5}, e=4 | 160 | 2 | 80 | 78/80 at m=6; exact for all m ∈ [7,14] |
| B=10, q=27, e=3 | 27 | 9 | 3 | exact for all m ∈ [4,9] |

Containment (part (a)) held in every row of every table at every
depth, as proved.

## Suggested repo actions (for the agent)

1. Add this file; in SINGLE-MODULUS.md mark Theorem 1 and Remark 1
   as superseded by this note (keep the interval lemma — it is used
   here).
2. In the OPEN-PROBLEM.md rewrite, state the cheap conditions per
   cyclotomic layer as: (i) the closed-form invariant congruence
   t ≡ Σd (mod V_e) for each layer, plus (ii) the layer DP image
   below fill depth; cite part (a) for validity of (i) at all
   depths.
3. Engine C: implement the mod-V_e congruence checks (O(1) per
   node); compute each layer's fill depth once per base by running
   the layer DP on intervals and cache the depth after which the
   image is the full coset — above it, replace the DP by the
   congruence.
4. Verify empirically across bases 7–60: fill depth as a function of
   Q/V and the class structure (expected: polytope scale, far below
   w² ≥ Q/V); this calibrates how conservative (b) is.
5. Open items unchanged from JOINT-COVERAGE.md, now sharper:
   (a) cross-layer coupling (distinct e) — compound moves /
   Theorem 2 generalization; (b) quantitative fill depth
   (coupon-collector rate) replacing w² ≥ Q/V; (c) refutation-volume
   (Conjecture B) experiments using the corrected |T_m| computed
   with these exact layer conditions.

## Verification script

```python
import math

def layer_image(B, m, e, Q):
    """Exact achievable set of N mod Q over all arrangements of
    A={1..m}, via position classes mod e (valid when all component
    orders divide e)."""
    ks = [sum(1 for i in range(m) if i % e == r) for r in range(e)]
    wts = [pow(B, r, Q) for r in range(e)]
    dp = {tuple(ks): {0}}
    for d in range(1, m + 1):
        nxt = {}
        for caps, residues in dp.items():
            for r in range(e):
                if caps[r]:
                    nc = list(caps); nc[r] -= 1; nc = tuple(nc)
                    add = d * wts[r] % Q
                    nxt.setdefault(nc, set()).update(
                        (x + add) % Q for x in residues)
        dp = nxt
    (img,) = dp.values()
    return img

def vP(n, p):
    v = 0
    while n % p == 0:
        n //= p; v += 1
    return v

def test(B, e, comps, mrange, label):
    Q = math.prod(comps)
    V = 1
    for q in comps:
        p = min(f for f in range(2, q + 1) if q % f == 0)
        V *= p ** vP(B - 1, p)
    print(f"{label}: B={B} e={e} Q={Q} comps={comps} V={V} "
          f"coset={Q // V}")
    for m in mrange:
        img = layer_image(B, m, e, Q)
        s = sum(range(1, m + 1))
        contained = all(t % V == s % V for t in img)
        exact = len(img) == Q // V
        print(f"  m={m:2d}: image={len(img)}/{Q // V} "
              f"contained={contained} exact={exact}")

test(16, 3, [9, 7, 13], range(9, 14), "base16 Q3")
test(49, 2, [32, 25], range(6, 15), "base49 Q2")
test(7, 4, [32], range(6, 13), "base7 q=32")
test(7, 4, [32, 5], range(6, 15), "base7 mixed Q=160")
test(10, 3, [27], range(4, 10), "base10 q=27")
```

## Status

Theorem 1′: elementary, believed correct as stated, not refereed.
Verification: exact enumeration via class-partition DP, no sampling,
reproducible from the script (Python 3, stdlib only). Session date
2026-07-21.
