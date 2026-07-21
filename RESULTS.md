# A113028 results log

Ground truth: OEIS b-file (b113028.txt), bases 2–48.
Binary: gcc -O2 -march=native, v2 two-engine source (a113028.c).

## Validation status
- Bases 2–39: **all match** the b-file exactly (38/38). Engine/timing below.
- Bases 40–48: in progress, bounded per-base runs (1h wall-clock cap each).

## Timing (validated bases, v2 baseline)

Bases 2–33 each completed in under 1.5s and match the b-file (engine mix S/D;
full verbose lines preserved in run_output.txt). Slow tail of the validated
range:

| base | engine | subsets checked | scanned | time |
|------|--------|-----------------|---------|-------|
| 34 | S | 171 | 1 | 1.8s |
| 35 | S | 139212 | 1 | 0.5s |
| 36 | S | 22943 | 1 | 2.1s |
| 37 | S | 20 | 1 | 27.5s |
| 38 | S | 210 | 1 | 116.4s |
| 39 | S | 3196 | 1 | 69.1s |

Note: on every slow base, `scanned=1` — the order-1/2 filters already reduce to
a single surviving subset, and essentially all wall-clock is engine work inside
it. So subset-level filters only help further if that survivor is *dead*;
otherwise the win has to come from faster engines (prefix pruning, suffix
enumeration).
| 40 | — | — | — | >1h (first run killed mid-flight at 17e9-node budget; bounded rerun in progress) |

## Tail runs (bases 40–48)
(updated as they land)

| base | outcome | wall-clock | validation |
|------|---------|-----------|------------|
| 40 | TIMEOUT (1h cap, v2 baseline) | >3600s | deferred — b-file says 2949491266532658135493053371770319593915307331801883500; awaiting optimized engine |
| 41 | COMPLETE, eng=S, subsets=22 | 49.0s | **MATCH** (791222981626154999235100158499550255615325307057668641337271200) |
