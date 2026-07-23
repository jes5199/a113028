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
| 42 | **bucket CERTIFIED PASS** — autonomous subset discovery churned 2,293,434 subsets to find \|D\|=35 (dropped 6), winningCandidate=25, 3095 survivors all direct-verified, peak RSS ~7MB | 358.3s | **MATCH** |
| 43 | **bucket CERTIFIED PASS — NEW RECORD** (prime base, no peel: bucket still fit, peak RSS ~1.27GB; \|D\|=41 drop-1, candidate 22, 1351 survivors all verified; beat the 968s quiet-box scan record despite 4 concurrent nice-19 dual-campaign processes) | **831.5s** | **MATCH** |
| 44 | **bucket CERTIFIED PASS — NEW RECORD, 10.3×** (old best 1240.7s scan; \|D\|=39 drop-4, candidate 24, 71 survivors all verified, peak RSS ~61MB, 50,760 subsets churned) | **120.4s** | **MATCH** |
| 45 | **bucket CERTIFIED PASS — NEW RECORD, 4.9×** (old best 343.8s scan; \|D\|=39 drop-5, candidate 24, 67 survivors all verified, peak RSS ~892MB, 490,682 subsets churned) | **70.3s** | **MATCH** |
| 46 | **bucket CERTIFIED PASS** — independently re-certifies the CORRECTED a(46) (strictly larger than the published OEIS value); \|D\|=43 drop-2, candidate 21, 253 survivors all verified, peak RSS ~9.7MB. Slower than the 388.2s scan record — honest datum | 1271.4s | **MATCH (corrected value)** |
| 47 | **bucket CERTIFIED PASS — NEW RECORD** (prime; old best 241.9s scan; admission gate ADMIT at Y=1,395,360 / projected 0.11GiB — first in-sweep Phase A trace; \|D\|=45 drop-1, candidate 21, 17 survivors verified, peak RSS ~125MB) | **190.5s** | **MATCH** |
| 48 | **bucket CERTIFIED PASS** (autonomous apples-to-apples number; hand-tuned branch best remains 10.35s, labeled separately). \|D\|=44 drop-3, candidate 20, 1 wrong turn refuted, 5 survivors verified, peak RSS ~13MB, 15,800 subsets churned | **18.3s** | **MATCH** |
| 49 | **bucket CERTIFIED PASS** (autonomous; hand-tuned branch best ~4.4s labeled separately). \|D\|=47 drop-1, candidate 20, 1 wrong turn refuted, exactly 1 survivor = jes's laptop-week value, peak RSS ~126MB | **16.3s** | **MATCH** |

## Engine: certauto planner (Sol docs #22+#23, Phase A+B) — VALIDATED 2026-07-23

Plan-search (peeled ∥ full-modulus × NX/NY/K, exact suffix-family DP,
legacy-default-unless-3×-better rule, preflight memory gate, Declined ≠
Refuted). Gates: certauto 44/45/48/49 all CERTIFICATION PASS char-exact
(44 had hard-declined under Phase A's fixed NX=2 — the split search solved
it); regressions cert 48 = 10.3s PASS (matches the 10.35s record), certfm 49
= 27.8s PASS. Known limitation (Phase C TODO): certauto 41 admits the
memory-feasible 3+6/K=4 plan and times out at 1800s instead of work-declining
— no absolute work-vs-scan cap yet; the sweep's rc-based scan fallback
covers it. Binary: carrytrie_cert.

## Frontier climb b53→ (jes mandate: find where it gets hard; started 13:32 UTC)

| base | verdict | wall | notes |
|-----:|---------|-----:|-------|
| 53 | **PASS (single-method exhaustive) — strong, pending 2nd method** | 279s | prime; \|D\|=51 drop {26}; first strain signal: 13 wrong turns refuted, winning candidate 8 (deep descent vs ~21 on b50–52); value self-verified (lcm=3099044504245996706400 divides N, mod-52 rule holds) |
| 54 | **STOP — HARDNESS ONSET, mode = refute-and-descend** | 725s | 11 subsets scanned, **10 refuted only within the fixed 21-position window** before a completion at \|D\|=50 (drop {4,22,27}), winning candidate 1 (!), 20 wrong turns. Value below is a valid, self-verified completion but maximality rests on 10 window-bounded refutations — the exact b50-lesson exposure, multiplied. CERTPOS=21 probe launched. |

a(53) ?= 8667796530759171030732652761285454124185037606953473954566810515107048776076588809114400
a(54) ?? = 416421702506789485219242774659857217353557918404448765237845114648269563412650818364000 (weak candidate — see row 54)

## a(51) and a(52) — FIRST-EVER CERTIFIED, 2 METHODS EACH (2026-07-23 ~13:01)

- **a(51) = 180145958793036691752603389680297418249529280174180771266050386968626019795153600**
  certauto exhaustive cert in 50.1s (full-modulus NX=2/NY=6/K=4, forced
  first-scanned subset drop {17,24,34}, candidate 22 refuted → 21 wins, 1
  survivor verified) — matches the independent v4 Engine-S scan completion
  (21080.9s) char-for-char.
- **a(52) = 448735208793063714451606009674691709006633117645639135533102744646118644150575200**
  certauto exhaustive cert in 21.5s (peeled NX=2/NY=4/K=3, drop
  {13,24,26,39}, candidate 22 refuted → 21 wins, 2 survivors verified) —
  matches the v15 prefix-guess candidate (7463.6s) char-for-char.

The certauto engine (Sol docs #22+#23 + the b50 autopsy fix) certifies in
under a minute what the scan/candidate arms needed hours for. Hardness
computed per the README definitions: b51/b52 both m*=21, log₁₀W=11.0
(P=12 — the W jump comes from the shallower leaf horizon, not m*).

## Dual-campaign cap scorecard (caps expired ~12:53 UTC 2026-07-23)

- **b50 v4 arbiter: CONFIRMED a(50)** — Engine S completed in 8043.8s with
  exactly the certified value, char-for-char. a(50) now rests on THREE
  independent engines (fixed full-modulus certauto, peeled width-21, v4 scan).
- **b51 FIRST VALUE EVER** — v4 Engine S COMPLETED (not capped) in 21080.9s:
  a(51) ?= 180145958793036691752603389680297418249529280174180771266050386968626019795153600
  Self-verified: 47 distinct nonzero base-51 digits, dropped {17,24,34}
  (17/34 kill the 17-factor per the ten-rule; 24 restores digit-sum ≡ 0 mod
  50), lcm=182296735543882159200 divides N, MSB descending 50,49,...
  Status: single-method (scan) — certauto second-method run launched.
  The v15 candidate arm emitted nothing before its cap (banner only).
- **b52: v4 scan INCONCLUSIVE at 6h cap** (budget ladder reached 2^36
  mid-grind, no completion line). The v15 candidate stands as the only b52
  value, still pending a second method — certauto run launched.

## b50 — CERTIFIED (2026-07-23 ~11:10, post-autopsy engine)

a(50) = 71024679959360285134972854735006247396917902186058529912810323948019000719382560

Canonical run (fixed engine, default width, b50_certified.log): 49.0s,
subsetsScanned=1 (the forced 47-subset {1..49}\{24,25} is the first scanned),
family=full-modulus NX=1/NY=6/K=4, candidate 21 refuted, candidate 20 wins,
1 survivor direct-verified. Maximality argument: prefix = naive descending
(the value agrees with it for 26 digits); any deeper-diverging arrangement is
lex-smaller; the window search covers all shallower divergence with
candidates descending. Concordant second path: the width-21 PEELED run
(different family, unaffected by the FM bug) found the same maximum.
v4 scan arbiter still running as an independent third signal.

**The autopsy that made this possible:** the original width-20 run had
actually FOUND this exact completion in its join — then lost it in
verification: `BucketRecordFM::yDigitsPacked` was a uint32_t (30 usable bits
= 5 packed digits), but the Phase-B planner legally selects NY=6; the 6th
y-digit's bits truncated silently (digit 12 → 0), the strict direct verifier
rejected the corrupted reconstruction (correctly — no wrong answer was ever
emitted), and the candidate loop, having consumed its one success, silently
descended to |D|=46. Fixed by widening to uint64_t (commit "FIX FM false
negative"). The bug was invisible pre-Phase-B because fixed NX=2 kept NY≤5 —
b49's certfm validation could not have caught it. Defense-in-depth verdict:
the strict verifier prevented a wrong VALUE, the honesty labels prevented a
wrong CERTIFIED, and the escalation protocol surfaced the discrepancy.

## b50 — SUPERSEDED: 47-digit completion FOUND at window 21 (2026-07-23 ~10:37)

The Phase C escalation (CERTPOS=21) found, on the forced 47-digit subset
{1..49}\{24,25}, in 321.5s (peeled family, NX=1 NY=4 K=2, candidate 22,
3 survivors verified):

    a(50) ?= 71024679959360285134972854735006247396917902186058529912810323948019000719382560

Self-verified: 47 distinct nonzero digits missing exactly {24,25}, lcm =
619808900849199341280 divides N, digit-sum ≡ 0 mod 49, agrees with the naive
descending arrangement for 26 digits, diverges only in the last 21 positions.
STRICTLY LARGER than (and superseding) the 46-digit candidate below.

**Status: STRONG CANDIDATE — certification withheld a second time, now for an
engine-soundness reason.** The value's structure proves the width-20 run's
refutation contained a FALSE NEGATIVE: the completion has candidate digit 20
at position 20 with all remaining divergence in positions 0..19 — squarely
inside the width-20 full-modulus search of candidate 20 (which reported zero
survivors, roots=840 = candidates 21 and 20). So at least one engine path
(full-modulus at b50/width-20, or the candidate-iteration logic above it) can
MISS completions. Also unexplained: only 2 of ~21 pool candidates were tried
at width 20 before the subset was abandoned. Until that false negative is
root-caused, no refutation-dependent claim from these paths is trustworthy —
including this value's own maximality (which additionally rests on the
width-21 peeled search being complete for candidate 22's window; the
maximality argument is otherwise sound: candidate 22 is the naive digit, no
larger candidate exists at that position, deeper divergence ⇒ lex-smaller).
Arbiters: v4 scan (running) + width-20 false-negative autopsy (next).

## b50 — SUPERSEDED 46-digit candidate (kept for the record)

certauto (planner-selected full-modulus on the forced 47-digit subset, then
refute-and-descend) completed in 143.2s and direct-verified, at |D|=46:

    a(50) ?= 1420493599187205702699457094700124947938358490376675977858654611838391983888320

Self-verified: 46 distinct nonzero base-50 digits, missing {10,14,25},
lcm = 619808900849199341280 (= the 47-subset's L; 10/14/24 are lcm-redundant),
N ≡ 0 mod lcm, digit-sum ≡ 0 mod 49, MSB prefix 49,48,...,42 descending.
Planner note: chose family=full-modulus over the 11,552-suffix peeled plan on
the 47-digit subset exactly as doc #23 predicted (suffix-DP self-test = 11,552 ✓).

**WHY THIS IS NOT CERTIFIED (audit 2026-07-23, this box):** the driver's
"CERTIFICATION PASS" line overclaims for refute-and-descend runs. The
maximality claim requires the 47-digit subset {1..49}\{24,25} to have NO
completion, but the audit of the search code shows its refutation only covers
(a) a hard-fixed 21-position window (candidate at absolute position 20; doc
#23 §8's "widen and re-plan" is Phase C, NOT implemented), and (b) ONE
feasibility-adjusted prefix (buildFeasiblePrefix commits to the first
feasible release/promotion swap, maxReleases=3, and never revisits
alternatives after refutation). The divergence-depth law (m*=21 for b50)
says the window is probably wide enough — but the law is empirical ±2, and
b50's T=5 is the deepest nilpotent suffix ever seen, the exact profile of
the b40/b48 law-outlier cases. Also: this is the FIRST run ever to exercise
the refute-and-descend path (every sweep base had subsetsScanned=1).
Upgrade path: v4 independent scan arm (launched), and/or Phase C window
widening on the 47-digit subset.

## b52 — FIRST VALUE EVER: STRONG CANDIDATE pending confirm (2026-07-23)

The v15 ENGC_GUESS arm of the dual campaign emitted, after 7463.6s, labeled
CANDIDATE-UNPROVEN (prefix-guess W=23) — **NOT a certified solve**:

    a(52) ?= 448735208793063714451606009674691709006633117645639135533102744646118644150575200

Independent self-consistency verification (Python, this box): 47 digits in
base 52, all distinct and nonzero, digit set exactly the forced subset
{1..51}\{13,24,26,39}, divisible by digit-lcm 238388038788153592800, 52∤lcm,
digit-sum ≡ N (mod 51) holds, descending prefix 51,50,...,44. Status upgrades
to certified only if the v4 full-scan arm (still running) or an exhaustive
cert reproduces it.

**SWEEP COMPLETE (07:51 UTC): 9/9 bases solved and verified against known
answers. 8/9 bucket-certified by the autonomous driver (b42–b49, 47m 57s
combined bucket wall); 4 new wall-clock records (b43 831.5s, b44 120.4s
10.3×, b45 70.3s 4.9×, b47 190.5s); b41 = the one honest bucket memory
failure (Y=19P7=254M records), rescued by v4 scan fallback in 44.1s. Every
row labeled by the method that actually produced it. Admission-control
Phase A (Sol doc #22) landed mid-sweep: b41-class configs now decline in
0.18s/4.5MB instead of ballooning.**
