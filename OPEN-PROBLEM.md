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

## The conjectures

*(Revised 2026-07-22 after external review: the original single statement
conflated two different claims — coverage, which makes the greedy descent
safe but was already the cheap part, and refutation volume, which is the
actual computational cost. They are separated below. The "cheap conditions"
are also upgraded: per-prime-power marginals are NOT sufficient — the right
conditions are the joint cyclotomic-layer conditions, DP-decidable mod
Q_e = Π{largest p-powers with B^e ≡ 1} for each small e; Q_e | B^e − 1 so
this stays polynomial for bounded e. Base-16 witness: at m = 9 the
marginals mod 9, 7, 13 admit 273 targets while the joint layer mod 819
admits 253 — exactly the true image.)*

**Conjecture A (coverage).** There is a constant c = c(C₀) such that for
every instance with the interval structure above and every target t
satisfying the joint cyclotomic-layer conditions:

    m ≥ m*(L_eff) + c   ⟹   PERM-FEAS(B, A, L_eff, t) = YES.

Above a bounded-width window, the joint layer conditions are sufficient —
{N(σ) mod L_eff : σ} is exactly the set they cut out. (Empirically, with
*joint* layers, sufficiency holds by m* + 2 across exhaustive scans of
bases 7–18; with marginals only, base 16 needs m* + 3.)

**Conjecture B (bounded refutation volume) — the algorithmically decisive
statement.** Every *infeasible* prefix node of the largest-first search has
at most B^{O(c)}·poly(B) descendants that continue to satisfy the joint
cyclotomic-layer conditions.

Conjecture A alone does **not** bound the running time: it certifies that
the descending prefix is safe, which the cost analysis (ALGORITHMS.md)
already showed is the free part. The expensive part is *certifying
infeasible children impossible* — e.g. at base 49, m* = 21, the child that
takes digit 21 is infeasible, and refuting it is the entire search cost.
Conjecture B is what implies the poly(B) per-base bound; A is what makes
the surviving branch correct to follow.

**Counting form** (stronger, circle-method-shaped): for m ≥ m* + c,
uniformly over admissible t,

    #{σ : N(σ) ≡ t (mod L_eff)} = (m!/L_eff)·(1 + o(1)).

**Refinement (see JOINT-COVERAGE.md).** When E = lcm{e_q : q ∥ L_eff}
satisfies E ≤ m, N(σ) mod L_eff factors through the position-class
partition, and the correct first-moment count is the class-partition number
P(m, E) = m!/Π_r k_r! (k_r the class sizes), not m!. The honest general
statement is therefore

    P(m, E) ≥ L_eff · ln L_eff   ⟹   PERM-FEAS(B, A, L_eff, t) = YES

for targets satisfying the cheap conditions, with the counting form
#{σ : N(σ) ≡ t} = (P(m,E)/L_eff)·(1+o(1)). In genuine A113028 instances
E ≫ m (orders of many prime powers), every class is a singleton, P(m,E) =
m!, and the m* form above is unchanged — but an analytic attack must count
class partitions, and the ln L_eff coupon-collector factor is what makes
the observed window width c ≈ 1–2 (one depth level multiplies the count
by ≈ m, while the required oversampling is only ln L_eff).

Two formal corrections (external review, 2026-07-22): (i) at the frontier
the *remaining* interval has top M′ ≍ m ≍ B/ln B while ln L_eff ≍ B — the
symbol M above refers to the original digit ceiling B−1, not the remaining
interval's top; conflating them describes the wrong asymptotic family.
(ii) When the cheap conditions cut out a proper subset T_m of the quotient,
the correct counting main term is m!/|T_m| (or P(m,E)/|T_m|), not
m!/L_eff — the two agree only once T_m fills the effective quotient.

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
- **The distinct-coordinate sieve (Li–Wan; see also Li–Yu)**: for an
  additive character χ, the Fourier coefficient of N(σ) over bijections has
  the exact cycle expansion  μ̂(χ) = (1/m!)·Σ_{τ∈S_m} (−1)^{m−c(τ)}
  Π_{C∈cycles(τ)} S_A(Σ_{i∈C} B^i),  with S_A(u) = Σ_{d∈A} χ(ud) an
  explicit geometric sum plus O(C₀) hole terms for interval A. Large
  Fourier mass therefore requires many cycles resonating,
  Σ_{i∈C} B^i ≡ 0 (mod q) — turning the analytic attack into a
  combinatorial lemma about zero-sum subsets of a geometric orbit, which
  proliferate exactly in the low-order cyclotomic layers the joint DPs
  already handle. This route is far closer to the statistic here than a
  generic circle-method permanent bound. See also Nagy, *Permutations over
  cyclic groups* (the "braid trick" for how transpositions move
  permutational sums) and the recent Littlewood–Offord theory on S_m
  (anti-concentration of Σ w_i v_{π(i)}, currently at polynomial rather
  than 1/L_eff scale).

## Status

Open, to our knowledge (2026-07). The empirical evidence (47 instances,
window ≤ 3) is in this repository (FRONTIER.md, RESULTS.md); the divergence
law alone has already exposed one wrong published value (a(46)) and flags
two more (a(40), a(48)). Contact: open an issue on this repo.

Partial progress in this repository:

- **SINGLE-MODULUS.md** — proved: single-modulus feasibility for interval
  digit sets is unobstructed (every target achievable) for q with
  gcd(q, B(B−1)) = 1 under the depth condition w² ≥ q−1.
- **JOINT-COVERAGE.md** — proved: joint coverage across several prime
  powers with independent orders (mutually invisible exchange gadgets) —
  the first rigorous statement about the coupling; plus exact-enumeration
  evidence that single- and joint-modulus thresholds are pure
  coupon-collector with no structural obstruction found anywhere (scripts
  in `scripts/`, raw outputs in `evidence/`), and the P(m,E) correction to
  the first-moment count adopted above.
