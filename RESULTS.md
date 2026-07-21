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
| 42 | COMPLETE, eng=S, subsets=2293810 | 116.0s | **MATCH** (650828754201915243697436482327806321775960031977171416000) |
| 43 | COMPLETE, eng=S, subsets=23 | 1430.0s | **MATCH** (9374765438594117074250580509957460813483601689280434597219126021600) |
| 44 | COMPLETE, eng=S, subsets=50789 | 2067.9s | **MATCH** (12428520662963836843722648681332672663852331283150932087854716800) |
| 45 | COMPLETE, eng=S, subsets=490899 | 551.9s | **MATCH** (29858202121833974366127520253547500517971464443016809773917504800) |
| 46 | COMPLETE, eng=S, subsets=300 | 678.4s | **B-FILE ERROR FOUND** — see below |

### Base 46: published OEIS value is suboptimal

- b-file:  315044747190120671695735975284033252460559821155925276163089767538975200
- solver:  315044747190120671695735975284412123404260147529994283460952247723479200 (larger)
- Independently verified in Python: BOTH values use the same 43-digit subset
  {1..45}\{22,23} (itself uniquely forced: ten-rule removes 23, then digit-sum
  mod 45 forces d≡22), both divisible by lcm = 409547311252279200.
- The b-file arrangement breaks descending order at digit 28 (…29,27,26,…28
  buried in the tail); the solver's keeps 28 in place (…29,28,27,…) and is
  strictly greater. The published a(46) is therefore not maximal.

## Base 49 (out-of-range target; jes's independent answer, ~1 week on a 2020 laptop)

- jes's answer decodes (0-9, A-Z=10-35, α..ν=36-48) to
  27480664153312064994836939532520844560984511658005210290838348871641700981823200
  = 47 distinct digits, missing exactly {24}, divisible by lcm — self-consistent.
- Theory validated at subset level: 49 ≡ 1 (mod 16) and (mod 3) force digit-sum
  conditions that UNIQUELY determine the removed digit d=24 (d≡8 mod 16, d≡0 mod 3).
- v2 baseline 1h run: TIMEOUT. stderr shows the whole hour spent inside exactly
  that one subset (filters isolated it instantly; budget escalated to 4.3e9).
  Conclusion: b49's cost is arrangement-search inside an ALIVE subset — subset
  filters are already optimal here; engine speed is the whole game. b49 is now
  the primary optimization benchmark (known answer, known-hard).
