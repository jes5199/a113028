# Better arrangement-search algorithms (design doc)

## The residual problem

Subset selection is solved: number theory forces the digit multiset D (k
distinct nonzero base-B digits) and L = lcm(D). The wall is:

> Find the LARGEST base-B number using each digit of D exactly once with
> value ≡ 0 (mod L).

Max-lex permutation under a modular constraint. Both current engines are
exponential in the wrong regime (b40/b48/b49 all >1h; v4's 1.65× constant
factor is irrelevant to the growth).

## The divergence-depth law (empirically validated)

Order-1 prime powers (q | gcd-structure with B ≡ 1 mod q) constrain only the
digit SUM — permutation-invariant, satisfied by construction once the subset
is fixed. Let **L_eff = L / (order-1 part)**: the modulus that actually
constrains the *arrangement*.

Permuting the last m positions gives ~m! values; these hit a target residue
mod L_eff iff m! ≳ L_eff. So the answer should agree with the descending
arrangement except in its last **m\* ≈ min{m : m! ≥ L_eff}** positions.

Tested against every b-file answer (script in repo history): |m_actual −
m_pred| ≤ 2 for all bases EXCEPT:

| base | m_actual | m_pred | status |
|------|----------|--------|--------|
| 46 | 26 | 19 | **published value proven suboptimal** (our correction: m=20 ✓) |
| 40 | 18 | 15 | unverified (baseline timeout) — **suspect** |
| 48 | 23 | 20 | unverified (baseline timeout) — **suspect** |

The one confirmed b-file error is exactly the largest outlier. Prediction:
the published a(40) and a(48) may also be suboptimal. Engine C (below) will
settle both.

## Cost anatomy of the current engines

Let P = ⌈log_B L⌉ (for b49: P≈12, b40: P≈9). Engine S's prefix-skip cannot
skip *between* consecutive multiples of L once the varying window is below
position P — it must visit essentially every multiple with a valid mid-prefix,
each costing u128 arithmetic. Engine D has no leaf shortcut (brute-forces T!
tails) and, on weak-structure bases, no prefix pruning at all (ne2=0 at b40).
Both pay exponential cost in the band between m\* and P.

## Candidate algorithms

### A. Engine C — prefix DFS + CRT suffix-feasibility pruning + residue-leaf decoding  ← CHOSEN

MSB-first DFS over positions k−1 … P only (never below P). At each node,
prefix residues per tracked prime power are maintained incrementally (as
Engine D does today). Two additions:

1. **Leaf shortcut at depth P.** The remaining suffix is a P-position window
   holding the remaining digit set W. The required suffix value is fixed:
   v ≡ −(prefix·B^P) (mod L), 0 ≤ v < B^P — at most ⌈B^P/L⌉ candidates
   (b49: ≤1, b40: ~15). For each candidate, decompose v into base-B digits
   (O(P)) and test digit-multiset == W by bitmask equality. No tail brute,
   no multiple scan: the entire bottom of the tree is O(P) per leaf.
2. **Sound suffix-feasibility pruning at every node.** For each tracked q
   with small multiplicative order e = ord_q(B): remaining positions 0..m−1
   fall into e weight classes with known counts; exact partition-DP
   (existing `e2_check`/`order_e_check` machinery, target = residual) decides
   whether the remaining multiset can hit the required residue mod q.
   Sound relaxation: prunes only provably dead prefixes. Order-1 part: single
   modular sum check. Apply for e ≤ ~6 and small q (cost per node: microseconds);
   large-order q's are left to the leaf check (they are near-never infeasible
   at m ≫ e anyway — that's WHY they don't prune).

Correctness: pruning is sound + leaf enumeration is exact + descent is
lexicographic ⇒ first hit is THE maximum. Fully deterministic — the entire
budget-doubling/engine-alternation machinery disappears.

Complexity model: nodes ≈ arrangements of the band positions k−1…P that
(a) lie lex-above the answer and (b) survive the relaxations. Raw band size
for b49 ≈ 21!/12! ≈ 4e10; order-1 factor (48) and order-2 exact DP (mod
32·25) plus e=3 (9) realistically cut 10²–10⁴ → 1e6–1e9 nodes at ~1µs
each ⇒ **seconds to ~20 minutes** where baseline needed >1h with no result.
b40: raw ≈ 17!/9! ≈ 8.8e9, weaker pruning (no e2; e3 via 9, e6 via 7,
order-1 39) → similar range. Parallelizes trivially over top branches later.

### B. Per-q feasibility pruning bolted onto existing Engine D (fallback)

Same oracle, no leaf shortcut. Strictly dominated by A (the T!-tail remains).
Only rationale: minimal diff. Skip unless A's implementation stalls.

### C. Meet-in-the-middle residue hashing — REJECTED

Split the m\*-endgame in half and hash partial residues: the "halves" are
injections (choose digits AND arrange), m!/(m/2)! ≈ 2.8e13 for m=22 — no
subset-sum-style factorization exists for permutations. Dead by counting.

### D. CP-SAT / ILP endgame baseline — tooling only

Encode one endgame instance (fixed prefix, m free positions, CRT constraints)
for OR-tools as an independent cross-check of Engine C on single instances.
Not a production path (max-lex objective + huge L handled poorly).

## Is Engine C near-optimal? (analysis, 2026-07-22)

**1. The top descent is already free.** Above the divergence band, the DFS
tries the largest available digit first and (by the density argument m! ≫
L_eff) that choice essentially always has a completion — and no *larger*
choice exists to refute, because the descending arrangement is the lex-max of
all arrangements. So Engine C walks the forced prefix once, O(k) — seed #1
("skip the descent") is already achieved; there is no slack there.

**2. The irreducible core is critical-band refutation.** All real work is
refuting wrong turns at levels m ∈ [m*−c, m*+c] (c ≈ 2–3; above the band
wrong turns don't exist, below it per-q DPs bite). A wrong turn there is
lex-larger than the answer but has no completion, while *every cheap per-q
test passes* — necessarily: if a completion existed, that wrong turn would BE
the answer. Refutation currently means sweeping the subtree at per-q-feasible
granularity down to the P-leaf.

**3. Could an exact joint oracle kill the sweeps?** With a poly-time exact
completion-feasibility oracle, greedy descent would never backtrack: total
cost O(k·B·oracle). But the joint problem — a bijection of digits to
positions satisfying independent class-partition constraints per prime power
simultaneously — is a CRT-coupled assignment problem; the coupling across
moduli is exactly what the per-q relaxations lose, and positions do NOT
collapse into equivalence classes (a position's type is its residue vector
(i mod e_1, …, i mod e_r); for hard bases lcm(e_i) ≫ m, so all types are
distinct). This is 3-partition/exact-matching-flavored; we conjecture the
band decision is NP-hard in general (no reduction written down — open), and
in the band it sits at density Θ(1) where relaxation-based certificates
provably cannot decide. MITM re-dies in the band for the same reason it died
globally: halves are injections (choose digits AND arrange), 21!/10! ≈ 1e13.
Verdict: **Engine C has the right shape; the band sweep is (morally)
irreducible.** Remaining slack is multiplicative, not asymptotic:

- **(a) Incremental in-band DPs (biggest lever, est. 10–100× on structured
  bases).** v6 dropped e=4..6 node pruning because recomputing each DP from
  scratch costs ~73µs/node. But sibling nodes share 95% of the DP work —
  maintained incrementally (update tables as digits are placed/unplaced),
  ALL small-e moduli become affordable in-band, multiplying the pruning
  factor by the dropped q_e's (b49: ×11 (e=5), ×13 (e=6), …).
- **(b) Leaf widening Δ (est. 2–5×).** At depth P+Δ, enumerate the ≤
  B^Δ·(B^P/L) candidate values by decomposition (O(P) each) instead of
  DFS-ing the last Δ levels; breakeven Δ ≈ 2–3.
- **(c) Parallelism (×cores).** Band wrong-turn subtrees are independent;
  trivial work-stealing across them.
- **(e) Algebraic refutation (Fourier certificates) — investigated, fails
  exactly where needed.** The completion count is N(t) = (m!/L)·(1 +
  Σ_{a≠0} ω^{−at}·F(a)) where F(a) are Fourier coefficients of the
  permutation-sum distribution (permanental averages, boundable via
  row-product/Brégman-style estimates). If the bounded tail is < 1, we get
  N(t) > 0 (feasible) or N(t) < 1 (refuted) WITHOUT any search — a
  certificate oracle. But in the critical band the density is Θ(1) and the
  Fourier tail is Θ(1) too, so the bounds cannot separate 0 from 1 there.
  This independently confirms the band's irreducibility: any oracle whose
  power comes from concentration (relaxations, Fourier, counting bounds)
  fails precisely on the band decisions. Useful nonetheless as a cheap
  *above-band* certificate replacing the heuristic gate.
- **(d) A possible theorem, worth pursuing:** an EGZ/Cauchy–Davenport-style
  coverage result ("for m ≥ f(q), permutation sums mod q with ≥2 distinct
  weights and ≥2 distinct digits cover all residues") would make the
  above-band always-feasible claim *provable*, sharpening band edges and
  removing heuristic slack from the gate. Doesn't remove the band itself.

**Bottom line for jes:** Engine C is near-optimal in shape; expect another
1–2 orders of magnitude from (a)+(b)+(c) engineering, but no poly-time
algorithm for the band unless the NP-hardness intuition is wrong. The
divergence-depth law bounds the intrinsic difficulty of any base: work ≈
band sweeps of width m*(B) — bases with small L_eff relative to k! stay easy
forever; bases where m* creeps toward k get exponentially harder for ANY
algorithm of this family.

## Implementation plan (v5)

- v5 = v3 source + Engine C replacing `hybrid()`; keep old engines behind a
  CLI flag for cross-checking.
- Validation ladder: (1) full 2–39 regression vs b-file; (2) 41–45,47 vs
  b-file; (3) 46 must reproduce OUR corrected value (not the b-file's);
  (4) b49 vs jes's independent answer — the oracle test; (5) then b40/b48
  fresh solves → settle the suspect published values.
- Benchmark wall-clocks throughout; the b49 number is the headline (jes: ~1
  week on a 2020 laptop).
