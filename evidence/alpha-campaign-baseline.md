# Carry-trie α campaign — baseline (weakest pruning), 2026-07-22

Candidate-21 (known-NO) branch, base 49, C++ port (carrytrie.cpp),
nice -19, serial, this box. Soundness gate passed beforehand
(candidate-20: exactly 1 survivor = known completion, direct-verified).

| split | |X| | |Y| | trie nodes | visits/x | total visits | wall | RSS | survivors |
|-------|-----|-----|-----------|----------|--------------|------|-----|-----------|
| 3+4 | 5814 | 93024 | 1,093,819 | 12,511.6 | 72,742,203 | 12.2s | 135MB | 0 |
| 2+5 | 342 | 1,395,360 | 15,300,916 | 85,391.9 | 29,204,016 | 15.0s | 1.23GB | 0 |

1+6 skipped: projected 16GB trie > 8GB ceiling (80.7 B/node marginal,
~199M projected nodes).

**Fitted α = 0.7092** (visits/x = 3.745·|Y|^0.7092).
b53 projection (|X|≈|Y|≈3e6, Π≈2.7e12): total ≈ 4.4e11 ≈ **6.1× under Π**.
Verdict per §5 pre-registration: in-between (≥30× GO not met at zero
upgrades; not a NO-GO — §4 pruning rungs unimplemented). Baseline
established for per-upgrade attribution.

## Pruning ladder (same protocol, gate re-passed at each rung)

| pruning | α | b53 projection | Π/total |
|---------|---|----------------|---------|
| baseline | 0.7092 | 4.41e11 | 6.1× |
| rung 1 (mask-union) | 0.7065 | 3.61e11 | 7.5× |
| rung 1+2 (+ loose moment budgets) | 0.7065 | 3.61e11 | 7.5× |

Rung 2 as implemented (sound global envelope) fires ~never (<0.01%
visit change): rung 1's exact mask check at depth≥12 already screens
what the loose sum bound would catch. Tightening lever identified:
per-path available-digit pools instead of the global envelope.
Soundness gate re-passed at both levels (1 survivor = known completion).

## Rung 2′ (per-path pools) and FINAL VERDICT

Rung 2′: provably sound, strictly tighter than rung 2, zero added
memory — and inert: visits moved <0.001%, wall WORSENED at 3+4
(9.6s→13.4s, per-visit bit-scan cost with no pruning payoff).
Soundness gate re-passed at both levels. p2′ implemented + gate-tested,
not measured (p1 showed no life, pattern already established).

**FINAL: α = 0.7065. Ladder: baseline 6.1× → rung1 7.5× → rung2/2′
7.5× (saturated). Pre-registered 30× GO bar NOT met. VERDICT: NO-GO
for a prime-base production carry-trie; the mechanism is sound (beats
pair count 30×, engine baseline ~2.8×–7.5×) but its pruning ceiling
is ~7.5× at b53. Moment-based budgets have no purchase over 19 digits
regardless of bound tightness — a genuine, cleanly-attributed negative.
Carry-trie stands as a validated instrument (PRUNE_LEVEL 0–3
reproducible) with a measured constant. Architecture verdict per §5:
peel-plus-asymmetric-meet near-optimal pending shafts 3/4 (algebraic
certificates; non-marginal high-order use).**
