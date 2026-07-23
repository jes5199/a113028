# Band-Depth Certification — Sol (codex) design plan

**Status:** Design plan (Sol read-only exploration + codex reasoning). Mine-don't-trust; implementing engineer validates + refines. Correctness paramount (unpublished first-ever-value territory — an unsound certification is worse than an honest WEAK label).

**Problem:** Bases 54, 59, 61, 62, 64 yield only WEAK (window-bounded) lower bounds; base 63 yields NO-VALUE. The refute-and-descend search at fixed CERTPOS window (21 positions) cannot prove maximality when the divergence-depth m* (number of positions where the answer deviates from descending-digit order) exceeds the window. Brute window-widening is prohibitive: each +1 CERTPOS costs ~9× (exponential in the per-position search band).

**Three candidate approaches were flagged (evaluate, rank, combine):**
- **(a) FUSED suffix records** — cut per-width constant factor by amortizing suffix-index rebuild
- **(b) ANALYTIC BOUND on m*** — prove a rigorous upper bound on divergence depth, convert "unknown depth" into a terminating certificate
- **(c) PLANNER-CALIBRATION** — make wide-window planning affordable via tuned cost models

**Date:** 2026-07-23

---

## 1. Problem Restatement + Code Architecture

### 1.1 Certification Gate

`runCert` / `runCertFM` / `runCertAuto` all implement the same exit discipline: (L4636 `runCert`) if `subsetsScanned > 1`, the first-found success is labeled "STRONG CANDIDATE" (exit 4) not "CERTIFIED", because earlier subsets were refuted only within the fixed CANDIDATE_POS window. A true CERTIFICATION requires ALL earlier subsets to be exhaustively proven infeasible (exit 0).

**The bottleneck:** Once `subsetsScanned > 1`, refute-and-descend lacks a mechanism to certify earlier branches at their true maximal divergence depth. Every WEAK base (b54, b59, b61, b62, b64) exhibits this: the fast pass discovers valid (but unproven-maximal) completions; if you widen the window, some earlier subset often yields a strictly larger value, revealing the first run was submaximal.

**Evidence:** Frontier-status shows this pattern three times: b50 (corrected by window escalation), b56 (W=20 weak, W=21 found strictly larger drop-7 completion), b58 (same). b54, b59 remain open at W=21; b63 found nothing even in 90 minutes at W=21.

### 1.2 The Search Architecture

**Peeled path** (`runWrongTurnSearch`, L3217):
1. Enumerate admissible suffix tuples (T-digit combinations ≡ 0 mod L_nil), L3235
2. For each suffix tuple, build a suffix-aware bucket index, L3247
3. For each admissible x prefix, scan the index to find (x, suffix) → candidates at leaf level, L3250
4. Direct-verify each survivor, L3146
5. Declare candidate refuted if totalSurvivors == 0, L3289

**Full-modulus path** (`runWrongTurnSearchFM`, L3317):
- Bypass suffix enumeration; directly join x-permutations against one full-modulus residue space
- Same refutation at L3412: refuted iff totalSurvivors == 0

**Subset filtering** (before search, L3903 `SB_subset_filters`):
- Per-prime-power feasibility DPs (order-2 feasibility at L3822, order-e at L3855)
- These filter subsets, NOT candidates or wrong-turn branches
- Current implementation tests prime powers mostly separately, missing CRT coupling

**Key constraint:** The candidate position is FIXED at `certPos()` (L2481, default 20, now 21 for b60+). This bounds the refutation search to a 21-position window below the prefix. When m* > 21, refutation is incomplete.

### 1.3 Divergence-Depth Law (from ALGORITHMS.md)

For a subset D with lcm L, let L_eff = L / (order-1 part of L). The order-1 part is the product of prime powers q | gcd-structure where B ≡ 1 mod q — these only constrain digit-sum (permutation-invariant).

**Prediction:** m* ≈ min{m : m! ≥ L_eff}. Tested on all published b-file bases: |m_actual − m_pred| ≤ 2, except b46 (confirmed published value suboptimal, ours corrected).

For deep-structure bases (B−1 prime or prime-heavy), m* creeps large: b54 (B−1=53 prime) reaches m*>21 in its winning subset; b63 (B−1=62=2·31) likely m*>24.

---

## 2. Ranked Strategy (No Half-Measures)

### 2.1 Direction (b): Analytic Bound on Divergence Depth — CONDITIONAL PURSUIT

**Rationale:** The ONLY direction that eliminates the 9× ladder rather than speeding it up. But the bar for "certification" is strict.

**Current gaps in the literature:**
- ALGORITHMS.md (§4) already outlines EGZ/Cauchy–Davenport-style coverage ("for m ≥ f(q), permutation sums mod q with ≥2 distinct weights cover all residues") but explicitly notes: this only sharpens "above-band" boundaries and does NOT eliminate the critical band decision (the band width is intrinsically hard, likely NP-hard by max-flow duality / 3-partition reduction).
- The external review (EXTERNAL-REVIEW-2026-07-22.md, §3) confirms: "coverage theorems do not dissolve the computational frontier; the expensive branch is an infeasible child at the band edge."

**Therefore: treat (b) as a "SPIKE" with strict go/no-go criteria.**

**Go criteria (any of these):**
1. Prove a rigorous theorem of the form: "for this D and L_eff, no completion can diverge before position W" (e.g., by CRT Chinese Remainder Theorem tightness, or explicit modular structure).
2. Compute a proven upper bound on m* such that W_safe = ceiling(m* + 2) ≤ CERTPOS + 1 (within reach of current engine).
3. Bound the total refutation volume to a provable polynomial in B and k.

**No-go criteria:**
- Coverage theorems that only say "large enough permutations hit all residues" without bounding band width
- Heuristic "m* prediction is usually good" without a theorem
- Complexity estimates that still depend on unknown m*

**Spike implementation (if pursuing):**
- Add module `BandDepthAnalysis` computing per-subset: L, L_eff, ord_q(B) for all q | gcd-struct, factorization of cyclotomic layers, predicted m*, any theorem-certified W_safe.
- Mark all results `advisory` until a theorem is written.
- Promote to `certifying` only with: (i) formal theorem statement; (ii) executable checker logging proof hypotheses; (iii) regression test on b50/51/52/56/58/60 showing it does NOT falsely extend certification.
- Never suppress a search based on advisory bounds; only use them to guide window width.

**Honest assessment:** This spike is likely to produce helpful heuristic bounds (useful for planning, faster restarts) but NOT a theorem that closes band certification. If so, abandon in favor of §2.2/§2.3.

### 2.2 Direction (a): Incremental In-Band DPs — PRACTICAL FIRST LEVER

**Scope:** Current subset filters (L3913–L3927) test order-e moduli mostly independently. Compute **grouped low-order cyclotomic-layer DPs** that capture CRT coupling, reducing pruning volume by orders of magnitude.

**Two deployment sites:**

**Site 1: Subset-level filtering** (L3903 `SB_subset_filters`).
- Combine small-e prime powers into joint moduli Q_e = product(q_{p,e}) for e = 2, 3, 4, 5, 6.
- Compute digit-partition DP once per subset, not per-prime-power.
- Replace or augment `SB_e2_check()` / `SB_order_e_check()` calls; preserve order-1 modular sum.
- Result: faster subset rejection, fewer subsets reach wrong-turn search.

**Site 2: Candidate-window prechecks** (before L3247 `buildSuffixBranchGen` or L3367 full-modulus build).
- Before building indices, test whether a candidate could possibly have survivors via grouped cyclotomic DPs.
- Fail fast on impossible candidates; saves expensive index construction.
- Sound: only prune branches that violate proven necessary conditions.

**Correctness:** DP computation is exact (existing `countAdmissibleSuffixTuplesDP` pattern, L2576). Pruning is sound (a refuted necessary condition is a refuted branch).

**Estimated benefit:** 10–100× speedup on structured bases (e.g., b54 with B−1 prime → very few families survive; each family filtered by grouped DPs multiple times).

**Implementation complexity:** Medium. Reuse existing DP machinery; add a layer of modular-product composition. No new architecture.

### 2.3 Direction (c): Planner Calibration — LOWEST RISK, HIGHEST ROI

**Status quo:** The planner at `planBucket()` (L3527) explicitly states it is "uncalibrated" and yields predictions like `predLookups`, `predScans` that diverge from actual values by 2–10×. This directly impacts budget decisions in `runCertAuto`.

**Mechanism:**
- Current: heuristic hard-coded constants (e.g., "aim for B^NY in the low millions") drive NX/NY/K selection.
- Actual search characteristics vary by base, family, subset structure.
- Wider windows fail prematurely because planner over-estimates feasibility.

**Calibration path:**
1. Add structured telemetry around L4596 (where `certauto` already prints predicted vs actual).
2. Capture rows: (B, |D|, family, NX, NY, K, suffixMult, predLookups, actualLookups, predScans, actualScans, wall).
3. Offline regression fit: predict wall-time from (family, structure-signature, budget). Derive tuned constants `PLAN_C_BUILD`, `PLAN_C_SCAN`, etc.
4. Check tuned constants back into the repo; keep legacy safety margins (L3691) until b50/51/52/56/58/60 pass.
5. For future bases, use calibrated defaults; reserve env overrides for experimental runs.

**Why this matters:** Wider windows (CERTPOS 22–24) then become affordable because the planner no longer picks pathological (NX/NY/K) tuples. For b54/b59/b61/b62, +1 CERTPOS would cost 9× with current heuristic, but 3–5× with calibrated model.

**Estimated benefit:** 2–5× per planner tuning round. Multiplicative with (a).

**Risk:** Very low. Calibration only uses telemetry from successful runs; does not change algorithm. Regression gates ensure no silent correctness regression.

### 2.4 Direction (a): FUSED Suffix Records — DEFERRED

**Scope:** `runWrongTurnSearch` rebuilds a suffix-aware index for every admissible suffix tuple (L3241–L3247). With large suffixMult, this is expensive.

**Idea:** Fuse suffix and y into a single composite record, keyed by (suffix_id, y_residue). One build per candidate instead of per suffix.

**Why deferred:** High integration cost (T_target computation at L2681–L2688 folds suffix into target; fused records must carry per-suffix target shifts). High risk of mixing distinct completions. Only pursue after profiling shows:
- Peeled family selected often on weak bases
- Suffix rebuild is the dominant wall component
- suffixMult is large enough to amortize complexity

**Sound design (if implemented):** Records stored as (suffix_id, y_carrier, masks, target_shift). Final reconstruction through `verifySurvivorDirectGen()` (L3146) to ensure one-to-one mapping. Never deduplicate unless proven safe.

---

## 3. Correctness Argument

**Certification is sound only if every lex-larger alternative is either:**
1. Exhaustively searched by exact bucket engine and has zero survivors (definitive refutation)
2. Rejected by a proven necessary feasibility condition (sound pruning)
3. Covered by a rigorous theorem whose executable hypotheses are logged

**Key invariants to preserve:**

- **Resource decline ≠ refutation** (L2862–L2865). If the planner declines a bucket, exit 3 (resource limit), not 4 (refuted). `runCertAuto` enforces this at L4577–L4585. Do not weaken.

- **Direct verification gates** (L3146–L3165 `verifySurvivorDirectGen`). Every survivor must pass: range check (leaf < B^12), digit distinctness, digit set membership, divisibility by L. A survivor is reportable only when ar.success && ar.verifiedOk >= 1 && ar.verifiedBad == 0 (L4616). No survivor can be promoted past direct verification.

- **Sound pruning only** (new cyclotomic DPs). Never prune a branch unless the pruning condition is a proven necessary feasibility gate. Log every pruning decision to stderr (DEBUG mode) for audit.

- **Window-bounded refutations labeled as such** (L4636). If subsetsScanned > 1 and search completes, exit 4 (STRONG CANDIDATE), never exit 0 (CERTIFIED). Keep this distinction forever.

---

## 4. Validation Gates

### 4.1 Regression: Known-Good Certified Bases

These must remain CERTIFIED (exact value, maximality proven):
- **b50:** drop {24,25}, |D|=47, value starts 7102467995... (CERTIFIED ×3 engines)
- **b51:** drop {17,24,34}, |D|=47, value starts 1801459587... (CERTIFIED ×2 methods)
- **b52:** drop {13,24,26,39}, |D|=47, value starts 4487352087... (CERTIFIED ×2 methods)
- **b56:** drop {8,16,24,32,40,48,52}, |D|=48, value starts 8182417951... (CERTIFIED W=21)
- **b58:** drop {28,29}, |D|=55, value starts 9736569208... (CERTIFIED W=21)
- **b60:** drop {5,10,15,20,25,30,35,40,45,50,55,24}, |D|=48, value starts 3740967955... (CERTIFIED post-churn-fix)

**Regression policy:**
- Any change to pruning, DPs, or candidate enumeration must reproduce all 6 bases as CERTIFIED (exit 0) with byte-identical maxDecimal.
- Wall-time may vary ±20% (caches, system load); any slower than current baseline × 2 is a sign of algorithmic regression.
- DEBUG_SUBSETDISC logging must show byte-identical subsetsScanned / subsetsChecked counts.

### 4.2 Weak-Base Targets (First Certification Attempts)

**b54:** B−1=53 prime, currently WEAK at W=21 with 10 window-bounded refutations. First hard base; if (a)+(c) unlock this, it validates the strategy.
- Target: reach CERTIFIED or STRONG with all earlier subsets exhaustively refuted.

**b59:** B−1=58=2·29, currently WEAK at W=21 with different subset surviving at W=21 than W=20. Subtle non-monotonicity; stress test for correctness.
- Target: stable certification regardless of CERTPOS within reason.

**b63:** NO-VALUE; refute-descend never completed in 90 min at W=21. Stress test for "can we find a value at all?"
- Target: find any direct-verified completion and prove maximality.

### 4.3 Shadow-Mode Testing

Before promoting any analytic bound or pruning change to production:
1. Run on b50/51/52/56/58/60 with shadow flags (e.g., `SHADOW_ANALYTIC_BOUND=1`).
2. Log every pruning/bound decision to stderr.
3. Verify: no base is newly "certified" that was WEAK before; no CERTIFIED base is newly refuted.
4. Only after clean shadow run, flip feature flag to active.

---

## 5. Risks + Mitigation

### 5.1 Unsound Certification (HIGHEST SEVERITY)

**Risk:** A pruning rule incorrectly rejects a branch containing the true maximum. Or, a bound on m* is too tight, suppressing necessary search depth. Result: silent certification of a submaximal value.

**Mitigation:**
- All new pruning rules must be proven necessary conditions (DP or modular arithmetic, not heuristic).
- Analytic bounds must be published theorems with formalized hypotheses + executable checker.
- Shadow-mode testing on all 6 known-good bases before deployment.
- Maintain WEAK/STRONG/CERTIFIED distinction forever; never collapse exit 4 → exit 0.

### 5.2 Analytic-Bound Spike Produces Only Heuristic Bounds

**Risk:** After weeks of effort, the spike proves "large permutations cover residues" (above-band only), which docs already identify as insufficient. Band certification remains unsolved; 9× ladder still required.

**Mitigation:**
- Set clear go/no-go criteria upfront (§2.1).
- Spike is time-boxed (~1 week); if no theorem emerges by then, pivot to (a)+(c).
- The practical path (a)+(c) alone may achieve 50–100× speedup, enough for b54/b59 at W=22/W=23 without a proof.

### 5.3 Planner Calibration Leads to Pathological Choices

**Risk:** Tuned constants make the planner overly aggressive; wider windows with bad (NX, NY, K) tuples that explore the wrong part of the search space.

**Mitigation:**
- Calibration uses data from successful runs only; backtest on b50/51/52 first.
- Keep safety margin (1.5× predicted budget) in force until multiple bases pass.
- Telemetry rows include wall-time; any tuned constant that causes >2× slowdown is flagged.

### 5.4 Fused Suffix Fusion Incorrectly Deduplicates Distinct Completions

**Risk:** Merging suffix-id and y-record breaks the one-to-one correspondence between search branches and distinct valid numbers. Silent certification of a merged aggregate.

**Mitigation:**
- Defer (a) until profiling demands it.
- If implemented, reconstruct the full completion through `verifySurvivorDirectGen()` (L3146) — never trust the fused record alone.
- Unit-test on b50 candidate-21 (known to have multiple survivors) that every survivor is recovered distinctly.

### 5.5 Regression in b50/b56/b58 Certification

**Risk:** A change to DP state management or pruning order silently breaks the three multi-engine concordances.

**Mitigation:**
- Regression gates run on all 6 known-good bases before every merge.
- Use exact comparison (subsetsScanned, subsetsChecked, maxDecimal, wall-time ±20%).
- If any base regresses, revert immediately; post-mortem before re-landing.

---

## 6. Implementation Order

### Phase 1: Instrumentation + Baseline (Week 1)

**Goal:** Establish telemetry and regression gates.

1. Add structured planner telemetry around L4596 (already prints pred/actual; capture to CSV).
2. Implement regression test harness: run b50/51/52/56/58/60 in certification mode, capture stdout/stderr, diff subsetsScanned/subsetsChecked/maxDecimal/wall-time.
3. Confirm all 6 bases pass regression with current code (baseline).

### Phase 2: Grouped Cyclotomic DPs (Week 2)

**Goal:** Implement (a): direction.

1. Implement cyclotomic-layer DP (e.g., Q_e2 = product(q with ord_q(B)=2)) computation as subroutine.
2. Add optional shadowing at subset-filter site (L3913–L3927) with DEBUG flag; do NOT change default yet.
3. Run shadow mode on b50/51/52/56/58/60; verify pruning decisions are logged, no false certifications.
4. Once confident, replace `SB_e2_check` / `SB_order_e_check` calls with grouped DP.
5. Add candidate-window prechecks (before index build) using grouped DPs.
6. Regression test again (should be same or faster).

### Phase 3: Planner Calibration (Week 2–3)

**Goal:** Implement (c) direction.

1. Collect telemetry from b50/51/52/56/58/60 in both fast and wide-window modes (if time permits).
2. Add b54/b59 partial runs (e.g., first 3 subsets at W=21/W=22) to calibration dataset.
3. Offline regression fit: model wall-time as f(family, NX, NY, K, suffixMult, B).
4. Implement tuned cost model; check back to repo with safety margins (1.5× predicted).
5. Re-test b50/51/52/56/58/60 with tuned model; should be ±5% wall-time.

### Phase 4: Analytic-Bound Spike (Week 3, time-boxed)

**Goal:** Evaluate (b) direction under strict go/no-go criteria.

1. Study cyclotomic factorizations of L_eff for b54/b59/b61/b62/b63/b64.
2. Attempt to bound m* rigorously for at least one WEAK base.
3. If a theorem emerges, implement BandDepthAnalysis module, test on known-good bases in shadow mode.
4. If no theorem after 5 days, deprioritize and proceed to Phase 5.

### Phase 5: Certification Runs on Weak Bases (Week 4+)

**Goal:** Attempt to certify b54, b59, b61, b62, b63, b64.

1. Run b54 with tuned planner at W=21 (with grouped DPs, expect <2× wall-time compared to current W=21).
2. If still window-bounded (exit 4), widen to W=22; expected cost ~3–5× (with tuned planner), not 9×.
3. If W=22 certifies, move to b59/b61/b62.
4. b63 is the stress test; if grouped DPs + tuned planner fails, implement fused-suffix optimization (Phase 6).

### Phase 6: Fused-Suffix Optimization (If Needed, Week 5+)

**Goal:** Implement (a) direction part 2.

1. Profile b54/b59 runs from Phase 5; measure suffix-rebuild time as fraction of wall-time.
2. If >30% of wall time is suffix rebuild AND suffixMult is large, proceed.
3. Design fused suffix records (suffix_id, y_carrier, masks, target_shift).
4. Implement one-to-one reconstruction through verifySurvivorDirectGen().
5. Unit-test on b50 candidate-21 (multiple survivors).
6. Re-run b54/b59 with fusion; target 2–5× speedup.

---

## 7. Expected Outcomes (Realistic Estimates)

With (a) + (c) combined (phases 1–3):
- **b54/b59:** Expected to reach CERTIFIED or STRONG with 3–5× wall-time increase per +1 CERTPOS, instead of 9×. Likely certifiable at W=22 (a few hours per base).
- **b61/b62:** Similar, may require W=23.
- **b63:** Likely still requires W=24 or fused-suffix optimization to find value at all.
- **Known-good regression:** b50/51/52/56/58/60 remain CERTIFIED, wall-time ±20%.

If analytic-bound spike succeeds (§2.1 go criteria):
- May reduce required W for b54/b59 to W=21, eliminating wide-ladder entirely for some bases.
- If spike fails, no loss; proceed with (a)+(c) above.

---

## 8. Key Files to Review / Modify

**Exploration (read-only for this plan):**
- `carrytrie.cpp` L2481 (certPos), L3217 (runWrongTurnSearch), L3317 (runWrongTurnSearchFM), L3903 (SB_subset_filters), L4636 (certification gate)
- `ALGORITHMS.md` (divergence-depth law, band-irreducibility)
- `FRONTIER-STATUS.md` (weak bases, calibration data)
- `EXTERNAL-REVIEW-2026-07-22.md` (band NP-hardness intuition, coverage limits)

**Implementation targets:**
- `carrytrie.cpp` L3913–L3927 (order-e checks → grouped DPs)
- `carrytrie.cpp` L3247, L3367 (candidate prechecks)
- `carrytrie.cpp` L4596 (planner telemetry expansion)
- New module `BandDepthAnalysis` (if spike pursued)
- New regression test suite (Phase 1)

---

## 9. Deliverables (What the Engineer Gets)

1. **Planner telemetry framework** (CSV rows: B, |D|, family, (NX, NY, K), predicted vs actual).
2. **Grouped cyclotomic-DP implementation** (proven necessary conditions, sound pruning).
3. **Planner calibration constants** (tuned cost model checked back to repo).
4. **Regression test harness** (runs b50/51/52/56/58/60, compares subsetsScanned / maxDecimal).
5. **Band-depth spike results** (if theorem emerges, formalized + executable checker; if not, clear documentation why).
6. **Certification campaign on b54/b59/b61/b62/b63/b64** (results logged, correctness gates enforced).

---

**Bottom line:** This plan prioritizes *sound certification* over brute-force search depth. (a) + (c) together offer a realistic 10–50× speedup via proven pruning + tuned planning, enough to certify b54/b59 at affordable widths. (b) remains a speculative win; pursue under strict criteria only. Maintain WEAK/STRONG/CERTIFIED distinction forever; any unsound certification is unacceptable.


---

## Review note (Fable, 2026-07-23, pre-implementation vet)

Overall: sound discipline, right priorities, correct invariants (§3). Three
substantive corrections required before build, plus minor flags.

**C1 — the §4.1 regression gate contradicts Phase 2's purpose.** "Byte-identical
subsetsScanned/subsetsChecked" cannot survive stronger subset filters: grouped
DPs are SUPPOSED to reject subsets that previously reached (and failed) full
scans, so subsetsScanned legitimately DECREASES. As written, the gate passes
only if the new filters are no-ops. Corrected gate: byte-identical maxDecimal +
exit code + WINNING-subset identity; subsetsScanned may only decrease; every
newly-filter-rejected subset logged with its refuting modulus; and any
filter-rejection of a subset that previously PRODUCED a winner = loud abort.

**C2 — grouped-DP soundness specifics the doc omits:**
(i) only combine moduli of EXACTLY equal order e (joint weight pattern must
have period e; mixing orders needs period lcm and misaligned classes silently
prune feasible subsets = unsound);
(ii) cap the joint modulus size (Q_e products blow up; splitting a group is
sound — weaker but still necessary);
(iii) derive every Q_e programmatically from the subset's own lcm structure,
excluding p | B — the b39 incident (wrong Q=3120 vs correct 80, spectacular
false finding, 2026-07-22) is this exact bug class;
(iv) the Site-2 candidate precheck must use the correct modulus (L_c vs L) and
post-peeling position offsets — the historical silent-position-arithmetic bug
class that caused false refutations; treat this insertion point as the
highest-risk line of the whole plan.

**C3 — make shadow mode an active unsoundness oracle, not just a log:** run
prechecks in shadow WITH the search; if a precheck says "impossible" and the
search then finds a survivor, abort loudly (this is a proof of an unsound
condition). Same for C1's winner-rejection detector. Passive logging can miss
what an active contradiction check catches by construction.

Minor: §1.3's "b54 winning subset has m*>21" and "b63 m*>24" are conjectures,
not established (our data shows only window-bounded refutations of earlier
subsets) — keep advisory. §4.2's b59 target "stable certification regardless
of CERTPOS" overpromises given proven non-monotonicity; the achievable target
is certification at some width with window-COMPLETED refutations at that
width. §4.1's wall-time tolerances conflict (±20% vs ×2) — harmonize. Cited
line numbers pre-date the churn-fix commit — grep, don't trust. Priority
order (c)→(a)→(b-spike) endorsed; Phase 1 (telemetry + regression harness)
and (c) are safe to start immediately.
