# TASK: join-key false-positive measurement (prefix-MITM go/no-go)

*(Agent task spec. Decides whether prefix meet-in-the-middle is
viable, using existing leaf-level data — no new theory required.
Background: after peeling, refutation cost = enumeration of the
combinatorial prefix (levels above the leaf horizon) × ~3
arithmetic candidates per leaf. Prefix-MITM would halve the
enumerated depth and join the halves on an additive key, for a
near-square-root of the prefix cost — the only avenue that scales
at prime bases where peeling is inert. Its sole open question is
empirical: how much of the leaf predicate's rejection power is
captured by additive statistics? This task measures that as a
false-positive curve. Everything here is measurement + arithmetic;
the one theoretical fact needed (moments are additive across
halves and must match exactly at a true solution) is stated and
proved inline. Session 2026-07-24.)*

## 1. Definitions

At a leaf: S = the set of digits consumed by the enumerated
prefix; r = the prefix residue; the leaf predicate F(S, r) asks
whether some multiple of L_c in the leaf window has base-B digit
multiset EXACTLY equal to the complement A∖S in the last P
positions (this is what residue-leaf decoding checks; ~B^P/L_c ≈ 3
candidates per leaf at peeled b49).

**Key fact (exact).** If a candidate multiple accepts, its digit
multiset equals A∖S, hence EVERY statistic of its digits equals
the same statistic of A∖S — in particular the integer moments
M_k = Σ d^k and any count statistic. And every such statistic of
A∖S is additive across prefix halves: stat(A∖S) = stat(A) −
stat(S_top) − stat(S_bottom). So (moments of consumed digits,
residue) is a sound MITM join key: a true solution can never be
rejected by it. The open question is only its POWER.

**FP(k) — the quantity to measure.** For a candidate multiple c at
a leaf (S, r) with required complement R = A∖S: define
sep(c, S) = the smallest k such that M_k(digits of c) ≠ M_k(R),
with sep = ∞ if all tested statistics match but the multisets
differ (and sep = ACCEPT if they are equal). Then

    FP(k) = fraction of (leaf, candidate) pairs with sep > k

is exactly the fraction of arithmetic candidates that an
order-≤k moment key fails to reject. Small FP(k) for small k ⟹
the MITM join sheds almost all spurious pairs ⟹ prefix-MITM
approaches its square-root ideal.

**Known redundancy (account for it, don't be surprised by it):**
M₁ of the candidate is automatically ≡ M₁(R) mod the (B−1)-part
of L_c (casting out nines is inside the residue); the integer
value of M₁ still carries information beyond it. Report FP both
with and without M₁ to show the marginal value of each statistic.

## 2. Procedure

1. **Data.** Instrument the nilpeel verifiers (b49 first — logs
   and binaries exist) to emit, per (leaf, candidate): S-mask, r,
   the candidate's digit multiset, and accept/reject. Sample if
   volume is an issue: ≥ 10⁶ (leaf, candidate) pairs, stratified
   across the prefix (record the top-level branch id so FP can be
   checked for uniformity across the tree — clustered FP is a
   finding, cf. the corridor episode).
2. **Statistics ladder.** k = 1, 2, 3 integer moments; then count
   statistics: #digits in each of 4 quartile bands; then class
   counts mod 2 and 3 of digit values. Compute sep for each pair
   under the ladder order; report FP after each rung.
3. **Soundness check (mandatory).** Run the same pipeline on the
   known-YES branch (b49 candidate-20, last digit 7): the
   accepting candidate must have sep = ACCEPT, never a rejection.
   Any violation is a bug in the pipeline, full stop.
4. **Collision-sum estimate (the actual speedup number).** From
   the same logs, build the half-prefix signature histograms at
   the split level h = ⌈(m − P)/2⌉ and compute
   Σ_key n_top(key)·n_bot(key) — the surviving-pair count the
   join would actually process. Report it against the ideal
   (m−P)!-vs-two-halves arithmetic and against the engine's
   current prefix count.
5. **Guard rails.** assert gcd(L_c, B) = 1 everywhere; assert the
   leaf-window candidate count matches B^P/L_c ± 1; cross-check
   total leaves against the closed forms (e.g. 19!/12! at b49).

## 3. Decision thresholds (computed in advance, per campaign discipline)

Let Π = prefix enumeration count (engine baseline), and let the
measured surviving-pair count be Σ. Prefix-MITM wins iff
2·√Π·(1 + overhead) + Σ < Π, i.e. roughly Σ ≪ Π and FP small
enough that Σ ≈ (collisions from key entropy) rather than ≈ Π.

- **GO** if FP(3 moments + quartiles) < 10⁻² AND the measured
  collision sum projects ≥ 10× total-cost reduction at b49
  scale. (b49's own prize is modest — ~10³ ceiling on 7 levels —
  the GO is about validating the mechanism for prime bases.)
- **NO-GO** if FP plateaus above ~10⁻¹ through the full ladder:
  conclude decomposability hinges on multiset structure additive
  statistics don't see, record it as the natural-optimality
  finding for the current architecture, and promote shafts 3/4.
- In between: report the FP curve and stop for human/Sol review —
  the shape of the curve (which rung buys what) determines
  whether a smarter key is worth designing.

## 4. The prize, sized (companion computation)

For the prime bases the frontier runs through, with T = 0 (no
peeling) and P = ⌈log_B L_eff⌉:

| B | m* | P | prefix levels m*−P | prefix count Π ≈ m*!/P! | √-ideal |
|---|---|---|---|---|---|
| 53 | 22 | 12 | 10 | ~2.7×10¹² | ~10⁶·polylog |
| 59 | 23 | 13 | 10 | ~4.3×10¹² | ~10⁶·polylog |
| 61 | 25 | 13 | 12 | ~2.9×10¹⁴ | ~10⁷·polylog |

(Agent: recompute these exactly from L_eff — the table uses the
ln L_eff values of IRREDUCIBILITY-LAW.md and is ±1 in P; commit
the exact versions.) A working join key takes week-scale prime
refutations to hours. This is the number the GO decision buys.

## 5. Deliverables

1. FP(k) curve with per-rung marginals, stratified by branch;
   soundness check result; collision-sum estimate and projected
   b49 and b53 costs.
2. One-paragraph verdict against §3 thresholds.
3. Raw logs committed under evidence/ with the sampling seed.

## Notes

- The Irreducibility Law does not constrain this avenue: the key
  carries the full residue mod L_c (complete-information
  additive), and the moment statistics are properties of the
  digit multiset, not bounded-order congruence marginals.
- If FP clusters by branch or by leaf depth rather than being
  uniform, report the clustering before averaging — after this
  campaign, a structured failure is a finding and a uniform
  failure is a verdict, and the difference matters.
