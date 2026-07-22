# The Bridge Theorem and the hardness baseline

*(Companion to OPEN-PROBLEM.md and the theorem files. Two results,
session 2026-07-22. Part I proves the **Bridge Theorem**: under a
single named hypothesis NC (non-clumping), the refutation volume of
the search equals an explicit product over the measurable
survivor-density profile — making Conjecture B equivalent, modulo
NC, to a decay property of numbers the agent can compute. A
measurement protocol is included. Part II proves that PERM-FEAS with
**unrestricted** digit sets is NP-complete already for a single
prime modulus with ord_q(B) = 2, with the padding construction
written out in full; hence any hardness of the interval family must
flow through the interval structure. The reduction's gadget equation
is verified by brute force below.)*

## Part I — The Bridge Theorem

### Setting

Fix the base. L_eff and V = gcd(L_eff, B−1) are constants of the
whole search. A search node at level m carries a residual digit set
A′ (an interval minus at most C₀ holes, |A′| = m) and a residual
target t ∈ Z_{L_eff}; the node **survives** if t ∈ T(A′), the fiber
of the cheap conditions (joint cyclotomic-layer conditions; see
OPEN-PROBLEM.md and THEOREM-1-PRIME.md). A child places some digit
d ∈ A′ at position m−1, producing residual set A′∖{d} and target
t − d·B^{m−1}. [Corrected per ERRATA-DIGIT-GCD.md D1/A5: reachable
residual sets do NOT stay interval-minus-C₀ — successive nonmaximal
choices create arbitrarily many internal gaps. 𝒜_m below is defined
as the *reachable* residual sets, whatever their shape, and T̄_m
majorizes those; nothing in the theorem depends on a shape claim.]

Let 𝒜_m denote the residual sets reachable at level m and

    T̄_m = max_{A′ ∈ 𝒜_m} |T(A′)|.

The **refutation volume** of a node is the number of surviving
descendants the search visits before certifying infeasibility.
(Non-surviving children are touched and rejected at cost
O(m·poly(B)) per surviving parent; this is absorbed into the poly
factors throughout.)

All fibers live inside the ambient invariant coset of size L_eff/V
(THEOREM-1-PRIME.md part (a), as corrected; the coset base point
shifts consistently from parent to child since the placed digit
moves both the target and the residual digit sum by d mod V).

### Hypothesis NC (non-clumping)

There is a constant K ≥ 1 such that for every surviving window node
(A′, m, t):

    #{ d ∈ A′ : t − d·B^{m−1} ∈ T(A′∖{d}) }
        ≤  K · ( 1 + m · V · T̄_{m−1} / L_eff ).

In words: the number of surviving children never exceeds K times
what uniform scattering of the children's targets over the ambient
coset predicts, plus integer slack. This is the *entire* unproved
content of the runtime claim; everything else below is a theorem.

### Bridge Theorem

**Theorem.** Under NC, the refutation volume of any node at level
m₀ is at most

    Vol  ≤  Σ_{k ≥ 0}  Π_{i < k} β_i,
    β_i := K · ( 1 + (m₀ − i) · V · T̄_{m₀−i−1} / L_eff ).

**Proof.** Let N_k be the number of surviving nodes k levels below
the root; N₀ = 1. A surviving node at depth i sits at level
m = m₀ − i with some reachable residual set, so NC bounds its
surviving children by β_i (T̄ majorizes every reachable fiber at
that level). Induction gives N_k ≤ Π_{i<k} β_i, and the volume is
Σ_k N_k. ∎

### Corollary (Conjecture B, conditional and quantitative)

Suppose the profile decays below a burn-in width c:

    β_i ≤ γ < 1   for all i ≥ c.

Then

    Vol ≤ ( Π_{i<c} β_i ) · (1−γ)^{−1} ≤ (2K m₀)^c / (1−γ)
        = B^{O(c)} · poly(B),

since β_i ≤ K(1+m₀) in the burn-in and m₀ ≤ B. This is exactly
Conjecture B's bound.

**Converse direction.** If NC also holds in lower-bound form
(surviving children ≥ K^{−1}·(m·V·T_{m−1}/L_eff) whenever that
quantity exceeds a constant, for the *actual* reachable fibers),
the same induction gives Vol ≥ K^{−depth}·Π(·) — the volume is
pinned to the profile product within K^{O(window width)}. Hence:

> **Modulo NC, Conjecture B is equivalent to a decay property of
> the measurable profile m·V·T̄_{m−1}/L_eff.**

The theorem is deliberately elementary: its purpose is to move all
uncertainty into one named, testable hypothesis and out of the
runtime claim. The logical chain is now

    counting form ⟹ NC ⟹ (Bridge) volume = profile product
                 ⟹ B^{O(c)}·poly runtime,

with NC the only unproved link.

### Measurement protocol (for the agent)

Decision-grade experiment; requires complete sweep logs for at
least two bases with finished refutations.

1. **Fiber profile.** For each level m in the window, compute
   |T(A′)| by the joint cyclotomic-layer DPs for every residual set
   actually occurring in the sweep logs [D2 wording] — never the
   interval substitute. Record per-node g(A′) and
   H = gcd(Λ, (B−1)·g) alongside |T(A′)|: stride sub-invariants
   (ERRATA-DIGIT-GCD.md Part B) are expected to be a major source of
   fiber shrinkage at gap-scattered nodes, and the H-condition
   belongs among the cheap conditions being profiled.
2. **K.** For every surviving node in the logs, record actual
   surviving-children count vs the predictor
   m·V·|T(A′∖·)|/L_eff (use the actual child fibers, not T̄, for
   the tightest K). Report the distribution of the ratio; K is its
   upper envelope, and clumping — NC violation — shows up as
   heavy-tailed ratios localized at specific levels or hole
   patterns. A violation is a finding, not a failure: it says
   residual targets correlate with fiber structure, and where.
3. **γ and c.** From the profile, locate the level where β drops
   below 1 and its decay rate; report c (burn-in width above it)
   and γ.
4. **Validation.** Compare Σ_k Π β_i against actual per-root node
   counts. Agreement within K^{O(c)} across bases calibrates
   Conjecture B empirically and prices every proposed
   cheap-condition upgrade in advance: a candidate condition's
   value is exactly its effect on the T̄ profile.

## Part II — Unrestricted PERM-FEAS is NP-complete

**Theorem.** PERM-FEAS(B, A, Λ, t) with an arbitrary set A of
distinct nonzero digits < B is NP-complete, already with Λ = q a
single prime and ord_q(B) = 2.

**Membership** in NP: the arrangement is the certificate.

**Gadget.** Choose q an odd prime and B = q − 1 ≡ −1 (mod q), so
B^i ≡ (−1)^i and, with s₀ the sum of digits in even positions
(a class of size ⌈m/2⌉ out of m),

    N(σ) ≡ s₀ − (ΣA − s₀) = 2·s₀ − ΣA   (mod q).

With q larger than twice the sum of all digits, integer arithmetic
is exact and 2 is invertible, so feasibility for target t is
*exactly*: does some ⌈m/2⌉-subset of A have integer sum
(t + ΣA)/2? — exact-size subset sum.

**Hardness chain, in full.**

*Step 1 (size-k subset sum is NP-hard).* From SUBSET SUM
(x₁,…,x_n distinct, target τ; distinctness is WLOG by the standard
scaling x_i ↦ (n+1)x_i + i, τ ↦ (n+1)τ + Σ_{i∈S} i handled by
trying the polynomially many index-sums, or by any standard
distinct-SUBSET-SUM formulation): shift y_i = x_i + D₁ with
D₁ > Σ_i x_i. A subset of the y's sums to τ + kD₁ iff it has
exactly k elements with x-sum τ, since the low block (< D₁) and the
D₁-count cannot interfere. Trying each k ∈ [0, n] gives EXACT-SIZE
SUBSET SUM: (y's, k, τ + kD₁).

*Step 2 (force the size to be ⌈m/2⌉).* Given the exact-size
instance (y₁,…,y_n, k, τ′), add n dummy digits

    z_j = D₂ · 2^j,   j = 1, …, n,
    D₂ > 2n · (D₁ + max_i x_i),

so m = 2n and the forced class size is ⌈m/2⌉ = n. Set the class
target

    τ* = τ′ + Σ_{j=1}^{n−k} z_j,

i.e. designate dummies 1,…,n−k as chosen. Correctness: real digits
total < D₂, and dummy sums have unique binary representations in
the D₂-blocks, so any n-subset with sum τ* must contain exactly the
designated dummies (n − k of them) and hence exactly k reals with
y-sum τ′ — which by Step 1 means k originals summing to τ.
Conversely a valid original solution assembles τ* directly. All
digits are distinct and positive; bit-lengths are polynomial; take
q prime with q > 2·(sum of all 2n digits) and q > max digit + 1 so
every digit is a valid nonzero digit below B = q − 1. Finally set

    t ≡ 2τ* − ΣA   (mod q).  ∎

**Consequence [corrected per ERRATA-DIGIT-GCD.md D3].** The
anti-conjecture of OPEN-PROBLEM.md cannot be "PERM-FEAS is hard in
general" — that is now a theorem. The hard instances above use
adversarial digit *sets*; for the interval family nothing is proved
in either direction.

### Gadget verification (brute force, k = ⌈n/2⌉ case)

A = {3, 5, 8, 9}, forced class size 2; q = 71, B = 70:

| τ | 2-subset with sum τ exists | PERM-FEAS | agree |
|---|---|---|---|
| 13 | yes ({5,8}) | YES | ✓ |
| 17 | yes ({8,9}) | YES | ✓ |
| 8 | yes ({3,5}) | YES | ✓ |
| 6 | no | NO | ✓ |
| 7 | no | NO | ✓ |

```python
import itertools, sympy

def perm_feas(B, A, q, t):
    for perm in itertools.permutations(range(len(A))):
        if sum(d * pow(B, p, q) for d, p in zip(A, perm)) % q == t % q:
            return True
    return False

def gadget(xs, tau):
    S = sum(xs)
    q = sympy.nextprime(2 * S + max(xs) + 10)
    return q - 1, q, (2 * tau - S) % q

xs = [3, 5, 8, 9]
for tau, expect in [(13, True), (17, True), (8, True),
                    (6, False), (7, False)]:
    B, q, t = gadget(xs, tau)
    assert perm_feas(B, xs, q, t) == expect
print("all gadget checks pass")
```

## Suggested repo actions (for the agent)

1. Add this file; in OPEN-PROBLEM.md, restate Conjecture B as "NC +
   profile decay" citing the Bridge Theorem, and replace the
   informal anti-conjecture discussion with: general PERM-FEAS is
   NP-complete (Part II); the open hardness question concerns the
   interval family at threshold scale only.
2. Run the measurement protocol (Part I, steps 1–4) on every base
   with complete sweep logs; report K, γ, c, and predicted-vs-actual
   volumes. This is the highest-value computation in the program:
   it either certifies the runtime model modulo NC or localizes the
   clumping.
3. Price the pending cheap-condition upgrades (joint-layer DPs at
   e = 4, 5; full-support scanner) by their effect on the T̄
   profile before implementing them in Engine C.

## Status

Bridge Theorem: elementary given NC; believed correct as stated;
not refereed. NP-completeness: standard blocking reduction, gadget
verified by exact enumeration above. Scripts Python 3 (sympy for
the prime search only). Session date 2026-07-22.
