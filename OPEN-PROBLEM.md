# An open problem: the A113028 threshold conjecture

*(Permutation zero-sums for interval digit sets under geometric weights.
If you recognize this as known, or see how to attack it, please open an
issue — resolving it in either direction settles the computational frontier
of OEIS A113028.)*

## Setup

Fix a radix B ≥ 3 and let A ⊆ {1, …, B−1} be a set of m distinct nonzero
"digits". For a bijection σ : A → {0, 1, …, m−1} (an *arrangement* — each
digit gets a distinct position) define

    N(σ) = Σ_{d ∈ A} d · B^{σ(d)}.

For a modulus Λ and target t ∈ Z_Λ, the **feasibility problem** is:

    PERM-FEAS(B, A, Λ, t):  does some arrangement σ satisfy N(σ) ≡ t (mod Λ)?

The instances arising from A113028 have special structure:

1. **A is an interval minus a bounded set**: A = {1,…,M} \ R with |R| ≤ C₀
   for a small constant C₀ (the search that generates these instances places
   the large digits first, so the *remaining* set is always of this shape).
2. **Λ = L_eff**: start from L = lcm(D) of the full digit set D, discard the
   prime powers q with B ≡ 1 (mod q) (for those, N(σ) ≡ Σd is
   arrangement-invariant), and the parts with gcd(q, B) > 1 (they constrain
   only a bounded suffix). What remains is a product of prime powers q, each
   with multiplicative order e_q = ord_q(B) ≥ 2, and ln L_eff ≈ ln L ≈ M
   (Chebyshev).
3. **The weights are geometric**: position i carries weight B^i mod q, which
   depends only on i mod e_q.

Necessary conditions for feasibility, all checkable in polynomial time
("the cheap conditions"): the order-1 congruences (automatic in Λ = L_eff),
the nilpotent last-digit conditions, and for each prime power q of *small*
order e_q ≤ 3, the exact class-partition dynamic program (positions fall
into e_q weight classes with fixed counts; a partition of A's residues into
classes with the right sums must exist — poly-time for bounded e_q).

## The empirical phenomenon

Heuristically N(σ) mod Λ equidistributes, so ≈ m!/Λ arrangements hit each
admissible target. Empirically (all 47 known values of A113028, plus one
independently computed base-49 value) the answer's divergence from the
trivially maximal descending arrangement occurs at depth within ±2 of

    m* = min { m : m! ≥ L_eff },

i.e. **feasibility transitions from rare to certain inside a window of
observed width ≤ ~3 levels** around m*. The transition is sharp and
law-like across 47 heterogeneous instances.

## The conjecture

**Threshold conjecture.** There is a constant c = c(C₀) (absolute, or at
worst slowly growing in B) such that for every instance with the interval
structure above and every target t satisfying the cheap conditions:

    m ≥ m*(L_eff) + c   ⟹   PERM-FEAS(B, A, L_eff, t) = YES.

Equivalently: above a bounded-width window, *the cheap necessary conditions
are sufficient* — the set {N(σ) mod L_eff : σ} is exactly the coset cut out
by the arrangement-invariant congruences.

**Counting form** (stronger, circle-method-shaped): for m ≥ m* + c,
uniformly over admissible t,

    #{σ : N(σ) ≡ t (mod L_eff)} = (m!/L_eff)·(1 + o(1)).

**Anti-conjecture** (equally valuable): PERM-FEAS restricted to these
interval instances with composite Λ is NP-hard. This would prove the
current algorithm is essentially optimal.

## Why resolution breaks (or fixes) the frontier

The best known algorithm ("Engine C", this repo) computes a(B) by a
largest-first descent whose entire cost is *refuting* prefixes that are
lexicographically above the answer but have no completion. All such
refutations live in the transition window; outside it, feasibility is
(empirically but unprovably) determined. Without the conjecture, each
refutation is an exhaustive sweep of size ~m*!/P! where P = ⌈log_B L⌉ —
super-polynomial, ≈ exp(Θ(n·ln ln n / ln n)) at n = B−1, which caps
consecutive computability of A113028 near B ≈ 60–65 forever.

With the conjecture (bounded c): every feasibility question outside the
window is answered by the cheap conditions in polynomial time; refutation
recursion is confined to windows of bounded depth, giving ~B^{O(c)}·poly(n)
per base. **The frontier dissolves** — the sequence becomes computable
essentially as far as one cares to run it.

## Where this lives in the literature (pointers, not claims)

- **Erdős–Ginzburg–Ziv (1961)** and the weighted zero-sum literature
  (weighted Davenport constants, e.g. Adhikari et al.): "dense sequences
  admit zero-sum subsequences" — ours is a *bijection* analogue: every
  weight used exactly once.
- **Complete mappings / Hall–Paige**: bijections with prescribed sums over
  groups. The analytic proof of the Hall–Paige conjecture and the counting
  of additive triples of bijections (Eberhard–Manners–Mrazović and
  subsequent work) count bijections σ of Z_n with Σ i·σ(i)-type constraints
  by the circle method — the closest known technique family. Differences
  here: A is a proper subset (interval-minus-holes, not a group), weights
  are geometric powers B^i (not all of Z_n), and Λ ≫ m is composite with
  mixed orders. The critical regime m! ≈ Λ is exactly where power-saving
  minor-arc bounds would be needed.
- **Cauchy–Davenport / Vosper / Sárközy-type density results**: sumset
  growth and dense-set coverage in Z_p — relevant to showing achievable
  residues expand to everything under interval structure.
- **Exact matching (Mulmuley–Vazirani–Vazirani 1987)**: PERM-FEAS for a
  *single* prime modulus is a weighted perfect-matching-with-congruence
  question on a complete bipartite graph and is plausibly in randomized
  poly-time; the class DP already decides bounded-order cases. The
  conjecture's entire content is the *joint* statement across prime powers
  — the coupling is what the relaxations lose.

## Status

Open, to our knowledge (2026-07). The empirical evidence (47 instances,
window ≤ 3) is in this repository (FRONTIER.md, RESULTS.md); the divergence
law alone has already exposed one wrong published value (a(46)) and flags
two more (a(40), a(48)). Contact: open an issue on this repo.
