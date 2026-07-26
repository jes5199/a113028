# SECOND-MOMENT-CERTIFICATE — a rigorous one-sided decider for window-band feasibility

**Status:** SPEC — predictions registered, tasks not yet run.
**Date:** 2026-07-22
**Depends on:** BRIDGE-AND-HARDNESS.md (why one-sided is the ceiling), JOINT-COVERAGE.md
(why per-layer ≠ joint), NILPOTENT-PEELING.md (peel first; certificate applies to the
peeled body), the character-sum program (orbit organization, health numbers).

---

## 1. Motivation

Exact band feasibility is NP-complete as L_eff scales (BRIDGE-AND-HARDNESS), so no
polynomial decision procedure exists unless the near-m* promise regime is doing real
work. What *can* exist without contradicting hardness is a polynomial **semi-decision**
procedure: a certificate that a band node's full completion ensemble hits every target
residue, silent when it doesn't. This document upgrades the empirical health number
from the character-sum program into that certificate, states the two theorems that make
it rigorous, isolates the one conjectural lemma, and specs the measurements that decide
whether the lemma is worth proving.

The prize: per DFS node with remaining depth k and node modulus L_node, a passing
certificate ends the feasibility question at that node. "Must actually search" shrinks
to the shell between the equidistribution depth and the leaf horizon P, plus the
resonant layers handled exactly.

## 2. Setup and notation

A **band instance** is (L, D, w) with modulus L, digit set D, |D| = m, and weights
w_i = B^i mod L for the m band positions (the fixed descending prefix's contribution is
absorbed into the target). For t ∈ Z_L:

    N(t) = #{ σ ∈ S_m : Σ_i w_i · d_{σ(i)} ≡ t (mod L) }

Fourier over Z_L, with e_L(x) = exp(2πi x / L):

    F(r) = Σ_{σ ∈ S_m} e_L( r · Σ_i w_i d_{σ(i)} )  =  per(M_r),
    M_r[i,j] = e_L( r · w_i · d_j )

    Ŝ(r) = F(r) / m!          (so Ŝ(0) = 1)
    S₁   = Σ_{r ≠ 0} |Ŝ(r)|
    S₂   = Σ_{r ≠ 0} |Ŝ(r)|²

Inversion:  N(t) = (m!/L) · ( 1 + Σ_{r≠0} e_L(−rt) · Ŝ(r) ).

S₁ is (up to normalization) the **health sum** already measured in the character-sum
program. Ŝ(r) is exact via Ryser in O(2^m · m) per frequency — ~4.4×10⁷ ops at m = 21.

**Orbits.** r ∼ Br partitions Z_L∖{0} into ⟨B⟩-orbits. Replacing r by Br shifts the
weight window w_i ↦ w_{i+1}, i.e. M_{Br} is M_r with one row leaving and one entering;
|Ŝ| is therefore *near*-constant on orbits (this is the structural reason deviation
organizes into orbits, per the b49/817 measurement — it is a heuristic here, not an
assumption of any theorem below). A frequency is **resonant** when its orbit is short:
ord_Q(B) ≤ 3 for the prime power Q it lives on (the 1 + B + B² ≡ 0 layer at b49/817 is
the canonical example).

## 3. Theorem A (exact-spectrum certificate)

**If S₁ < 1 then N(t) > 0 for every t ∈ Z_L.**

Proof: N(t) ≥ (m!/L)(1 − S₁). ∎

Nothing else is needed — no second moment, no independence model. Wherever the full
spectrum is affordable (per prime-power layer: Q values of r, so ~3×10¹¹ ops for
Q₃ = 7353 at m = 21 — hours single-core), Theorem A is a finished theorem the moment
the computation is exact. Caveat per JOINT-COVERAGE: layer-level Theorem A certifies
that *each layer separately* is surjective (some permutation per target per layer); it
does **not** certify joint surjectivity mod L, because the same σ must serve all layers
at once. Joint is Theorem B's job.

## 4. Theorem B (hybrid certificate for the joint modulus)

Let O ⊂ Z_L∖{0} be any set of frequencies (in practice: the resonant orbits, computed
exactly at representatives) and write tail(t) = Σ_{r ∉ O∪{0}} e_L(−rt) Ŝ(r). If t is
unreachable then 1 ≤ S₁(O) + |tail(t)|, so |tail(t)| ≥ 1 − S₁(O). By Parseval,
Σ_t |tail(t)|² = L · S₂(rest). Hence

    #unreachable targets  ≤  L · S₂(rest) / (1 − S₁(O))².

If the right side is < 1, the ensemble is jointly surjective — using only exact values
on O plus an upper **bound** on S₂ off O. Everything is rigorous except producing that
bound, which is:

## 5. Lemma C (conjectural — the actual work)

Independent model per position: ĝ_i(r) = (1/m) Σ_{d∈D} e_L(r w_i d), product bound
P(r) = Π_i |ĝ_i(r)|. Define the **without-replacement correction**

    ρ(r) = |Ŝ(r)| / P(r)     (clamped where P(r) < ε).

**Conjecture (Lemma C).** For non-resonant r, ρ(r) ≤ K with K bounded (or at worst
sub-exponential in m), uniformly over in-window band instances.

Why plausible: (i) for r ≢ 0 mod Q with e = ord_Q(B) large, the orbit {r B^i}
equidistributes and P(r) decays exponentially in the number of spread positions;
(ii) converting the independent product into a bijection bound is exactly the
switching / negative-correlation machinery of Eberhard–Manners–Mrazović and
Müyesser–Pokrovskiy for complete-mapping problems — built for this, but a transfer to
weighted position sums is real work. Why the resonance exemption is principled: at
e ≤ 3 the orbit cycles among ≤ 3 values, P(r) has no decay, and these are precisely
the frequencies observed carrying ~120× iid deviation — the lemma's exception set *is*
the dominant orbit. Lemma C + Theorem B = the polynomial semi-decider.

**Relation to hardness.** No tension with BRIDGE-AND-HARDNESS: a node failing the
certificate is not refuted. Infeasibility remains expensive, as it must.

## 6. Registered predictions

- **P1.** Every in-window b49 layer has S₁^(Q) < 1, including the resonant 817 layer
  (measured health ≈ 0.08 predicts a comfortable pass).
- **P2.** The b49 Q₃ = 7353 layer has S₁^(Q) < 1 — i.e. its full completion ensemble
  is surjective, and the observed never-filling is a **visited-set artifact** of the
  DFS order, not an ensemble deficiency. (This is the sharpest falsifiable claim in
  the program: SATURATION-PROFILES already argues the phenomenon is
  reachable-set-shaped; P2 says the spectrum will confirm it.)
- **P3.** On non-resonant frequencies, ρ(r) is bounded: median ≤ 3, no upward drift
  in m across bases.
- **P4.** On resonant frequencies (ord_Q(B) ≤ 3), ρ(r) ≫ 1, quantitatively matching
  the known deviation orbit.

Falsification of P3 means the deviation is not purely spectral/positional and Lemma C
is false as stated — record and stop. Falsification of P2 means in-band sub-saturation
is an ensemble phenomenon after all, and SATURATION-PROFILES needs amending in the
opposite direction.

## 7. Tasks

### T1 — Exact per-layer spectra, b49 window (Theorem A pass)

For each in-window prime-power layer Q of the peeled b49 body: compute Ŝ(r) exactly
(Ryser) for all r mod Q; report S₁^(Q), S₂^(Q), orbit-resolved mass, and the fraction
of S₂ carried by the top orbit. C implementation; complex accumulation in double is
fine (m! ≤ 21! headroom vs double mantissa: verify with one long-double spot check).

*Budget:* worst layer Q₃ = 7353 × 4.4×10⁷ ≈ 3×10¹¹ flops — hours, single core.
*GO:* every layer S₁ < 0.5 and top-orbit S₂ share ≥ 90%.
*Conditional:* any layer with 0.5 ≤ S₁ < 1 still certifies but flags Theorem B margins.
*NO-GO:* any layer with S₁ ≥ 1 — Theorem A silent there; record which and stop T3.

### T2 — Correction-factor measurement (Lemma C, empirical)

Bases 20–30 (m* = 10–13) plus b38/b39 (m* = 16): stratified sample of ≥ 200
frequencies per base — strata by ord_Q(B) bucket (≤3, 4–10, >10) and by layer.
Compute exact |Ŝ(r)| (Ryser; ≤ 10⁵ ops per frequency at m = 13) and P(r); emit
(base, Q, r, ord, |Ŝ|, P, ρ). One small layer gets a *full* spectrum as a Parseval
sanity check. Regress log ρ on m over non-resonant strata.

*GO:* non-resonant median ρ ≤ 3, max ≤ 20, slope of log ρ vs m ≤ 0.1.
*NO-GO:* slope > 0.3 or a heavy tail (>5% of non-resonant ρ over 20). NO-GO kills the
EMM transfer as specced; the fallback is Theorem B with exact computation on a larger
O, which is weaker but still poly per node.

### T3 — Theorem B closure at b49 (contingent on T1 GO)

Take O = all orbits with ord ≤ 3 plus any layer flagged in T1; exact Ŝ on O; bound
S₂(rest) from T1's exact layer values (cross-layer frequencies bounded by the product
of layer factors — document the inequality used); evaluate the Theorem B count.
*GO:* bound < 1 → joint surjectivity of the b49 window ensemble is a theorem modulo
the documented inequality. Report margins either way.

## 8. Deliverables

- `scripts/spectrum_exact.c` (Ryser per frequency, layer sweep, orbit bookkeeping)
- `evidence/second-moment/` — per-task CSVs + a SUMMARY.md with the GO/NO-GO verdicts
  against §6–§7 verbatim
- On T1 GO + P2 resolution: one-paragraph amendments to JOINT-COVERAGE.md and
  SATURATION-PROFILES.md
- On T2 GO: a THEOREM-target statement ("in-window layer, health < ½, positive layer
  counts ⇒ surjective") queued for the outreach package, with the EMM/MP transfer
  flagged as the open proof obligation

## 9. Out of scope

Infeasibility certification (impossible cheaply, per hardness); visited-set/DFS-order
instrumentation (separate program — this doc only decides whether the *ensemble* is
the problem); any wall-clock claims (the carry-trie NO-GO stands; this program is
mathematical, not a solver speedup candidate).
