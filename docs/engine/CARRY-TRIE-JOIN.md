# The carry-trie join: first measurement and the scaling experiment

*(For the agent and Sol. Follows Sol's sharpened MITM target: an
additive signature φ plus a sound acceptance superset Γ such that
pairs with φ(x)+φ(y) ∈ Γ are reported output-sensitively. This
note records: the lift/borrow construction that makes the trie
walk well-defined; a working Python prototype run on the real b49
candidate-21 branch; the first bucket-visit measurement — the
mechanism passes, already beating the engine's baseline node count
at b49's own unfavorable split with only the weakest pruning; and
the spec for the one experiment that now carries the program: the
scaling exponent α of per-x cost in |Y|. Session 2026-07-24.)*

## 1. Setup and the two constructions that make it work

Notation (b49 candidate-21, suffix digit 7): 19 remaining digits
A₁₉ = {1..20}∖{7} in positions 1..19; leaf block = positions 1..12
(P = 12); prefix levels 13..19 split as y-block (13..16, NY = 4)
and x-block (17..19, NX = 3). Condition:
Σ d_i B^i ≡ T_mid (mod L_c), L_c = L/7.

**Lift resolution.** The leaf value is B·w with w ∈ [0, B^P)
carrying the 12 leaf digits. The congruence gives
w ≡ c_x − u_y (mod L_c) with c_x = B⁻¹(T_mid − r_x),
u_y = B⁻¹·r_y — but a residue does not determine base-B digits;
the lift does. Resolve by iterating W = c_x + j·L_c over the
~⌈B^P/L_c⌉ + 1 lifts and computing w = W − u_y as an INTEGER.

**Borrow-deterministic walk.** Store all y in a base-B trie keyed
by the LSD-first digits of u_y (depth 14 covers L_c plus lift
headroom). Per (x, j), walk the trie computing the digits of
W − u_y least-significant-first: the borrow is determined by the
path, so each trie node yields exactly one candidate leaf digit.
Prune when that digit is 0, exceeds the digit range, is outside
A₁₉∖S_x, or repeats among w's digits so far; beyond depth P the
remaining digits of W − u_y must be zero. At terminals, check mask
disjointness and that w's digit set equals the exact leftover.
Soundness is by construction: no valid completion is ever pruned.

## 2. First measurement (real branch, known NO)

Prototype `carrytrie2.py` (repo scratch; reference implementation
of §1), 250-of-5,814 x-sample, all four lifts:

| quantity | value |
|---|---|
| nodes visited per x (all lifts) | ~15,600 |
| extrapolated total bucket visits | ~9.1×10⁷ |
| unrestricted pairs (x·y·lifts) | 2.7×10⁹ |
| engine baseline leaves (19!/12!) | 2.54×10⁸ |
| survivors | 0 (known-NO branch ✓) |

**Reading:** the implicit-Γ join is output-sensitive in practice —
30× under the pair count, and ~2.8× under the engine's baseline
node count *at b49's own unfavorable 3+4 split*, with only
digit-membership + distinctness pruning: no moment budgets, no
subtree aggregates. Caveats: Python node counts, not wall time;
sample extrapolation; the branch target T_mid was reconstructed
independently and could differ in detail from the verifier's —
the join-cost measurement is insensitive to the exact target, so
the mechanism conclusion stands either way, but the agent should
recompute T_mid from nilpeel_b49's own constants before any
production use.

## 3. The experiment that now carries the program: the exponent α

Per-x cost was ~15.6k at |Y| = 93,024. Define α by
per-x visits ∝ |Y|^α. Everything about the prime-base prize
reduces to α:

- Total join cost ≈ |X|·|Y|^α·polylog. At b53
  (|X| ≈ |Y| ≈ 3×10⁶, Π ≈ 2.7×10¹²): α ≈ 1 gives ~10¹³ (no win);
  α ≈ 0.7 gives ~10¹⁰–10¹¹ (10–100×); α near 0 (polylog) gives
  ~10⁷–10⁸ — the square-root-class advance.

**Spec:** measure the 2+5 split at b49 itself: |Y| = ¹⁹P₅ =
1,395,360 (a 15× step in |Y| with ground truth intact), |X| =
¹⁹P₂ = 342. One data point pins α between 93k and 1.4M; a third
point (3+4 vs 2+5 vs synthetic 1+6, |Y| = ¹⁹P₆ ≈ 19M if memory
allows) confirms linearity of the fit. C++ scale (trie ≈ 20M+
nodes); port §1's construction — the two subtleties (lift
iteration, LSD borrow) are the only nonobvious parts and are
solved above. Report per-x visits at each |Y|, the fitted α, and
projected b53/b59/b61 totals against their Π.

## 4. Pruning upgrades (expected to push α down; add in this order)

1. **Subtree mask-union bitsets:** at each trie node store the OR
   and AND of descendant y-masks; prune when every descendant
   intersects x, or when the union cannot supply the required
   leftover complement.
2. **Moment budgets:** per node store min/max of p₁(S_y) (then p₂)
   over the subtree; prune when the running digit-sum of w plus
   the subtree's p₁ range cannot meet p₁(A₁₉) − p₁(S_x). Sol's FP
   table (p₁,p₂ pass rate ≈ 2×10⁻⁶ on 5M pairs) says these are
   ferociously selective; the open question is only how early in
   the walk they bite.
3. Measure α after each upgrade separately — attribution matters
   for deciding what to implement at prime-base scale.

## 5. Decision framing

- GO for a prime-base production implementation if fitted α with
  upgrades projects ≥ 30× under Π at b53.
- If α stays near 1 despite upgrades: record it as evidence the
  carry structure's pruning saturates, fall back to the TASK-
  MITM-FP thresholds (which the FP half has already passed —
  the failure would be purely in the join), and treat the
  peel-plus-asymmetric-meet architecture as near-optimal pending
  shafts 3/4.
- Guard rails as always: assert gcd(L_c, B) = 1; assert lift
  count = ⌈B^P/L_c⌉ ± 1; validate against a known-YES branch
  (candidate-20, suffix 7) where exactly the known completion
  must survive — that run doubles as the soundness proof of the
  implementation.

## 6. Reference prototype

`carrytrie2.py` in the session scratch is the reference. Core walk
reproduced here for the port:

```python
# per (x, j):  W = c_x + j*L_c ; walk trie of u_y digits LSD-first
stack = [(trie_root, 0, 0, 0)]          # node, depth, borrow, used-mask
while stack:
    node, dep, borrow, used = stack.pop()
    visit_count += 1
    if dep == DEPTH:
        if borrow == 0:
            for ymask in node.terminals:
                if not (ymask & xmask) and used == (allowed & ~ymask):
                    report_survivor()
        continue
    for dig, child in node.children:
        wd = Wdigits[dep] - dig - borrow
        nb = 0
        if wd < 0: wd += B; nb = 1
        if dep < P:
            if wd == 0 or wd not in allowed_minus_used: continue
            stack.append((child, dep+1, nb, used | bit(wd)))
        elif wd == 0:
            stack.append((child, dep+1, nb, used))
```

## Status

Construction: exact/sound as argued in §1. Measurement: sampled
prototype, caveats as stated in §2. α and everything downstream:
open, specced in §3–4. Session date 2026-07-24.
