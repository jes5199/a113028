# Bases 65–89: reachability, classification, and first-pass results

**Date:** 2026-07-25 · **Status: IN PROGRESS — facts only, no conclusion.**
A second round is live at corrected widths and one release-layer probe is
running. Nothing here is a verdict on the range.

## The four outcome classes

Keeping these separate is not pedantry — collapsing them manufactures false
statements about A113028 rather than about our engine.

| class | meaning |
|---|---|
| **REFUTED** | a real mathematical verdict: no completion under *that prefix* inside *that width* |
| **INCONCLUSIVE (resource)** | timeout or resource decline — a statement about our budget, never about the maths |
| **COULD-NOT-RUN @ W** | the engine refused to attempt the base *at that width*; configuration inadequate, not a negative |
| **UNREACHABLE** | no *supported* width can attempt the base at all — an engine limitation |

## Reachability is derivable, and it bites

`certset` requires `NX + NY = W − T − Pc ≥ 2`, i.e. **`T + Pc ≤ W − 2`**.
`CERTSET_W` is validated only on **[20, 24]**, so

> **`T + Pc ≤ 22` is a hard engine limit.**

`T` and `Pc` come from the digit set with no search (`deriveConstantsGen`), so
**which bases can be attempted, and at what width, is knowable before spending
anything.**

- **b82** (`T=6, Pc=17, T+Pc=23`) — **UNREACHABLE**. Confirmed by run: at
  W=24 it still returned `DECLINED: NX+NY<2`.
- **b86** (`T=6, Pc=18, T+Pc=24`) — **UNREACHABLE** by the same arithmetic.
- The other **23 of 25 are reachable**, at the minimum widths tabulated below.

⚠️ **A reader must not infer "no answers in 65–89" from these results.** Two
bases were never attemptable, and four more (b74, b78, b84, b87) require
`W ≥ 23` and so were *not tested* by the W=22 sweep.

## Results so far — with the honest denominator

The W=22 sweep could only address bases whose minimum width is ≤ 22: **19 of
the 25**. Against that denominator:

> **17 REFUTED · 2 INCONCLUSIVE (resource) — out of 19 attemptable at W=22.**

Quoting "17 of 25" would understate coverage *and* overstate the negatives at
the same time. **Zero completions found so far.**

Wall-clocks at fixed W=22 span **0.192 s (b81) to 422.2 s (b65) — a 117×
spread**, which is itself a finding: see the cost ledger below.

## The 25-base classification

`r0 = 1` means the naive descending prefix is feasible, so a hit there would
be *immediately maximal* — it says nothing about whether a hit exists
(footguns §15).

| base | \|D\| | forced drops | L_nil | T | Pc | T+Pc | min W | r0 | W=22 outcome |
|-----:|------:|---|------:|--:|---:|-----:|------:|:--:|---|
| 65 | 59 | 13,26,30,39,52 | 25 | 2 | 14 | 16 | 18 | 1 | REFUTED 422.2s |
| 66 | 59 | 11,22,30,33,44,55 | 1728 | 6 | 13 | 19 | 21 | 1 | INCONCLUSIVE (timeout 1800s) |
| 67 | 65 | 33 | 1 | 1 | 15 | 16 | 18 | 1 | REFUTED 155.9s |
| 68 | 63 | 17,32,34,51 | 64 | 3 | 15 | 18 | 20 | 1 | REFUTED 168.5s |
| 69 | 65 | 23,33,46 | 27 | 3 | 15 | 18 | 20 | 1 | REFUTED 348.2s |
| 70 | 59 | 7,14,21,28,30,35,42,49,56,63 | 1600 | 6 | 14 | 20 | 22 | 1 | INCONCLUSIVE (timeout 1800s) |
| 71 | 69 | 35 | 1 | 1 | 16 | 17 | 19 | 1 | REFUTED 45.2s |
| 72 | 63 | 9,18,27,32,36,45,54,63 | 192 | 2 | 15 | 17 | 19 | 1 | REFUTED 3.3s |
| 73 | 71 | 36 | 1 | 1 | 17 | 18 | 20 | 1 | REFUTED 28.2s |
| 74 | 71 | 36,37 | 64 | 6 | 16 | 22 | 24 | 1 | COULD-NOT-RUN @W22 (needs 24) |
| 75 | 71 | 25,36,50 | 135 | 3 | 16 | 19 | 21 | 1 | REFUTED 6.1s |
| 76 | 71 | 19,36,38,57 | 64 | 3 | 16 | 19 | 21 | 1 | REFUTED 26.7s |
| 77 | 69 | 11,22,33,35,44,55,66 | 49 | 2 | 16 | 18 | 20 | 1 | REFUTED 8.8s |
| 78 | 71 | 13,26,36,39,52,65 | 1728 | 6 | 15 | 21 | 23 | 1 | COULD-NOT-RUN @W22 (needs 23) |
| 79 | 77 | 39 | 1 | 1 | 18 | 19 | 21 | 1 | REFUTED 4.8s |
| 80 | 74 | 16,32,48,64,77 | 200 | 2 | 17 | 19 | 21 | 0 | REFUTED 1.6s |
| 81 | 79 | 40 | 27 | 1 | 18 | 19 | 21 | 0 | REFUTED 0.2s |
| 82 | 79 | 40,41 | 64 | 6 | 17 | 23 | **—** | 1 | UNREACHABLE |
| 83 | 81 | 41 | 1 | 1 | 19 | 20 | 22 | 1 | REFUTED 4.0s |
| 84 | 71 | 12 drops | 5184 | 4 | 17 | 21 | 23 | 1 | COULD-NOT-RUN @W22 (needs 23) |
| 85 | 79 | 17,34,40,51,68 | 25 | 2 | 18 | 20 | 22 | 1 | REFUTED 3.3s |
| 86 | 83 | 42,43 | 64 | 6 | 18 | 24 | **—** | 1 | UNREACHABLE |
| 87 | 83 | 29,42,58 | 81 | 4 | 18 | 22 | 24 | 1 | COULD-NOT-RUN @W22 (needs 24) |
| 88 | 79 | 11,22,33,40,44,55,66,77 | 64 | 2 | 18 | 20 | 22 | 1 | REFUTED 1.3s |
| 89 | 87 | 44 | 1 | 1 | 19 | 20 | 22 | 1 | REFUTED 3.6s (+W23 5.6s, W24 53.5s, r<=1 195.3s) |

## Minimum width per base — the reusable artefact

| min W | bases |
|------:|---|
| 18 | b65, b67 |
| 19 | b71, b72 |
| 20 | b68, b69, b73, b77 |
| 21 | b66, b75, b76, b79, b80, b81 |
| 22 | b70, b83, b85, b88, b89 |
| 23 | b78, b84 |
| 24 | b74, b87 |
| — | **b82, b86 — unreachable at any supported width** |

## b89 in depth

The range's largest prize (170 decimal digits, `|D|=87`) and the u128
arithmetic ceiling. Its descending prefix is now settled across the whole
supported width range:

| probe | wall | verdict |
|---|---:|---|
| W=22 | 3.638 s | REFUTED |
| W=23 | 5.605 s | REFUTED |
| W=24 | 53.476 s | REFUTED |
| release layer r≤1 (65 prefixes) | 195.306 s | all refuted, 0 declined |

**Measured, and it closes a real unknown:** 65 terminals in 195.306 s =
**3.005 s each**, against 3.638 s for the descending terminal — **ratio
0.83×**. Non-descending terminals are *not* more expensive. That converts the
deeper layers from projection to budget: `r≤2` = 2,081 prefixes ≈ **1.74
CPU-h**; `r≤3` = 43,745 ≈ 36.5 CPU-h. Prefix counts are exactly `C(64, k)`.

Width ratios were **1.54× then 9.5×** — irregular, consistent with every
other base.

## Correction: the engine's `T` starts at 1

An earlier recon computed `T` as *minimal `t ≥ 0` with `B^t ≡ 0 (mod L_nil)`*,
giving **T = 0** when `L_nil = 1`. The engine does not:

```cpp
for (T = 1; T <= 12; T++) { p = (p * (u128)B) % c.Lnil; if (p == 0) { ... } }
```

The loop **starts at 1**, so `L_nil = 1` ⇒ **`T = 1`**, as the engine's own
logs printed throughout (`T=1 Pc=19`). The table above uses the engine's
definition. Nothing downstream changed (the primes' `T+Pc` moved 19→20, still
inside 22), but a base at `T+Pc = 21` or `22` would have flipped
reachability — see footguns §17.

## Cost ledger

Eight quantities **derivable from the digit set** have held without exception:
forced sets, the b64 obstruction, `r0`, `L_nil`/`T`, prefix counts `C(P,k)`,
window-fit, reachability, minimum-width-per-base.

Seven quantities **inferred from observed behaviour** have all failed: census
cheapness, filter strength, release-axis advantage, terminal cost,
"found at the top rung", cheap-up-here, and per-width cost ratios.

> **Everything derivable from the problem statement has held; everything
> extrapolated from one base's behaviour has failed.** Cost appears not to be
> a property of a base at all, but of the interaction between its constants
> and the planner's chosen split — for which we have no theory.

## Round 2 — bases re-run at their correct minimum widths

The W=22 sweep could not address six bases. Round 2 re-runs them at the width
each actually needs (derived, not guessed), and re-runs the two timeouts with
a larger cap. Results so far:

| base | width | outcome |
|---|---:|---|
| b87 | 24 | **REFUTED**, 678.869 s |
| b84 | 24 | **REFUTED**, 11.611 s |
| b82 | 24 | `DECLINED: NX+NY<2` — **UNREACHABLE confirmed by run**, as derived |
| b74 | 24 | running |
| b78 | 24 | queued |
| b86 | 24 | queued — **derived UNREACHABLE** (`T+Pc = 24`); the run will only confirm it |
| b66 | 22 (cap 7200 s) | queued |
| b70 | 22 (cap 7200 s) | running |

**Running tally across 65–89: 19 REFUTED, 0 completions, 1 UNREACHABLE
confirmed + 1 derived, 4 still open.** Every refutation is of that base's
*descending prefix at one width* and is bounded on both axes.

## Open, not concluded

- Round 2 running: b74/b87 at W=24, b78/b84 at W≥23, b66/b70 at a 7200 s cap.
- b89 `r≤2` running (2,081 prefixes).
- **Hypothesis, explicitly not a conclusion:** the §8 bases may have been
  tractable because their forced sets happened to be *near-descending-
  completable*, and this range may simply not be. Held at 17 of 19; several
  bases remain unsettled, and any single hit would move it.
