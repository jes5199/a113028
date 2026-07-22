# More-cores analysis (frontier primes; drafted 2026-07-22, delivery after final table)

Model: band-sweep W = m*!/P! nodes; single-core rate 3.1e6 nodes/s
(anchor: b43 = 3.0e9 nodes / 968s measured); wrong-turn multiplicity
smear ×3–30 (central ×10; calibrated against b48's 2h non-finish);
parallel efficiency 0.85 (work-stealing over ~B² top-level refutation
subtrees; heaviest branch ≈ 1/m* of total → ceiling ≫ 64 cores, so
near-linear holds to 64c).

| B | m* | P | W | 1-core (central) | 32c | 64c |
|---|----|---|---|------------------|-----|-----|
| 53 | 22 | 12 | 2.35e12 | ~88d (26–263d) | 3.2d | **1.6d** |
| 59 | 23 | 13 | 4.15e12 | ~155d | 5.7d | **2.8d** |
| 61 | 25 | 14 | 1.78e14 | ~18y | 244d | 122d |
| 67 | 26 | 15 | 3.08e14 | ~32y | 423d | 212d |
| 71 | 28 | 16 | 1.46e16 | ~1490y | 55y | 27y |

Verdict: NOTHING crosses "afternoon" (<8h) at 64 cores under the
central estimate; b53 approaches ~12h only if the optimistic ×3
multiplicity holds. 64 cores buys **b53 and b59 as multi-day runs —
exactly two more bases — then the wall reasserts at b61** (4+ months
at 64c). Peeling is inert at all of these (T=0). This is the
Irreducibility Law in dollars: ×64 compute ≈ log_factorial(64) ≈ 2
bases. Cost: 64 vCPU ≈ $1.5–3/hr → b53 ≈ $60–120, b53+b59 campaign
≈ $150–350, b61 ≈ $9–18k (not recommended).

## Post-49 composites (b50–b52, peeling applies) — added per request

| B | Λ_nil | s | m* | P_c | band W | 1-core (×10 mult) | 64c | verdict |
|---|-------|---|----|----|--------|-------------------|-----|---------|
| 50 | 800 | **5** | 19 | 10 | 3.4e10 | ~30h | ~0.6h | needs-cores 1-core / afternoon-with-day-rental; **s=5 needs an enumerator upgrade in v13** (residue-guided tuple search; admissible ≈ 2e5 of 49⁵) — without it b50 falls to scan (b48-class, likely timeout) |
| 51 | 459 | 3 | 20 | 11 | 6.1e10 | ~2.3d | ~1h | v13-ready (s=3); overnight 1-core / afternoon 64c |
| 52 | 416 | 3 | 20 | 11 | 6.1e10 | ~2.3d | ~1h | v13-ready (s=3); same class as 51 |

Where the frontier actually sits past 49: **b50–b52 are overnight-to-
few-days single-core, afternoon-class with modest cores, all
peeling-eligible** (b50 conditional on the s=5 enumerator upgrade —
a contained v13 change, not new theory). **b53 is where it turns
genuinely hard**: first peeling-inert base past 49, raw-search-only,
~88d single-core / ~1.6d at 64 cores. Caveats: same ×3–30 multiplicity
smear as the prime table; witness-count per subset unmeasured for
50–52 (b45's lesson: large witness families kill liveness-prune
value — but v13's per-suffix + domination is the right engine here,
and its b48/b49 auditions tonight are the direct calibration).
