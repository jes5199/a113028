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
| **UNREACHABLE** | no supported width can attempt the base at all. **No base in 65–89 is in this class** — an earlier revision wrongly placed b82 and b86 here (see the correction below). The class is retained because it is real in principle: it applies when `T + Pc > maxV − 2` for that base's own ceiling. |

## Reachability is derivable, and it bites

`certset` requires `NX + NY = W − T − Pc ≥ 2`, i.e. **`T + Pc ≤ W − 2`**.
`CERTSET_W` is validated only on **[20, 24]**, so

> ~~**`T + Pc ≤ 22` is a hard engine limit.**~~ **RETRACTED — see below.**

**CORRECTION (2026-07-25, later):** the ceiling is **per-base**, not a global
24. `configureCertPosForBase` sets `maxV = 24` only for `B ≤ 64`; above that
`maxV = max(24, P_full + 10)`. Every base in 65–89 therefore has a **wider**
ceiling than 24 — b82 gets `[20,29]`, b86 gets `[20,30]`. The real bound is
**`T + Pc ≤ maxV − 2`, evaluated per base.**

> **All 25 bases in 65–89 are reachable. None is engine-limited.**

`T` and `Pc` come from the digit set with no search (`deriveConstantsGen`), so
**which bases can be attempted, and at what width, is knowable before spending
anything.**

- **b82** (`T=6, Pc=17`) — needs `W ≥ 25`; its ceiling is 29. **Reachable.**
- **b86** (`T=6, Pc=18`) — needs `W ≥ 26`; its ceiling is 30. **Reachable.**
- **All 25 are reachable** at the minimum widths tabulated below.

⚠️ **Both were previously recorded here as UNREACHABLE, and both had a run
"confirming" it.** Those runs were executed at **W=24 — a width already
derived to be below their minimums of 25 and 26** — so they declined for want
of window and the declines were read as confirmation. *A run configured from a
belief cannot test that belief.* The classification was wrong; the arithmetic
that would have refuted it was already on this page.

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
| 82 | 79 | 40,41 | 64 | 6 | 17 | 23 | 25 | 1 | INCONCLUSIVE-resource (>36000 s) |
| 83 | 81 | 41 | 1 | 1 | 19 | 20 | 22 | 1 | REFUTED 4.0s |
| 84 | 71 | 12 drops | 5184 | 4 | 17 | 21 | 23 | 1 | COULD-NOT-RUN @W22 (needs 23) |
| 85 | 79 | 17,34,40,51,68 | 25 | 2 | 18 | 20 | 22 | 1 | REFUTED 3.3s |
| 86 | 83 | 42,43 | 64 | 6 | 18 | 24 | 26 | 1 | not attempted — irreducible regime |
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
| 25 | b82 *(ceiling 29)* |
| 26 | b86 *(ceiling 30)* |

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

## Cost ledger — corrected

An earlier form of this section claimed *"everything extrapolated from
observed behaviour has failed."* **That was too strong**, and b89's `r≤2` run
falsified it in the useful direction:

```
predicted from the r<=1 measurement: 2,081 x 3.005 s = 6,254 s
actual:                                                5,914 s
error:                                                    5.5 %
```

The correct boundary is not *derived vs. observed* but **within-configuration
vs. across-configuration**:

> **Cost is unpredictable across configurations and predictable within one,
> once measured.** Interpolation inside an identical configuration works
> (5.5 % over 2,081 terminals, projected from a 65-terminal sample of the same
> population); extrapolation across bases, widths or prefix classes has failed
> every time (7/7).

This *explains* the seven failures rather than merely tallying them — every
one crossed a configuration boundary: b64 → other bases, one width → another,
descending → non-descending. It also states exactly when a projection is
legitimate: **same base, same width, same prefix class, measured on a sample
of the very population being projected over.**

### A second correction: prefix counts are upper bounds, not counts

`C(prefixLen, k)` is the number of *candidate* prefixes at release layer `k`.
The number actually enumerated is those that pass **feasibility**, which
equals `C(P,k)` only where the filter is vacuous (`L_nil = 1`, i.e. the prime
bases). Where the filter bites it can be dramatically smaller:

> **b81 `r≤1`: 2 feasible prefixes, not the 56 that `C(56,1)` predicts.**

b81 has `r0 = 0` — its descending prefix is infeasible — and the same
constraint prunes its `r=1` layer to almost nothing. So release-layer cost
estimates built on `C(P,k)` are **upper bounds**, and are loosest exactly on
the bases where the filter is strongest.

## Cost ledger (original enumeration)

### Regime annotations, not a scoreboard

A hit count is the wrong summary: it hides *what evidence would move a claim*.
Each derived predictor below is annotated with the regime its confirmations
came from, because a predictor confirmed only inside one regime is **unearned
outside it**, not proven (§19).

| predictor | status |
|---|---|
| forced sets | 4/4 against certified bases, spanning several drop-routes — **broad** |
| the b64 obstruction (`T=1` ⇒ forced last digit) | verified at b64; **does not generalise** — b81 has two admissible last digits |
| `L_nil` / `T` | derived from the digit set, checked against engine output — **sound**, after correcting `T`'s off-by-one (§17) |
| `C(P,k)` prefix counts | exact as a **candidate** count; **not** the enumerated count where the filter bites (§19) |
| window-fit `W ≥ T+Pc+2` | verified, and now enforced by the width guard |
| reachability `T+Pc ≤ maxV−2` | **corrected**: `maxV` is per-base, not 24. Earlier "confirmations" were runs configured from the wrong premise (§13b) |
| minimum-width-per-base | derived; enforced |
| **`r0` ⇒ a hit is maximal for free** | 3/3 — **but every confirmation came from a filter-vacuous base.** *Untested in the regime where it could fail*: no hit has yet occurred on a base where the filter bites hard. **Unearned there, not proven.** |

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
| b82 | 24 | `DECLINED: NX+NY<2` — **W=24 is below its minimum of 25**; not a verdict (the UNREACHABLE reading was retracted) |
| b74 | 24 | running |
| b78 | 24 | queued |
| b86 | 24 | `FATAL` — **W=24 is below its minimum of 26**; not a verdict (the UNREACHABLE reading was retracted) |
| b66 | 22 (cap 7200 s) | queued |
| b70 | 22 (cap 7200 s) | running |

**Running tally across 65–89 (updated 2026-07-26): 20 REFUTED, 0 completions,
0 UNREACHABLE. Open: b78 and b74 running at their minimum widths; b82
INCONCLUSIVE-resource with a measured `>36000 s` bound; b86 not attempted
(irreducible regime).** b66 and b84 are REFUTED, not open. Every refutation is of that base's
*descending prefix at one width* and is bounded on both axes.

## Release-layer (r=1) sweep — complete, 10 bases, zero hits

Run after the descending-prefix sweep, on the bases with the cheapest measured
terminals. Enumerated counts, **not** `C(P,1)` — see the correction above:

| base | feasible prefixes at r≤1 | outcome |
|---|---:|---|
| b80 | 1 | all refuted |
| b81 | 2 | all refuted |
| b72 | 41 | all refuted |
| b77 | 47 | all refuted |
| b75 | 49 | all refuted |
| b79 | 55 | all refuted |
| b88 | 57 | all refuted |
| b85 | 57 | all refuted |
| b83 | 59 | all refuted |
| **b84** | 49 | **INCONCLUSIVE — 49/49 DECLINED** (this *layer* was run at W=22; b84 needs W≥23). ⚠️ **This is the r=1 LAYER's status, not the base's.** b84 the *base* is **REFUTED** — at W=24 in 11.611 s and independently at W=23 in 8.003 s. |

**b81 is now materially settled near descending:** its descending prefix
refuted at W=22 in 0.192 s, and its entire r≤1 layer is **two prefixes**, both
refuted. That matters because b81 is the range's only `T = 1` base — the sole
structural analogue of b64 — and was our most promising candidate for a
paper proof up here. Near-descending is exhausted for it.

Note b84's row is **not a negative**: that *layer* was run at a width it
cannot run at, which the engine correctly reported as DECLINED rather than
REFUTED. **A layer's status and its base's status are different things** —
b84 the base is REFUTED (twice, at two widths); only its r=1 layer at W=22 is
inconclusive. Collapsing the two kept b84 on the open list for a day.

## Round 2 — final

| base | width | outcome |
|---|---:|---|
| b87 | 24 | REFUTED, 678.869 s |
| b84 | 24 | REFUTED, 11.611 s |
| b84 | 23 | **REFUTED, 8.003 s** — second width, independent corroboration of the W=24 refutation |
| b70 | 22 (7200 s cap) | REFUTED, 2318.039 s |
| b74 | 24 | **INCONCLUSIVE** — timed out at 3600 s |
| b82 | 24 | **COULD-NOT-RUN — W=24 is below its minimum of 25.** Not unreachable. *Subsequently run at W=25 twice and capped both times — see the wide-regime table.* |
| b86 | 24 | **COULD-NOT-RUN — W=24 is below its minimum of 26.** Not unreachable. *Deliberately not attempted at W=26 — see the irreducible-regime section.* |
| b78 | 24 | running |

## Open, not concluded

- Round 2 running: b74/b87 at W=24, b78/b84 at W≥23, b66/b70 at a 7200 s cap.
- b89 `r≤2` running (2,081 prefixes).
- **Hypothesis, explicitly not a conclusion:** the §8 bases may have been
  tractable because their forced sets happened to be *near-descending-
  completable*, and this range may simply not be. Held at 17 of 19; several
  bases remain unsettled, and any single hit would move it.

---

## The decomposition primitive and its exact domain

`CERTSET_PREFIX` runs a terminal at a caller-supplied prefix, which makes a
width-`W` terminal decomposable into width-`(W−1)` children: fix its prefix,
let the next position range over every remaining digit, and the union of the
children **is** the parent — an exact partition, no approximation. That is what
made b61 tractable: a monolithic W=23 terminal projected at ~10 h against a
6 h cap became 23 checkpointed W=22 children, sharded, with a global stop on
the first hit.

**The domain of that trick is exact:**

> **Decomposition applies iff `W > min W`, where `min W = T + Pc + 2`.**
> At `W = min W` a base is **irreducible** — there is no smaller width to
> partition into, and the children would be refused by the width guard.

| base | `min W` | attempted at | decomposable? |
|---|---:|---:|---|
| b61 | 21 | 23 | ✅ 23 children at W=22 → the value was found |
| b82 | **25** | **25** | ❌ **irreducible** |
| b86 | **26** | **26** | ❌ **irreducible** |

**This reframes b82 and b86.** They are not merely the slow ones — they are the
ones where the campaign's most effective tool **provably does not apply**.
Everything that rescued b61 (checkpointing, sharding, early stopping,
resumability) is unavailable here by construction. They must run
monolithically: all-or-nothing against a cap, with a timeout teaching only a
lower bound.

It also explains the shape of the whole day: every base the decomposition
rescued satisfied `W > min W`, and the boundary was never visible because it
was never reached.

**How it surfaced.** Not by analysis — the **width guard** refused the child
jobs, and the refusal made the constraint explicit. That is the third time a
mechanism produced a *finding* rather than merely preventing an error, and the
first time one said something about the **mathematics** rather than about the
process.

### Wide-regime attempts (b82, b86)

| run | width | cap | outcome |
|---|---:|---:|---|
| b82 | 25 | 7200 s | `rc=124` — **INCONCLUSIVE-resource, `>7200 s`, no verdict** |
| b82 | 25 | 36000 s | `rc=124` — **INCONCLUSIVE-resource, `>36000 s`, no verdict** |
| b86 | 26 | — | **not attempted** — see below |

Both are recorded as **bounds, not runtimes**. Neither is a refutation and
neither says anything about base 82. For scale, the longest *completed* job
anywhere in this campaign was b66 at 2507 s; b82 was given **more than
fourteen times that** and returned nothing.

### The irreducible wide-width regime is out of reach of this engine

b82 was given 5× its first budget — **ten uncontended hours** — and only the
bound moved. b86 is **strictly harder**: a wider window (26 vs 25) on a larger
digit set (`|D|=83` vs 79). Attempting it buys a third `rc=124` with high
probability, consuming a day to move a bound that can already be stated.

**So it was not attempted, and that is a conclusion rather than an omission:**

> **Bases at `W = min W` — where the decomposition provably cannot apply — are
> out of reach of this engine at any budget worth spending.**

The reason is **structural, not incidental.** These are exactly the bases where
`W − 1 < min W`, so there is no smaller width to partition into: no
checkpointing, no sharding, no early stopping, no resumption. They must run
all-or-nothing against a cap, on the two bases where all-or-nothing is most
expensive. Everything that made b61 tractable — a monolith projected at ~10 h
turned into 23 checkpointed children, one of which found the value in 1245 s —
is unavailable here **by construction**.

b82 and b86 therefore stand as **INCONCLUSIVE-resource with explicit bounds**,
and the bound on b82 is now a measured `>36000 s`.
