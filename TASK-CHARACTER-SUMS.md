# TASK: character-sum measurement program (Fourier health of the counting form)

*(Agent task spec. Extends the two pilot measurements from session
2026-07-24 into the systematic dataset that (a) quantifies the
counting form's error term at real window parameters and (b)
becomes the lead figure of the analytic-outreach package. Both
pilots are exact enumerations; their results, one confirmation and
one partial falsification, are recorded below with the refined
pre-registered predictions this task must test. All claims here
follow campaign discipline: predictions are registered with
pass/fail criteria BEFORE the runs.)*

## 1. Background and definitions

For a layer modulus Q (prime powers of L_eff with orders dividing
e), digit set A, |A| = m, the residue distribution p(t) is the
fraction of class-partitions with Σ B^r s_r ≡ t (mod Q), computed
EXACTLY by the counting class-DP (no sampling). Its Fourier
coefficients μ̂(k) = Σ_t p(t)·e(kt/Q) control the counting form:
count(t) ∝ 1 + Σ_{k≠0} μ̂(k)·e(−kt/Q), so feasibility for all
admissible t follows from Σ_{k≠0}|μ̂(k)| < 1 (over the reduced
character set — see finding P1).

## 2. Pilot findings (exact; reproduce before extending)

**P1 (structure check).** Characters constant on the invariant
coset show |μ̂| = 1.000 exactly (measured at b49 layers 800 and
7353, matching V = 16 and V = 3). These must be EXCLUDED from all
health sums — they are the known invariants, not deviations.

**P2 (b49 window-live layer, Q = 817, e = 3, m = 19, V = 1).**
Max nontrivial |μ̂| = 1.56×10⁻², ~120× the iid scale. The top SIX
characters are exactly one multiplicative orbit under ×B:
{±272} → {±256} → {±289} (272·49 ≡ 256, 256·49 ≡ 289 mod 817).
Mechanism: gcd(Q, B−1) = 1 and Q | B³−1 force 1 + B + B² ≡ 0
(mod Q) — the vanishing-cycle-sum resonance predicted by the
Li–Wan expansion in OPEN-PROBLEM.md's literature section. Falloff
past the orbit is steep (rank 7 at 5.2×10⁻⁴). Total health sum
Σ|μ̂| ≈ 0.08 ≪ 1: the window-live counting form is healthy and
its entire deviation is one identified orbit.

**P3 (blind test, B = 10, Q = 37, e = 3, m = 14 — deep
saturation).** Registered rule "leading orbit = ⟨B⟩-orbit of k
with e·k ≡ ±1 (mod Q)" PARTIALLY FAILED: the top four characters
are orbit members {9, 28, 16, 21} (chance ≈ 3×10⁻⁴ — the orbit
mechanism is confirmed), but ranks 5–6 went to a DIFFERENT orbit
{6, 23, 8} (±), and the predicted members {25, 12} ranked lower.
Note the regime: deeply saturated, all magnitudes within ~1.5× of
each other and below iid scale (consistent with the measured
under-dispersion of the early campaign). The frequency-selection
rule is incomplete; the orbit organization is not.

## 3. Pre-registered predictions for this task (test these)

**R1 (orbit organization, both regimes):** for every layer with
V-part removed, the top-4 nontrivial characters lie in at most two
⟨B⟩-orbits. Fail if any tested layer's top-4 spans ≥ 3 orbits.

**R2 (window dominance):** for layers at window-live parameters
(m within ±2 of the base's m*, layer below its fill depth is NOT
required — use the actual residual m), the leading orbit's |μ̂|
exceeds the first non-orbit character by ≥ 10×. (b49/817 showed
30×.) Fail if any window-live layer shows < 3×.

**R3 (health):** at every window-live layer, the reduced sum
Σ_{k≠0, non-invariant}|μ̂(k)| < 0.5. This is the conjecture's
empirical margin; report the number per layer.

**R4 (selection rule, open):** do NOT assume e·k ≡ ±1. Instead
record, for each layer, the leading orbit's canonical invariant
(e.g. the minimal |e·k mod Q| over the orbit, and the orbit of
k with e·k ≡ ±1 for comparison) and hand the table to Sol/the
outreach recipients — the correct rule is the open question; the
dataset is the deliverable.

## 4. Runs

1. Reproduce P1–P3 from the session scripts (chartier1.py,
   chartop.py, predict.py in scratch; consolidated version below).
2. Window-live layers across bases: for each B in
   {40, 48, 49, 53, 59, 61} and each layer with e ≤ 6 and
   Q < 10⁵ (V-reduced), compute the exact spectrum at the base's
   actual residual m (peeled where applicable — use the real
   residual digit sets from the certificates, not intervals, for
   b40/48/49; intervals acceptable for the prime bases pending
   instrumentation).
3. For each spectrum: top-10 table with orbit identification
   (multiply by B mod Q, group), the R1–R3 verdicts, and the R4
   invariants.
4. Tier 2 at each base: collision-rate MC (≥ 10⁶ pairs) per
   layer, coset-corrected, reported as Σ|μ̂|² vs the
   partition-count prediction with the under-dispersion factor.
5. Guard rails: assert gcd(Q, B) = 1 on every layer (the b39
   lesson, permanently); assert the |μ̂| = 1 characters exactly
   match the predicted invariant subgroup and exclude them by
   construction, not by threshold.

## 5. Deliverables

1. The spectrum tables and R1–R4 verdicts, committed under
   evidence/ with scripts and seeds.
2. One figure: |μ̂| vs frequency for a window-live layer (b49/817
   or a prime-base analog), orbit members highlighted — the lead
   figure for the outreach package.
3. A short RESULTS section for OPEN-PROBLEM.md's analytic
   paragraph: measured Fourier decay, orbit localization, the
   open selection rule, and the health margins — replacing
   "power-saving minor-arc bounds would be needed" with "the
   minor-arc mass is measured, localized on O(1) multiplicative
   orbits fed by vanishing cycle sums, with the selection rule
   open" plus the two-line 1+B+⋯+B^{e−1} ≡ 0 derivation.

## 6. Consolidated script

```python
import math, cmath

def spectrum(B, digits, e, Q, topn=10):
    m = len(digits)
    ks = [sum(1 for i in range(m) if i % e == r) for r in range(e)]
    wts = [pow(B, r, Q) for r in range(e)]
    dp = {tuple(ks): {0: 1}}
    for d in digits:
        nxt = {}
        for caps, cnts in dp.items():
            for r in range(e):
                if caps[r]:
                    nc = list(caps); nc[r] -= 1; nc = tuple(nc)
                    tgt = nxt.setdefault(nc, {})
                    a = d * wts[r] % Q
                    for res, c in cnts.items():
                        kk = (res + a) % Q
                        tgt[kk] = tgt.get(kk, 0) + c
        dp = nxt
    (cnts,) = dp.values()
    tot = sum(cnts.values())
    p = [cnts.get(t, 0) / tot for t in range(Q)]
    mags = sorted(((abs(sum(pp * cmath.exp(2j*math.pi*k*t/Q)
                            for t, pp in enumerate(p) if pp)), k)
                   for k in range(1, Q)), reverse=True)
    return mags[:topn], sum(a for a, _ in mags)

def orbit(k, B, Q):
    o, x = [], k
    while x not in o:
        o.append(x); x = x * B % Q
    return sorted(min(y, Q - y) for y in o)

# example: b49 window-live layer
top, health = spectrum(49, [d for d in range(1, 21) if d != 7], 3, 817)
for a, k in top:
    print(f"|muhat|={a:.3e} k={k} orbit={orbit(k, 49, 817)}")
print("reduced health sum:", f"{health:.3f}")
```

## Status

P1–P2 exact and reproduced twice this session; P3 is a recorded
partial falsification, not an error. R1–R4 registered 2026-07-24
before any §4 run. Python 3, stdlib only.
