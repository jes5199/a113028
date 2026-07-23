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
| 40 | **SOLVED by Engine C (v9)**, eng=C, subsets=502588 | 6856.5s (~114 min) | **MATCH** (2949491266532658135493053371770319593915307331801883500) — published a(40) CONFIRMED; its divergence-law flag was a false alarm (legit +3 outlier) |
| 41 | COMPLETE, eng=S, subsets=22 | 49.0s | **MATCH** (791222981626154999235100158499550255615325307057668641337271200) |
| 42 | COMPLETE, eng=S, subsets=2293810 | 116.0s | **MATCH** (650828754201915243697436482327806321775960031977171416000) |
| 43 | COMPLETE, eng=S, subsets=23 | 1430.0s | **MATCH** (9374765438594117074250580509957460813483601689280434597219126021600) |
| 44 | COMPLETE, eng=S, subsets=50789 | 2067.9s | **MATCH** (12428520662963836843722648681332672663852331283150932087854716800) |
| 45 | COMPLETE, eng=S, subsets=490899 | 551.9s | **MATCH** (29858202121833974366127520253547500517971464443016809773917504800) |
| 46 | COMPLETE, eng=S, subsets=300 | 678.4s | **B-FILE ERROR FOUND** — see below |
| 47 | COMPLETE, eng=S, subsets=25 | 386.2s | **MATCH** (1754681573582232514378787438934811607312193893426436545338224933544695955200) |
| 48 | **CERTIFIED CORRECT** (nilpotent-peeling certificate, NILPOTENT-PEELING.md; last digit forced =24) | cert ~3.3 min (jes-side; our v12 reproduction queued) | **MATCH** (94237804886307950779486130179671488973571078333724597158459950718126090200) — divergence-law flag refuted (nilpotent suffix effect) |

**Tail sweep complete (v2 baseline, 1h caps): 7 complete (41–47), all validated
— 6 matches + 1 published-value error found (46, ours strictly larger, doubly
verified). 2 timeouts (40, 48), deferred to the optimized engine, alongside
benchmark target 49.**

### Base 46: published OEIS value is suboptimal

- b-file:  315044747190120671695735975284033252460559821155925276163089767538975200
- solver:  315044747190120671695735975284412123404260147529994283460952247723479200 (larger)
- Independently verified in Python: BOTH values use the same 43-digit subset
  {1..45}\{22,23} (itself uniquely forced: ten-rule removes 23, then digit-sum
  mod 45 forces d≡22), both divisible by lcm = 409547311252279200.
- The b-file arrangement breaks descending order at digit 28 (…29,27,26,…28
  buried in the tail); the solver's keeps 28 in place (…29,28,27,…) and is
  strictly greater. The published a(46) is therefore not maximal.
- Independently re-verified from scratch by boss-clod (second Python check,
  2026-07-21): same subset, same lcm, both divisible, ours strictly larger.
  Relayed to jes as an OEIS-correction candidate. Consequence: the b-file is
  no longer a blind oracle for remaining bases — any divergence gets the same
  treatment (verify validity + strict comparison before assigning blame).

## Engine C benchmarks (v6, idle box, vs v2 baseline)

| base | v2 | v6 | ratio |
|------|----|----|-------|
| 33 | 0.04s | 0.056s | ~parity |
| 36 | 2.1s | 12.1s | 5.8× slower |
| 37 | 27.5s | 34.1s | 1.24× slower |
| 38 | 106s (idle A/B) | 12.2s | **8.7× faster** |
| 39 | 69s | 69.2s | parity |
| 43 | 1430s | 2738s | 1.9× slower |

Pattern: v6 wins where the band has strong e2/e3 structure (b38), loses where
the band is wide with weak mid-band pruning (b36, b43). v7 adds quantitative
leaf widening (only when band pruning power ≤ 32): b36 12.1s → 2.4s (parity).

## Wall-clock records campaign (2026-07-22, canonical = this box)

| base | prior best | new | engine | verdict |
|------|-----------|-----|--------|---------|
| 43 | 1430s (v2) | **968.0s** | v4 (incremental scan) | value matches b-file ✓ — RECORD |
| 40 | 6856s (v9) | **14.322s** → 9.03s (v13) → **5.81s** (v13 fixed) | v12→v13 peeling | value matches ✓ — RECORD chain |
| 48 | no our-box solve (24h grind pending) | **10.35s CERTIFIED** (both branches, 69 suffix triples, gates passed, value char-exact) | shallow-bucket join, auto-configured | 19× Sol's own verifier; prefix-release digit 24 DISCOVERED from congruences |
| 49 | no our-box solve | **~4.4s certificate-equivalent** (cand-21 zero + cand-20 known completion, direct-verified) | shallow-bucket join | b49's cert structure reproduced our-box |

Monster attempts: b49 under v6 = 1h TIMEOUT (while sharing the box with b40;
band sweep estimate ~8.4e9 nodes ≈ 42 min solo — near miss). Next: solo 2h
rerun, then v8 = Barrett constants for tracked-moduli updates (3–5× node cost)
if needed. b40 under v7 (wide leaf) in progress.

## Base 49 (out-of-range target; jes's independent answer, ~1 week on a 2020 laptop)

- jes's answer decodes (0-9, A-Z=10-35, α..ν=36-48) to
  27480664153312064994836939532520844560984511658005210290838348871641700981823200
  = 47 distinct digits, missing exactly {24}, divisible by lcm — self-consistent.
- Theory validated at subset level: 49 ≡ 1 (mod 16) and (mod 3) force digit-sum
  conditions that UNIQUELY determine the removed digit d=24 (d≡8 mod 16, d≡0 mod 3).
- v2 baseline 1h run: TIMEOUT. stderr shows the whole hour spent inside exactly
  that one subset (filters isolated it instantly; budget escalated to 4.3e9).
  Conclusion: b49's cost is arrangement-search inside an ALIVE subset — subset
  filters are already optimal here; engine speed is the whole game.
- **2026-07-22: a(49) CERTIFIED** via the nilpotent-peeling certificates
  (NILPOTENT-PEELING.md): candidate-21 refuted exhaustively (2 × 253,955,520
  = 19!/12! leaves — arithmetic independently checked), candidate-20 yields
  exactly jes's laptop-week value, digit-for-digit. a(49) =
  27480664153312064994836939532520844560984511658005210290838348871641700981823200
  — the first value beyond the published b-file.

**Divergence-law audit, final scoreboard: 3 flags → 1 real error found and
corrected (a46), 2 cleared (a40 by direct solve, a48 by certificate — both
"outliers" were forced nilpotent-suffix effects, now understood).**

## Bucket-engine scorecard (2026-07-23 sweep, carrytrie `cert` mode)

Autonomous per-base certifier: subset discovery + divergence + shallow-radix
bucket join + direct verification, self-checked against the known answer.
BUCKET-only times — no scan fallback. Sequential sweep, nice 19, per-base
1800s cap, 3GB RSS self-abort + 6GB address-space backstop. b40 excluded
(5.81s v13 peeling record stands; bucket untested there pre-crash).

| base | bucket outcome | wall | self-check |
|------|----------------|------|------------|
| 41 | **bucket FAILED — memory** (auto-configured build ballooned to 8.9GB peak RSS >3GB budget at `certdrv-bucket-post-generation`, clean self-abort at 197.9s; prime base, no nilpotent peel — expected hard case). **Solved by scan fallback (v4, eng=S): 44.1s** | bucket: abort @197.9s · scan: 44.1s | **MATCH** |
| 42–49 | in flight (bucket-first, scan fallback on bucket failure) | | |
