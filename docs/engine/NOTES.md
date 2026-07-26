# A113028 solver — theory & optimization notes

## Problem
a(B) = largest number whose base-B representation uses **distinct nonzero digits**
(a subset S ⊆ {1..B-1}) and is divisible by lcm(S). Two engines per surviving
subset: S = multiple-scan with digit-prefix skip; D = MSB-first prefix DFS with
7-digit tail brute force.

## Why base 40 explodes (the cliff)

Validated timings: b37 = 27s, b38 = 116s, b39 = 69s, b40 > 1h (first surviving
k=34 subset alone escalated past a 17e9-node budget).

The engines' pruning power is governed by the **multiplicative order of B modulo
the prime powers of lcm(S)**:

- order 1 (q | B−1): free — digit-sum filter kills at subset level.
- order 2 (q | B²−1): cheap — `e2_check` DP kills at subset level and per DFS prefix.
- order ≥ 3: **no filter at all today**. The only recourse is exhaustive engine
  work, and for a *dead* subset that means proving emptiness by near-exhaustion.

So the difficulty of base B is driven by the factorizations of B−1 and B+1:

- B=37: 36 = 4·9 → rich order-1/2 structure → fast.
- B=40: 39 = 3·13, 41 prime, B²+1 = 1601 prime, B²+B+1 = 3·547 (547 > 39) →
  almost **no** order-≤2 structure. For the hard k=34 subsets, ne2 = 0: the DFS
  has *zero* prefix pruning and the scan has to walk a huge multiple-space.
  Dead subsets are then catastrophically expensive to refute.

But b40 does have *higher-order* structure the solver ignores:
- ord(40 mod 9) = 3 (40 ≡ 4, 4³ ≡ 1 mod 9) — an order-3 subpower of 27.
- ord(40 mod 7) = 6.
So an order-e feasibility filter would restore subset-level kills exactly where
the current code goes blind.

## Optimization roadmap (priority order)

1. **Order-e subset infeasibility DP** (e = 3..6, small q). Positions 0..k−1
   fall into e weight classes by i mod e with fixed counts c_j; weights
   w_j = B^j mod q. Subset is alive only if the digit multiset can be
   partitioned into groups of sizes c_j with Σ w_j·(group sum) ≡ 0 (mod q).
   DP over digits, state = (counts used per class 0..e−2, running weighted sum
   mod q). e=3, q=27, k=35 → ~5e3 states: trivial. e=6, q=7 → ~1e5 states:
   still fine at subset level. Generalizes q1 (e=1) and q2 (e=2) subpower logic:
   for each prime power, find the largest subpower with ord dividing e.
2. **Order-3 prefix prune in DFS** (generalize `e2_check` to e=3 for small q):
   restores per-node pruning where ne2 = 0.
3. **Nilpotent-suffix enumeration**: moduli q | B^t with t ≥ 2 (e.g. 25 for
   B=40, t=2) constrain only the last t digits, but tail_rec discovers this
   only at m==0. Enumerating admissible t-suffixes first cuts tail work by ~q.
4. **Resumable DFS**: budget-doubling currently restarts the DFS from scratch
   each round (×4 caps) — wasted re-descent, worth ~1.3–2×.
5. **Parallelism** across subsets (pthreads) — deferred; box is 4-core and
   shared, load already ~3.8.

## Model correction (b42 outcome)

Predicted b42 as worst-in-range (41, 43 both prime) — WRONG: completed in 116s
(2.29M subsets checked, filters killed nearly all instantly). The smoothness
model was too crude. Refined model: what matters is the **size of the usable
order-1/2 moduli**, where usable means the prime power can divide lcm(S), i.e.
p ≤ B−1. B−1 prime is therefore the BEST case, not the worst: digit B−1 brings
a giant order-1 modulus (digit-sum ≡ 0 mod B−1, a 1-in-(B−1) filter). The bad
case is B−1 with *mid-size* factors and B+1's factors useless (> B−1):
- B=40: 39 = 3·13 → only small order-1 moduli; 41, 1601, 547 all > 39 →
  blind. Uniquely bad in range.
- B=42: 41 prime = B−1 → giant digit-sum filter → fast. Same for 44 (43),
  48 (47).
- Remaining risk in range: B=46 (45 = 9·5 small moduli, 47 > 45 useless).

## Strategic fork (post-regression insight, 2026-07-21 evening)

Real baseline data shows `scanned=1` on every slow base (34–39): the order-1/2
filters already isolate a single surviving subset, and ~all wall-clock is
engine work *inside* it (b38: one k=35 subset, 116s, engine S wins after cap
escalations and yields the answer). Two worlds for b40's escalating k=34 subset:

- **Survivor is dead** → order-e subset filter (v3) kills it pre-engine and the
  base collapses to seconds. The 5-min b40 probe with v3 answers this.
  **PROBE RESULT: no kill.** b40's k=34 survivor passes order-e (mod 9 e=3,
  mod 7 e=6) — with 34 digits and tiny moduli the partition DP is almost always
  feasible, so the filter cannot refute large survivors. Order-e stays merged
  (free insurance, will matter for smaller-k survivors at other bases), but
  b40 = b49 = engine-bound. Engine speed is the only live path for both.
- **Survivor is alive** (like b38's) → subset filters can't help; need faster
  engines:
  - **Engine S incremental residues**: today each scan iteration recomputes the
    full number's residue = O(k) u128 divisions. Maintain per-position prefix
    residues and recompute only below the highest changed digit (skips mostly
    touch low digits) — plausible 5–30× constant-factor win on exactly the
    114s-per-base grind. Alternative/complementary: CRT residue vector (u64
    per prime power) instead of u128 % LCM.
  - **Engine D order-3 prefix prune** (q=9 for B=40, state ~13·13·9) at
    shallow depths, plus **nilpotent m==t feasibility** (at m==t, the last t
    digits alone determine n mod q for q | B^t; e.g. q=25, t=2 for B=40:
    prune d1 unless some avail d0 has 40·d1+d0 ≡ 0 mod 25). Together these
    could flip D ahead of S on weak-modulus bases.

## Ops log
- Source reconstructed from a 3-part Telegram paste; compiled clean first try;
  bases 2–39 all match the OEIS b-file exactly → reconstruction is faithful.
- Tail strategy: per-base detached runs, `timeout 3600` each (bases 40–48),
  sequential (shared 4-core box), results appended to run_output.txt.

## Peeling-era notes (2026-07-22)

- SATURATION-PROFILES verified: b40/b48 coprime bodies are DP-inert (all
  DP-able layers fill far below the band) — peeling is the ENTIRE available
  win at those bases; no cheap-condition upgrade can matter there. b49's
  layers fill at m≈12–14 on intervals; any in-band sub-saturation is a
  reachable-gap-set property, so the b49 window profile MUST be computed on
  actual visited sets, never the interval convention.
- v12 validation ladder: after regression, peeled-b40 runs as the
  known-answer wall-clock check (v9 baseline 6856s) before v12 touches b48.
