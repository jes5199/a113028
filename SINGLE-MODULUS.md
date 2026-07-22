# Single-modulus sufficiency for interval digit sets

*(Companion to OPEN-PROBLEM.md. This note proves that for a single odd
prime power q with ord_q(B) ≥ 2 and gcd(q, B(B−1)) = 1, the feasibility
problem mod q is not merely poly-time decidable but **trivial**: under an
explicit depth condition, every target is achievable. This extends the
exact class-partition DP from orders e_q ≤ 3 to all orders, and localizes
the open problem entirely in the coupling across prime powers.)*

## Setting

As in OPEN-PROBLEM.md: radix B ≥ 3, digit set A = {1, …, M} \ R with
|R| ≤ C₀, m = |A|, positions {0, 1, …, m−1}, and for an arrangement
(bijection) σ : A → {0, …, m−1},

```
N(σ) = Σ_{d ∈ A} d · B^{σ(d)}.
```

Fix a prime power q = p^j with p odd, p ∤ B, p ∤ B−1, and let
e = ord_q(B) ≥ 2.

## Lemma (fixed-size subset sums of an interval are an interval)

Let U = {c+1, …, c+n} be n consecutive integers and 1 ≤ a ≤ n. Then

```
{ Σ_{x ∈ S} x  :  S ⊆ U, |S| = a }
```

is a complete interval of consecutive integers, of width a(n−a).

**Proof.** The minimum is the bottom block {c+1, …, c+a}, the maximum the
top block, and their difference is a(n−a). If S attains less than the
maximum, S is not the top block, so there exists x ∈ S with x+1 ≤ c+n and
x+1 ∉ S; the exchange S ↦ (S \ {x}) ∪ {x+1} increases the sum by exactly 1.
Iterating from the minimum reaches every intermediate value. ∎

(With O(1) holes removed from U the same exchange argument gives an
interval minus a bounded set near the endpoints; we avoid even that by
working inside a hole-free run below.)

## Theorem (single-modulus coverage)

Since |R| ≤ C₀, the set A contains a run of

```
ℓ = ⌈(M − C₀) / (C₀ + 1)⌉
```

consecutive integers (pigeonhole: the ≤ C₀ holes cut {1,…,M} into at most
C₀+1 runs). Put

```
w = min( ⌊m/e⌋, ⌊ℓ/2⌋ ).
```

**Theorem.** If w² ≥ q − 1, then for **every** t ∈ Z_q there is an
arrangement σ with N(σ) ≡ t (mod q). In particular, mod q there is no
obstruction at all: the cheap conditions are (vacuously) sufficient.

**Proof.** Position i carries weight B^i ≡ B^{i mod e} (mod q), so N(σ)
mod q depends only on the induced partition of A into e position-classes:

```
N(σ) ≡ Σ_{r=0}^{e−1} B^r · s_r  (mod q),
```

where s_r is the sum of the digits assigned to class r, and the class
sizes k_r = #{ 0 ≤ i < m : i ≡ r (mod e) } are determined by m and e,
with each k_r ≥ ⌊m/e⌋ ≥ w. Conversely, every partition of A into classes
of sizes (k_0, …, k_{e−1}) is induced by some arrangement (assign each
class to its positions in any order).

Choose a hole-free run J ⊆ A with |J| = ℓ and let U be the first 2w
elements of J (possible since 2w ≤ ℓ). Reserve quota w in class 0 and
quota w in class 1 (possible since w ≤ k_0, k_1); fill all remaining
quota of every class arbitrarily from A \ U and freeze that choice. Let
Σ_frozen denote the resulting frozen contribution Σ_r B^r · (frozen part
of s_r), and let T = Σ_{x ∈ U} x.

Now the only freedom is the choice of S₀ ⊆ U with |S₀| = w for class 0,
the complement U \ S₀ going to class 1. Writing s for the sum of S₀:

```
N ≡ Σ_frozen + s·B^0 + (T − s)·B^1
  ≡ (Σ_frozen + T·B) + s·(1 − B)   (mod q).
```

By the Lemma, s ranges over a complete interval of w² + 1 ≥ q consecutive
integers. Since p is odd and p ∤ B−1, the common difference (1 − B) is a
unit mod q, so the values above form an arithmetic progression of length
≥ q with unit difference in Z_q, which covers Z_q entirely. ∎

## Scope and remarks

1. **Excluded moduli.** The hypotheses p ∤ B−1 and p odd exclude exactly
   the moduli whose arrangement-invariant part is nontrivial. For
   p | B−1 with p^j ∤ B−1, the difference 1−B is a non-unit of valuation
   v = v_p(B−1), and the same progression covers precisely the coset of
   p^v·Z_q selected by the invariant congruence N ≡ Σ_{d∈A} d
   (mod p^v) — i.e. the statement becomes "the cheap conditions are
   sufficient" rather than "all targets", as it must. Powers of 2 need
   the analogous bookkeeping at v_2(B−1) and v_2(B+1). Neither case is
   expected to be deep; neither is written out here.

2. **Where the depth condition bites.** w² ≥ q − 1 asks roughly
   m ≥ e·√q and ℓ ≥ 2√q, i.e. m ≳ max(e, C₀+1)·√q. Since q ≤ M and, in
   the A113028 instances, the search operates near m ≈ m* ≈ M/ln M, the
   condition holds throughout the refutation window for every prime
   power with e_q ≲ √M / polylog — in particular for **all** small-order
   moduli, at every size q, unconditionally. It fails when B is close to
   a primitive root mod q for large q (e ≈ q ≈ M): the classes collapse
   toward singletons and the two-class exchange has no room.

3. **The primitive-root regime.** When e is large the situation is not
   hopeless, just different: with many singleton or near-singleton
   classes, the tunable quantities are the pair differences
   (d − d′)(B^i − B^{i′}), and the achievable perturbations form a
   high-rank generalized arithmetic progression in Z_q. Covering Z_q
   with such a GAP is a Cauchy–Davenport / Vosper-type question, not a
   counting triviality; it is the natural next lemma, and the interval
   structure of A should again be the decisive input.

4. **What this does *not* touch.** The proof freezes every class outside
   one two-class exchange, i.e. it discards exactly the degrees of
   freedom that the threshold conjecture is about. The joint statement —
   one permutation satisfying all prime powers simultaneously, within a
   bounded window of m* — remains fully open. What this note contributes
   to it is a reduction: every modulus satisfying the depth condition is
   *individually unobstructed*, so any counterexample to the conjecture,
   and any hardness in the anti-conjecture direction, must live entirely
   in the coupling (or in the large-order regime of Remark 3).

## Algorithmic consequence for Engine C

For each prime power q of L_eff with p ∤ B(B−1) and w² ≥ q − 1, the
feasibility question mod q may be answered YES in O(1) during descent,
for any target, regardless of e_q — these moduli can be dropped from the
joint refutation search outright, exactly as the order-1 moduli are
dropped in forming L_eff. The class-partition DP is then needed only for
the finitely many moduli that fail the depth condition or the gcd
hypotheses. (Care: the drop is valid only while the *remaining* digit
set still satisfies the depth condition; the check is a comparison of
two integers and can be re-evaluated at each node.)

## Status

Elementary; believed correct as stated; not refereed. If the exchange
argument or the unit-difference step fails in some edge case you hit in
practice (small m, huge C₀), the failure will be at the boundary of the
depth condition, not in the interior. Issues welcome on this repo.
