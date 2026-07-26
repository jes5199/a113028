# Wall-clock records and hardness metrics, per base

> **This table deliberately contains no values and no evidence-status
> labels.** Values live in the README table and `b113028.txt`; statuses live
> in the README and [FRONTIER-STATUS.md](FRONTIER-STATUS.md), which the
> pre-commit ladder checker keeps consistent. Keeping a third copy here is
> how prose/table drift starts (LESSONS 13d/13e), so this file carries only
> what nothing else does: timings and hardness metrics.

Wall-clock times are single-core canonical bests measured on this box
(quiet, nice'd; full record sweep 2026-07-22 — winning engine varies per
base: v4 incremental scan on most, v12/v13 peeling where nilpotent
structure pays). m\* and log₁₀ W are the hardness metrics defined in
[../engine/METHOD.md](../engine/METHOD.md) — m\* is the divergence depth
(how many trailing positions differ from the plain descending arrangement)
and W ≈ m\*!/P! is the predicted size of the irreducible search band.

Rows for bases 41–49 also carry the verdict of the 2026-07-23 autonomous
bucket-cert sweep (carrytrie `cert` mode — subset discovery + shallow-bucket
join + direct verification, self-checked against the known value): *bucket ✓
time* = certified by the autonomous driver in that wall-clock — one uniform
measurement basis across 41–49 (hand-tuned per-branch bests, where they
exist, are labeled separately); *bucket ✗* = honest bucket failure (reason +
scan-fallback time in [RESULTS.md](RESULTS.md)). Sweep total: 8/9 bases
bucket-certified in 47m 57s combined, 1 memory-declined base rescued by scan.

For bases 50–64 the wall-clock shown is the canonical certification run from
[FRONTIER-STATUS.md](FRONTIER-STATUS.md).

| base | wall-clock | m\* | log₁₀ W |
|-----:|-----------:|----:|--------:|
| 2–15 | 0.000s each | 1–8 | 0.0–2.5 |
| 16 | 0.021s | 8 | 2.5 |
| 17 | 0.003s | 9 | 3.5 |
| 18 | 0.001s | 9 | 2.7 |
| 19 | 0.002s | 10 | 3.7 |
| 20 | 0.005s | 10 | 3.7 |
| 21 | 0.002s | 10 | 3.7 |
| 22 | 0.006s | 10 | 3.7 |
| 23 | 0.014s | 11 | 3.9 |
| 24 | 0.009s | 12 | 5.0 |
| 25 | 0.16s | 12 | 5.0 |
| 26 | 0.035s | 12 | 5.0 |
| 27 | 0.084s | 13 | 5.2 |
| 28 | 0.045s | 12 | 5.0 |
| 29 | 0.032s | 13 | 5.2 |
| 30 | 0.795s | 13 | 5.2 |
| 31 | 0.125s | 14 | 5.4 |
| 32 | 3.53s | 16 | 6.8 |
| 33 | 0.059s | 15 | 6.6 |
| 34 | 1.0s | 15 | 6.6 |
| 35 | 0.498s | 15 | 6.6 |
| 36 | 1.5s | 15 | 6.6 |
| 37 | 16.8s | 16 | 6.8 |
| 38 | 11.3s | 16 | 6.8 |
| 39 | 46.4s | 16 | 6.8 |
| 40 | 5.81s | 15 | 6.6 |
| 41 | 24.9s · bucket ✗ (mem), scan 44.1s | 17 | 8.0 |
| 42 | 1m 12.6s · bucket ✓ 5m 58.3s | 18 | 8.2 |
| 43 | **13m 51.5s** · bucket ✓ (record — beat the 16m 8s scan even under concurrent load) | 18 | 8.2 |
| 44 | **2m 0.4s** · bucket ✓ (record — was 20m 40.7s scan; 10.3×) | 19 | 9.5 |
| 45 | **1m 10.3s** · bucket ✓ (record — was 5m 43.8s scan; 4.9×) | 19 | 9.5 |
| 46 | 6m 28.2s · bucket ✓ 21m 11.4s (independently re-certifies the corrected value) | 19 | 9.5 |
| 47 | **3m 10.5s** · bucket ✓ (record — was 4m 1.9s scan) | 20 | 9.7 |
| 48 | 10.35s (hand-tuned bucket best) · bucket ✓ 18.3s | 20 | 9.7 |
| 49 | ~4.4s (hand-tuned bucket best) · bucket ✓ 16.3s | 21 | 9.9 |
| 50 | 49.0s (certauto) | 21 | 9.9 |
| 51 | 50.1s (certauto; v4 concordance arm 5h51m) | 21 | 11.0 |
| 52 | 21.5s (certauto) | 21 | 11.0 |
| 53 | 279s (certauto) | 22 | 11.3 |
| 54 | 522.7s (certset @W22) | — | — |
| 55 | 7.1s (certauto) | 22 | 11.3 |
| 56 | 677s (@W21; outer proof 314s) | 22 | 11.3 |
| 57 | 46s (certauto) | 22 | 11.3 |
| 58 | 236s (@W21 post-calibration; outer proof 9559s) | 22 | 11.3 |
| 59 | 1774s (certset @W22) | — | — |
| 60 | 13.9s (@W21, post-churn-fix; was NO-VALUE at >5400s) | 23 | 11.5 |
| 61 | 1245.1s (decomposition child 16 of 23, @W22) | — | — |
| 62 | 1844s (certset @W22) | — | — |
| 63 | 136.1s resume-verify (after a 17h shard grind later shown unnecessary) | — | — |
| 64 | 1788s (certset @W24) | — | — |

Bases 2–15 individually: all 0.000s; their m\* values are 1, 1, 2, 3, 3, 4,
5, 5, 5, 6, 6, 7, 7, 8 and log₁₀ W runs 0.0, 0.0, 0.0, 0.5, 0.8, 0.6, 1.3,
0.7, 1.3, 1.5, 1.5, 2.3, 1.6, 2.5 (bases 2…15 in order).

The dashes past base 53: m\* is defined against the plain descending
arrangement, and for the deep-band bases the certification route (terminal
windows + outer branch-and-bound) never needed the band-size estimate — see
[MAXIMALITY-ARGUMENTS.md](MAXIMALITY-ARGUMENTS.md) for what replaced it.
