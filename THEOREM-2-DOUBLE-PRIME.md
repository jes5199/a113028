# Theorem 2″: joint exactness via digit-stride isolation — and errata to Theorem 1′

> **ERRATA APPLIED (2026-07-22, second review):** corrections A1–A6 from
> ERRATA-DIGIT-GCD.md are incorporated below (retitle; containment
> paragraph fixed; Remark 2 downgraded — the saturation statement lives in
> the generalized interval-coefficient model and says nothing uniform near
> m* when Λ grows with B, as in A113028; anti-conjecture claims removed;
> Engine C reachable-node correction; verification table relabeled). The
> digit-gcd lemma in that file strictly subsumes the completeness clause.

*(Companion to OPEN-PROBLEM.md, SINGLE-MODULUS.md, THEOREM-1-PRIME.md,
JOINT-COVERAGE.md. Two parts. Part I adopts the external review of
Theorem 1′ (reviewer "Sol", 2026-07-22) in full: two theorem-level
fixes and two Engine C corrections, each verified below — apply the
listed patches to THEOREM-1-PRIME.md. Part II proves Theorem 2″:
joint exactness of the invariant coset for **arbitrary** moduli
coprime to B — no independence conditions, no excluded primes, no
layer hypotheses. Its key tool, digit-stride invisibility, is also
exactly the mechanism behind Sol's second counterexample, which is
verified here as a consistency check. Session 2026-07-22.)*

---

## Part I — Errata to THEOREM-1-PRIME.md (adopted from review)

### E1 (required): V must be capped — define it as a gcd

With q_i = p_i^{a_i}, replace the definition of the invariant modulus
by

```
v_i = min(a_i, v_{p_i}(B−1)),      V = Π_i p_i^{v_i} = gcd(Q, B−1).
```

The uncapped draft definition can fail to divide Q (e.g. B = 10,
q = 3 gives draft V = 9 ∤ 3; order-1 components slip through the
"orders dividing e" hypothesis since 1 | e). In the proof of (b),
replace "(1−B)/V is a unit mod Q" by the correct and sufficient
statement:

> Since gcd(1−B, Q) = V, the element 1−B has additive order Q/V in
> Z_Q; hence const + s·(1−B) over s in any run of Q/V consecutive
> integers covers the full coset const + V·Z_Q.

### E2 (required): completeness in (a) needs ℓ ≥ 2

The containment N ≡ Σ_{d∈A} d (mod V) is unconditional, but the
*completeness* clause (differences generate V·Z_Q) uses a pair of
consecutive digits. Verified counterexample: B = 3, Q = 8,
A = {1, 3} (so ℓ = 1): the achievable residues are exactly {2, 6},
whose differences generate 4·Z₈, strictly finer than
V·Z₈ = 2·Z₈. Corrected statement:

> Every arrangement satisfies N(σ) ≡ Σ_{d∈A} d (mod V).
> **If ℓ ≥ 2**, arrangement differences generate V·Z_Q (swap two
> consecutive digits at adjacent positions: the difference
> ±B^j(B−1) has gcd V with Q), so this is the complete congruence
> invariant.

Part (b) is unaffected: when Q/V > 1 its depth condition forces
w ≥ 1 hence ℓ ≥ 2; when Q/V = 1 coverage is trivial.

### E3 (required): the mod-V check prunes only at the root

If t_rem and S_rem are the residual target and remaining digit sum
at a search node, then t_rem − S_rem ≡ t − ΣA (mod V) at **every**
descendant (placing digit d at position j changes t_rem by
d·B^j ≡ d (mod V) and S_rem by d). So the invariant congruence is
one root check; it cannot prune individual branches after the root
passes. Remove "O(1) per node congruence checks" from the action
list. The per-node value of the layer theorems is the *skip*
direction: at a node whose **local** m, ℓ satisfy w² ≥ Q/V − 1,
feasibility mod that layer is guaranteed for every target in the
coset, so the layer drops out of the joint search at that subtree.

### E4 (required): interval fill depths do not transfer to
### non-interval nodes

The base-49 result "image full at m = 14" was proved/computed for
A = {1,…,14}. It does not follow from m ≥ 14 alone. Verified
counterexample (Sol): B = 49, Q = 800, A = the 14 odd digits
{1, 3, …, 27} — the image has exactly **25** of the 50
invariant-coset residues. Safe policy: treat empirical fill depths
as valid only for certified interval states; elsewhere rely on the
local ℓ-condition of the theorem (real A113028 nodes have
ℓ ≥ ⌈(M′−C₀)/(C₀+1)⌉, so the condition is checkable in O(1)).

*Why 25 — and why this confirms rather than threatens the theory:*
a stride-s digit set has all digit differences divisible by s, so
every arrangement difference lies in gcd(Q, s(B−1))·Z_Q — a
**digit-side invisibility** invariant. For s = 2:
gcd(800, 2·48) = 32, predicting exactly 800/32 = 25 achievable
residues in one coset of 32·Z₈₀₀ (base point: N of any one
arrangement — not ΣA, since B ≢ 1 mod 32). Verified: the image is
exactly such a coset, size 25. This is precisely the mechanism
Part II turns into a tool.

### E5 (minor): Corollary/Remark 2 exponent

The single-q remark should read Q/V = p^{max(0, j−v)}, which for odd
p | B−1 equals ord_{p^j}(B); it equals the layer's chosen e only
when e is that exact order.

---

## Part II — Theorem 2″

### Digit-stride lemma

If U = {a, a+s, …, a+(2w−1)s} ⊆ A is an arithmetic progression of
stride s, the w-subset sums of U form a complete arithmetic
progression of common difference s with w² + 1 terms. (The interval
lemma of SINGLE-MODULUS.md applied to (U − a)/s.)

Dually to position-side invisibility (offset Δ with B^Δ ≡ 1 mod q),
**digit-side invisibility**: any exchange whose digit difference is
divisible by q is invisible mod q regardless of positions. Interval
digit sets contain APs of every stride, so sweeps remain available.

### Theorem 2″ (unconditional joint exactness)

Let Λ be any modulus with gcd(Λ, B) = 1, prime powers
q_p = p^{a_p}, E = lcm_p ord_{q_p}(B), and

```
V = gcd(Λ, B−1) = Π_p p^{min(a_p, v_p(B−1))}.
```

Partition the primes of Λ into groups G₁, …, G_k arbitrarily (e.g.
by exact order); put Q_j = Π_{p∈G_j} q_p, V_j = gcd(Q_j, B−1),
w_j = ⌈√(max(Q_j/V_j − 1, 0))⌉, and stride

```
S_j = Π_{p ∉ G_j} p^{a_p − min(a_p, v_p(B−1))}   ( = (Λ/Q_j) / (V/V_j) ).
```

Suppose A contains k pairwise disjoint arithmetic progressions of
stride S_j and length 2w_j, and the position classes 0 and 1
(mod E) each have at least Σ_j w_j positions (m ≥ E·(Σ w_j + 1)
suffices). Then the achievable set of N mod Λ is **exactly**

```
{ t ∈ Z_Λ : t ≡ Σ_{d ∈ A} d (mod V) }.
```

### Proof

**Containment** follows directly from B ≡ 1 (mod V); no completeness
clause from Theorem 1′ is needed [A2 — the previous derivation of
ℓ ≥ 2 from "the APs exist" was wrong: an AP of stride > 1 contains no
consecutive pair]. If every w_j = 0 then Q_j = V_j for every group,
hence Λ = V and containment is already equality. Otherwise the gadget
construction below proves coverage.

**Coverage.** Gadget j sweeps a w_j-subset S of its progression
U_j between position classes 0 and 1 (mod E); distinct gadgets use
disjoint *position slots* within these two shared classes, so
weight-constancy (all class-0 positions weigh 1·, all class-1
positions weigh B· modulo every q_p, since ord | E) and disjointness
coexist. All other digits are frozen arbitrarily. By the stride
lemma the swept sum is C_j + S_j·σ_j with σ_j ranging over
w_j² + 1 ≥ Q_j/V_j consecutive integers, so

```
N ≡ const + σ_j · S_j(1−B)   (mod Λ).
```

Valuations of δ_j = S_j(1−B): for p ∉ G_j,

```
v_p(δ_j) = (a_p − min(a_p, v_p)) + v_p ≥ a_p,
```

so the sweep is **invisible** mod q_p. For p ∈ G_j,
v_p(δ_j) = v_p(B−1) capped by nothing new, so
gcd(δ_j, Q_j) = V_j and δ_j has additive order Q_j/V_j in Z_{Q_j}
(E1 phrasing). Hence σ_j·δ_j covers the subgroup V_j·Z_{Q_j}
entirely while fixed modulo every other group. The k sweeps are
supported on disjoint digits and disjoint position slots, hence
independent; the product of the k coset coverages is the V-coset of
Z_Λ, and by containment the frozen constant already lies in the
correct coset. ∎

### Remarks

1. **Every prior hypothesis is gone.** p | B−1, p = 2, order-1
   components (absorbed into V by the cap), and entangled order
   sets are all handled uniformly. The {2, 6} deadlock that blocked
   Theorem 2 dissolves: tune the order-6 layer with a position-side
   compound move, and the order-2 layer with a digit-stride gadget
   (invisible to the order-6 primes on the digit side). Sol's E4
   counterexample is this same digit-side mechanism arising
   naturally as an obstruction in stride-restricted digit sets.

2. **[Downgraded per A3/A4.]** In the *generalized
   interval-coefficient model* (coefficient sets A ⊆ {1,…,M} of
   unbounded size, weights B^i mod Λ), every fixed pair (B, Λ) has an
   explicit finite saturation bound beyond which the invariant
   congruence is exactly sufficient. Within the radix-digit model the
   conclusion applies only when B−1 is large enough to contain the
   required gadgets — and in the A113028 family Λ grows with B, so
   fixed-Λ saturation gives no uniform information near m*(Λ).
   (The k = 1 case already followed from corrected Theorem 1′; the
   new content is the isolation gadget and the partition/stride
   tradeoff.) Nothing here rules out hardness of the interval family:
   NP-hardness could live below the saturation bound, or in the
   Λ-grows-with-B regime — which is the case of interest. For the
   actual interval family, nothing is proved in either direction.

3. **The strides are the price.** S_j ≈ Λ/Q_j makes the digit-AP
   requirement M ≳ 2w_j·S_j astronomically deep for many-prime Λ.
   Empirically (below and in JOINT-COVERAGE.md) images fill at
   coupon-collector depth; the gap is the analytic problem (sieve /
   resonance route in OPEN-PROBLEM.md).

4. **Group choice is free.** Coarser partitions trade smaller
   strides against larger per-group sweeps (w_j ≈ √(Q_j/V_j));
   k = 1 recovers corrected Theorem 1′ with Q = Λ. Optimizing the
   partition is an easy lever if anyone wants tighter m₀ bounds.

### Additional exact-coverage data (exact class-partition DP, A = {1,…,m})

[A6: these depths do not satisfy Theorem 2″'s AP/slot hypotheses, so they
are evidence for early coset fill, not exercises of the construction. The
example that exercises the construction is ERRATA-DIGIT-GCD.md Part C.]

Prediction: image contained in the invariant coset at every depth,
equal to it above some fill depth. "coset" = Λ/V.

| case | Λ | comps | E | V | coset | result |
|---|---|---|---|---|---|---|
| B=10 | 297 | 27·11 | 6 | 9 | 33 | contained ∀m; exact for all m ≥ 7 |
| B=7 | 288 | 9·32 | 12 | 6 | 48 | contained ∀m; exact for all m ∈ [6,14] |
| B=10 | 21 | 3·7 (order-1 comp) | 6 | 3 | 7 | contained ∀m; exact for all m ∈ [4,9] |
| B=10 | 2079 | 27·11·7 (orders {3,2,6}) | 6 | 9 | 231 | contained ∀m; exact for all m ≥ 7 |

The order-1 row exercises the E1 cap (V = 3, not 9); the last row is
the entangled {2,3,6} order set with a nontrivial 3-part in V.

Errata verifications (Part I): B=3, Q=8, A={1,3} → image exactly
{2, 6} (E2); B=49, Q=800, A = odd digits 1..27 → image exactly 25
residues = 800/gcd(800, 2·48), one full coset of 32·Z₈₀₀ (E4).

### Suggested repo actions (for the agent)

1. Apply E1–E5 as patches to THEOREM-1-PRIME.md (statement of V,
   ℓ ≥ 2 clause, additive-order phrasing, Engine C action list,
   Remark 2 exponent). Credit: external review, 2026-07-22.
2. Add this file; in OPEN-PROBLEM.md's partial-progress list, note
   that the qualitative form of Conjecture A is proved (Remark 2)
   and that open content is now purely quantitative (m₀ vs m* + c)
   plus Conjecture B.
3. Engine C: implement the *skip* rule with local (m, ℓ) per E3/E4;
   root-only invariant check; do not cache fill depths across hole
   patterns. Optional: certify fill depth for the finitely many hole
   patterns with |R| ≤ C₀ actually reachable at a node, by DP, if
   caching is wanted.
4. Open items: (a) quantitative m₀ — coupon-collector-rate coverage
   (sieve/resonance program); (b) Conjecture B refutation-volume
   experiments with corrected |T_m|; (c) second moment across the 47
   instances (mind the complement involution, JOINT-COVERAGE.md E3,
   and now also stride sub-invariants for hole patterns that create
   common digit strides).

### Verification script

```python
import itertools, math

def ordq(B, q):
    x, e = B % q, 1
    while x != 1:
        x = x*B % q; e += 1
    return e

def vP(n, p):
    v = 0
    while n % p == 0:
        n //= p; v += 1
    return v

def image(B, digits, E, Q):
    m = len(digits)
    ks = [sum(1 for i in range(m) if i % E == r) for r in range(E)]
    wts = [pow(B, r, Q) for r in range(E)]
    dp = {tuple(ks): {0}}
    for d in digits:
        nxt = {}
        for caps, res in dp.items():
            for r in range(E):
                if caps[r]:
                    nc = list(caps); nc[r] -= 1; nc = tuple(nc)
                    a = d * wts[r] % Q
                    nxt.setdefault(nc, set()).update(
                        (x + a) % Q for x in res)
        dp = nxt
    (img,) = dp.values()
    return img

def test(B, primepowers, mrange):
    Q = math.prod(primepowers)
    V, E = 1, 1
    for q in primepowers:
        p = min(f for f in range(2, q + 1) if q % f == 0)
        a = vP(q, p)
        V *= p ** min(a, vP(B - 1, p))
        E = math.lcm(E, ordq(B, q))
    print(f"B={B} Lambda={Q} E={E} V={V} coset={Q // V}")
    for m in mrange:
        img = image(B, list(range(1, m + 1)), E, Q)
        s = sum(range(1, m + 1))
        contained = all(t % V == s % V for t in img)
        print(f"  m={m}: {len(img)}/{Q // V} contained={contained} "
              f"exact={len(img) == Q // V}")

test(10, [27, 11], range(5, 12))
test(7, [9, 32], range(6, 15))
test(10, [3, 7], range(4, 10))
test(10, [27, 11, 7], range(6, 13))

# Errata checks
B, Q = 3, 8
imgs = {sum(d * pow(B, p, Q) for d, p in zip([1, 3], perm)) % Q
        for perm in itertools.permutations(range(2))}
print("E2:", sorted(imgs))          # expect [2, 6]
odd = list(range(1, 28, 2))
img = image(49, odd, 2, 800)
print("E4:", len(img), "expect", 800 // math.gcd(800, 2 * 48))
```

## Status

Theorem 2″ and the digit-stride lemma: elementary, believed correct
as stated, not refereed. Errata E1–E5: adopted from external review
("Sol", 2026-07-22), all counterexamples reproduced by exact
enumeration. Scripts stdlib-only, Python 3. Session date 2026-07-22.
