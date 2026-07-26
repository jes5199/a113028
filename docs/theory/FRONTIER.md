# The tractability frontier of A113028

## Reframing (jes's observation, formalized)

Past any real numeral system, "base B" is scaffolding. The problem is purely
arithmetic-combinatorial: let n = B−1 and D ⊆ {1..n} a set of distinct
values ("digits"). A113028(B) asks for the maximum of Σ d_i·B^{σ(i)} over
permutations σ of the (essentially forced) D, subject to lcm(D) | value.
B enters only as the radix of the weight geometric series. Everything below
is naturally stated in n.

## The law

Three quantities control everything (all computable a priori, before any
search):

- **L = lcm(D)** ≈ lcm(1..n) = e^{ψ(n)} ≈ e^n (Chebyshev/PNT), with small
  corrections from the ten-rule and forced removals.
- **L_eff = L / (order-1 part)** — the sub-modulus that actually constrains
  the *arrangement* (order-1 prime powers q | B−1 fix only the digit sum).
- **m\* = min{m : m! ≥ L_eff}** — the divergence depth: the answer equals the
  descending arrangement except in its last ~m\* positions (validated ±2
  against the entire b-file). Asymptotically m\* ≈ n / (ln n − ln ln n).
- **P = ⌈log_B L⌉ ≈ n/ln n** — the leaf horizon: below P positions the
  suffix value is determined mod L and enumerable directly.

Any algorithm of the Engine-C family (and, per the near-optimality analysis
in ALGORITHMS.md, plausibly any algorithm at all) must refute the wrong turns
in the critical band — a sweep of order the falling factorial

**W(B) ≈ m\*!/P!  =  exp(Θ( n·ln ln n / ln n ))**

— *subexponential in n, but far superpolynomial.* The band width m\*−P ≈
n·ln ln n/ln²n grows without bound: the problem gets qualitatively harder
forever, with no plateau.

## Calibration and the table

Raw W underestimates observed cost by 10–100× (wrong-turn multiplicity,
partial pruning inefficiency; measured: b43 model 26s vs 2738s actual; b49
model ~17min vs >1h). Applying that smear to the per-base computation
(1e7 nodes/s/core, single core):

| B range | predicted class (1 core) |
|---------|--------------------------|
| ≤ 43 | seconds–minutes (all validated ✓) |
| 44–47 | minutes–hours (validated: 9–34 min ✓) |
| 48–53 | hours–days ← **the current frontier: 48, 49 live here** |
| 54–60 | days–weeks |
| 61–64 | weeks–months (and B=64 is the code ceiling: 64-bit digit masks) |
| 65–79 | years–decades |
| ≥ 80 | effectively never (>10^17 nodes) |

Per-base structure fluctuates the boundary by ±2–3 bases in both directions
(b40 is a hours-class spike inside the minutes zone because 39 = 3·13 gives
it almost no usable moduli; b41 is trivial next door). The b-file's authors
stopped at 48 — exactly where the frontier predicts the wall.

## Implications

1. **Hardware is nearly useless; structure is everything.** W grows like
   exp(c·n ln ln n/ln n): multiplying compute by 1000 moves the frontier by
   ~4–6 bases. Moving from 1 core to a 128-core box buys ~2–3 bases. The
   only real lever is per-base luck (the arithmetic of B±1) and better
   *theory* (anything that shrinks the band or its sweep).
2. **The sequence is finitely computable in practice.** Under any realistic
   compute budget, A113028 is computable to B ≈ 60–65 and no further —
   a concrete, principled endpoint. B=80+ requires W > 10^17 refutation
   steps: not a matter of budget.
3. **Specific near-term predictions** (testable): 50–53 are overnight-class
   under v8+parallelism; 54–59 need the incremental-DP band pruning or a
   week of cores; 60 (59, 61 both prime — but 60 = 2²·3·5 rich nilpotent)
   sits right at the practical edge; 61–64 are the last plausibly reachable
   values, likely requiring both engineering levers plus luck.
4. **The right formalization for OEIS**: a(B) exists for all B, but the
   b-file can honestly be extended only to ~60-65 ever; beyond that, entries
   would require a structural breakthrough (a joint-modulus oracle beating
   the NP-hardness intuition), not faster computers.
