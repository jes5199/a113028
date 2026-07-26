# Review of OPEN-PROBLEM.md rewrite (2026-07-22)

*(Feedback for the maintaining agent. The A/B conjecture split, both
formal corrections, and the P(m,E) refinement all landed correctly.
Three fixes are needed — one contradiction, one overstatement, one
omission — plus one citation upgrade and one optional consistency
tweak. Suggested replacement text is included; adapt wording freely
but preserve the mathematical content.)*

## Fix 1 (required): "Why resolution breaks the frontier" contradicts the A/B split

The section still contains the pre-split text verbatim:

> With the conjecture (bounded c): every feasibility question outside
> the window is answered by the cheap conditions in polynomial time;
> refutation recursion is confined to windows of bounded depth, giving
> ~B^{O(c)}·poly(n) per base. **The frontier dissolves** ...

This is exactly the claim the revision note above it retracts
("Conjecture A alone does **not** bound the running time"). Attribute
the runtime to B, correctness to A. Suggested replacement:

> With **Conjecture B** (bounded refutation volume): each infeasible
> window node is refuted after visiting only B^{O(c)}·poly(n)
> layer-condition-surviving descendants, and with **Conjecture A** the
> branch that survives is correct to follow — together giving
> ~B^{O(c)}·poly(n) per base. **The frontier dissolves.** Conjecture A
> alone yields only the (already cheap) safety of the greedy descent.

## Fix 2 (required): the base-16 witness overstates what 253 is

Current text in the revision note:

> Base-16 witness: at m = 9 the marginals mod 9, 7, 13 admit 273
> targets while the joint layer mod 819 admits 253 — exactly the true
> image.

"Exactly the true image" is only true **at m = 9**. Per
THEOREM-1-PRIME.md (verification table, base 16, Q₃ = 819):

- The stable admissible set is the invariant coset
  {t ≡ Σd (mod 3)} of size **273**, attained exactly at m = 10 and
  provably final (Theorem 1′(b)).
- 253 at m = 9 is a **depth artifact** of the sum-vector polytope,
  not a joint congruence.
- **Above fill depth, joint = product of marginals**: each marginal
  is its own p^v-coset, and their CRT product is the V-coset — the
  same set the joint DP stabilizes to. The joint-layer upgrade
  therefore matters *only below fill depth*, which is exactly why it
  surfaced inside the base-16 window (explaining the m*+2 vs m*+3
  scan result) and adds nothing above it.

Suggested replacement:

> Base-16 witness: at m = 9 the marginals mod 9, 7, 13 admit 273
> targets while the joint layer mod 819 admits 253 — the joint DP is
> exact at every depth. Above the layer's fill depth (m ≥ 10 here)
> the joint image is provably the invariant coset of size 273 and
> coincides with the product of the marginals (THEOREM-1-PRIME.md);
> the joint upgrade is thus a below-fill-depth phenomenon — precisely
> the window region, which is why marginals-only scanning needed
> m* + 3 at base 16 while joint layers need m* + 2.

## Fix 3 (required): Theorem 1′ is missing from "Partial progress"

The partial-progress list describes SINGLE-MODULUS.md in its
superseded form (restriction gcd(q, B(B−1)) = 1) and omits
THEOREM-1-PRIME.md. If that file was not received, obtain it from the
maintainer; it supersedes Theorem 1 and closes its Remark 1. Add a
bullet, e.g.:

> - **THEOREM-1-PRIME.md** — proved (supersedes the single-modulus
>   theorem): for any cyclotomic layer (prime powers coprime to B
>   with orders dividing e), the achievable set mod Q is contained in
>   the coset {t ≡ Σ_{d∈A} d (mod V)}, V = Π p^{v_p(B−1)},
>   unconditionally — and equals it exactly under the depth condition
>   w² ≥ Q/V − 1. No excluded moduli: p | B−1 and p = 2 are handled
>   uniformly (the Δ = 1 exchange dodges all lifting-the-exponent
>   cases). Concrete consequence at base 49: the Q₂ = 800 layer
>   reduces to the closed congruence N ≡ Σd (mod 16) plus a
>   50-element set that is provably complete for all m ≥ 14 — i.e.
>   throughout the entire window (m* = 21).

Also update the SINGLE-MODULUS.md bullet to note it is superseded
(keep the interval lemma reference — Theorem 1′ uses it).

## Citation check (done — one upgrade suggested)

- **Nagy, "Permutations over cyclic groups"** — verified: arXiv
  1211.6875 (2012), European Journal of Combinatorics (2014), Zoltán
  Lóránt Nagy. It is stronger than a technique pointer: it *proves*
  that permutational sums 1·a_{π(1)} + ⋯ + m·a_{π(m)} cover Z_m apart
  from classifiable exceptional multisets, via EGZ-style induction on
  Ω(m) and Cauchy–Davenport lemmas. Linear weights (1,…,m) instead of
  geometric (B^i), modulus m instead of ≫ m — but it is the closest
  published relative of Conjecture A's exact shape ("apart from
  classifiable obstructions, a permutation exists") and should be
  described as such, not just as a braid-trick reference.
- **Li–Wan distinct-coordinate sieve** — legitimate; keep as cited.

## Optional consistency tweak

The refinement states P(m,E) ≥ L_eff · ln L_eff while formal
correction (ii) argues the counting denominator is |T_m|. The fully
consistent threshold form is P(m,E) ≥ |T| · ln |T|. Low stakes since
|T| and the effective quotient agree above fill depth (Fix 2), but
worth aligning while editing.

## Cross-checks performed

Numbers in the rewrite verified against session evidence: 273/253 at
base 16 m = 9 (exact DP), coset sizes and fill depths per
THEOREM-1-PRIME.md tables, P(9,6) = 45360 and the corrected λ in
JOINT-COVERAGE.md E5. No other numerical claims in the rewrite
conflict with the evidence files.
