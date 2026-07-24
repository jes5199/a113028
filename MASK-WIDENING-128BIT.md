# 128-Bit Digit-Mask Widening — Sol Design Plan

**Status:** Design plan (Sol/codex read-only exploration + reasoning). **Boss-reviewed 2026-07-24 (passes: honestly scoped to 65–~88, P_full/CERTPOS audit present, critical overflow risks flagged, byte-identical gate required — no overpromise of 128, no soundness gap).** Mine-don't-trust; implementing engineer validates every touchpoint against actual code (line numbers may have drifted post-churn-fix/calibration), refines, and flags discrepancies. **Correctness paramount** — a subtly wrong result is worse than an honest "cannot reach base X yet."

**Epic:** First phase of jes's "push toward base 128" — widen the 64-bit digit mask to 128-bit to enable bases 65+. ⚠️ **The arithmetic ceiling is ~BASE 88** (where L = lcm(1..n) overflows u128), NOT 128; bases 89–128 require a **separate follow-on u256/arbitrary-precision arithmetic epic** (§11).

**Date:** 2026-07-24

---

## 1. Problem + Hard Constraints

### 1.1 Current ceiling
Each digit d ∈ {1..B−1} maps to bit d of a `uint64_t` mask (in-set iff `(mask >> d) & 1`). Works to base 64 (digits 1..63). Base 65 needs digit 64, unrepresentable in 64 bits.

**Mask touchpoints (all `uint64_t`) — verify line numbers against live code:**
- `BucketRecord::yMask` (~L1073), `BucketRecord48::yMask` (~L2075), `BucketRecordGen::yMask` (~L2713), `BucketRecordFM::yMask` (~L3051)
- Join-walker locals `xmask`/`yMask`/`lowLeafMask` in `bucketSearchX` (~L1184), `bucketSearchX48` (~L2202), `bucketSearchXGen` (~L2971), `bucketSearchXFM` (~L3112)
- `bit(d)` macro (~L501: `(uint64_t)1 << d`)
- Intersection hot loops (~L1207 `r.yMask & (xmask|lowLeafMask)`, L2227, L2993, L3134); validity `A19mask & ~xmask & ~r.yMask` (~L1214, L2234, L2999, L3140)
- `SuffixBranchGen` mask setup (~L3295 `restAvailMask |= bit(d)`)
- **`subsetdisc` SB_setmask (~L4472, L4549: `SB_setmask |= 1ULL << SB_S[i]`) — SILENT BUG RISK for digits ≥64 (see §8.1)**

### 1.2 Arithmetic ceiling ≈ base 88 (not 128)
The modular layer assumes L < 2^128. L = lcm(1..B−1) ≈ e^(B−1): e^64≈2^92, e^88≈2^127.1, **e^89≈2^128.8 overflows u128**. `mulmod_u128`/`powmod_u128` (~L107–135) break once L ≥ 2^128.
**Scope of THIS epic: bases 65–~88.** Name bases 89–128 as the out-of-scope follow-on arithmetic epic; do NOT claim this reaches 128.

### 1.3 CERTPOS/window scaling problem
P_full = ceil(log_B L); window invariant `P_full + NX + NY == CERTPOS` (default 20, hardcoded range [20,24] ~L2492). As B→88, P_full→~20 (log_88(2^127)≈19.7) → NX+NY→0 → both join families die. **The window must expand and positions must scale with B, or certification silently fails in the high-70s/80s.** §4 addresses this.

---

## 2. Mask type: `unsigned __int128` (chosen over 2×u64 struct)
**Rationale:** the intersection hot loops run millions of times/run — native __int128 AND/OR compiles to 1–2 instructions vs 4–8 for a struct; bitwise ops are bitwise-identical to u64 (no carry/edge risk); codebase already assumes GCC/Clang + __uint128_t elsewhere; record doubling is manageable (admission gate already parameterizes on record size). The `bit(d)`/`1ULL<<d` idiom must be promoted to `(unsigned __int128)1 << d` (well-defined for d∈[0,127] on __int128).

---

## 3. Touchpoint inventory (widen uint64_t→unsigned __int128)
- **Type decls:** the 4 `yMask` fields (§1.1). Leave `BucketRecordFM::yDigitsPacked` as uint64_t (not a digit-set mask). Record sizes grow ~+8B each (BucketRecordGen 24→32, FM 32→40) at 16-byte alignment.
- **`bit(d)` macro:** widen to handle d∈[0,127] (`(unsigned __int128)1 << d`). Compile with `-Wshift-count-overflow`.
- **Mask construction sites:** all `m |= bit(d)` loops (runMeasurement, BucketIndex builds, SuffixBranchGen) — unchanged once bit() is widened.
- **⚠️ `SB_setmask |= 1ULL << SB_S[i]` (~L4472/L4549):** MUST become `|= ((unsigned __int128)1 << SB_S[i])` — else UB/silent-truncation for digits ≥64.
- **decode functions** `decodeDistinctLeaf12/Gen/FM` (~L1156/L2182/L2954): return-mask type → unsigned __int128; internal `outMask |= bit(d)` unchanged.
- **Hot loops** (intersection/validity): no change (bitwise ops identical on __int128).

---

## 4. P_full / CERTPOS scaling audit
P_full grows ~11 (b50) → ~13 (b64) → ~14 (b73) → ~15 (b81) → ~16 (b88); free window (CERTPOS−P_full) shrinks 9→4.
**Scheme:** compute P_full at startup (deriveConstantsGen); set `CERTPOS = min(28, P_full + 8)`; validate in `[20, max(24, P_full+10)]`; keep ≥8 free positions (enough for 2–4 x + 2–4 y digits). Per-range defaults: ≤64→20, 65–79→22, 80–88→24. Log `[setup] base=B P_full=.. CERTPOS=..`. **Assert NX+NY≥2 before any join search; else exit(3) resource-limit (NOT a refutation).** Position-enumeration architecture is already flexible — no refactor if CERTPOS is set dynamically. (Note for vet: P_full via lcm(1..B−1) is an upper-ish estimate since the ten-rule drops digits — conservative for window sizing.)

---

## 5. Memory / record-size re-audit
Records +~8B each → ~33% larger bucket indices. Admission gate (`bucketAdmissionQuery` ~L2831) already takes `recBytes = sizeof(BucketRecordGen/FM)` — grows automatically, no gate rewrite. **Re-audit projections on bases 65/73/81/88 vs the RSS budget (default ~3GB ~L2888); if projected recordsBytes exceeds budget → refuse via exit(3) (safe), or raise budget to 4GB.** Bases ≤64 unchanged.

---

## 6. Correctness argument
- **Bases ≤64 unchanged:** no base ≤64 uses digits ≥64 → high 64 bits always 0 → all bitwise ops preserve identical behavior. The regression suite (b50/51/52/56/58/60 byte-identical) is the catch.
- **Bases 65–88 sound:** masks are used only for set membership/intersection/union/complement (all bitwise, L<2^128 irrelevant); masks are never passed to mulmod/powmod. Leaf decode = arithmetic digit-extract (sound for B<128) + bitwise allowed-test.
- **Ceiling detection (§8.2):** at startup, after computing L, assert L < 2^128; if L≥2^128 print "arithmetic ceiling exceeded; bases 89+ need u256 epic" + exit(1). Prevents silent mulmod overflow.

---

## 7. Validation gates
1. **HARD GATE (before ANY base ≥65):** `regression_suite.sh` on b50/51/52/56/58/60 → all exit-0, decimal values **byte-identical**, subsetsScanned/Checked within ±5%. Any regression → STOP + debug.
2. **Stepping stones (smooth B−1):** b65 (B−1=64=2^6), b73 (72=2^3·3^2), b81 (80=2^4·5) — first real digit-≥64 + CERTPOS-scaling tests; expect strong lower bounds within hours→overnight. Skip b88 until the smooth ones validate.
3. **Ceiling test:** b89 must fail cleanly (exit-1, clear message), no silent overflow.

---

## 8. Risks (highest first)
1. **CRITICAL — `1ULL << d` for d≥64 (UB/silent truncation):** audit ALL `1ULL <<`/`(1ULL<<q)`; promote to `(unsigned __int128)1 <<`; `assert(d<128)` in bit-construction; only enter subsetdisc if all digits <128. Test: D={1..64} → SB_setmask correct.
2. **CRITICAL — L overflow past 2^128:** `lcm_u128` (~L64) silently wraps if `(a/g)*b ≥ 2^128`. Startup guard: `if (L==0 || L >= ((u128)1<<127)) { fatal; exit(1); }`. Test: b89/b128 refuse cleanly.
3. **HIGH — CERTPOS starvation (b85–88):** if CERTPOS not scaled, P_full eats the window → families silently "refute". Mitigate via §4 dynamic scaling + NX+NY≥2 assert.
4. **MED — record-size memory pressure:** re-audit admission projections (§5); exit(3) if over budget.
5. **MED — planner miscalibration for high bases:** planner uncalibrated; use telemetry (~L4596) + tune constants from b50–60 before b65; flag if wall >10× predicted.

---

## 9. Implementation order (phased, regression-gated)
1. Type widening + `bit(d)` macro + audit all `1ULL<<` (compile `-Wshift-count-overflow`).
2. Decode function signatures → __int128 masks.
3. Bucket record structs → __int128 yMask; verify sizeof.
4. Hot-loop mask ops (no change; verify codegen).
5. Startup guards + P_full compute + dynamic CERTPOS + NX+NY≥2 assert.
6. Memory audit on 65/73/81.
7. **Regression gate: byte-identical b50–b60** (iterate 1–6 until clean).
8. Smooth bases 65 → 73 → 81 (validate digit-≥64 + consistency across runs).
9. Ceiling-detection test (b89 fails cleanly).
10. Doc + README update (new range 65–88; 89+ blocked on u256 epic) + commit.

## 10. Second-method arms
v4/v15 have `MAXB2 = 64` (~L3947). **Decision:** keep MAXB2=64 → certauto-only above 64 (reduces audit scope; revisit later), OR widen+audit their digit handling for >64. Recommend certauto-only initially. SUBSETENUM prime-family enumeration generalizes cleanly (more primes/B, still cheap).

## 11. Out-of-scope follow-on epic — Arbitrary-Precision Arithmetic (bases 89–128)
Extend mulmod/powmod/gcd/lcm to m∈[2^128, 2^256): u256 limb-arithmetic or an external bignum (GMP/Boost.Multiprecision) or cap at a finite frontier. ~2–3× this epic's effort. Do not attempt until 65–88 are explored. Prime-B−1 bases will be band-deep WEAK lower bounds from day one.

---
**Engineer:** validate touchpoints vs live code (line numbers drift), refine CERTPOS scaling from measured P_full, build incrementally, regression-gate after Phase 7, and flag any plan↔code discrepancy — do not assume correctness. Boss-verify checkpoint: any base ≤64 that changes value = STOP; any new 65+ value = boss verifies from scratch before it's called anything but a candidate.

---

## Review note (Fable, 2026-07-24, pre-implementation vet)

Sound, honestly scoped, right risk ranking. Corrections:

**C1 — CRITICAL, the §8.2 guard as written can pass a wrapped L.** A wrapped
lcm mod 2^128 can land ANYWHERE in [1, 2^127) — post-hoc inspection
(`L==0 || L>=2^127`) is not a reliable overflow test. The guard must use
overflow-CHECKED accumulation inside the lcm computation itself
(`if (a/g > U128_MAX / b) → overflow-detected`), fatal-exit at the point of
wrap. Test vector: lcm(1..89) must be DETECTED as overflow, never produce a
small bogus L that passes inspection. Guard scope: compute checked
lcm(1..B−1) once at startup — it upper-bounds every subset's L, so one
startup guard covers all subsets.

**C2 — the §7 gate's "subsetsScanned/Checked within ±5%" is too loose.** The
engine is deterministic and bases ≤64 must be BEHAVIORALLY UNTOUCHED: counts
must be exactly identical, values byte-identical; only wall-time gets
tolerance. A ±5% counts drift is a bug signal, not acceptable variance.

**C3 — widen the audit beyond `1ULL<<d`:** (i) any
`__builtin_popcountll`/popcount on masks (must become two-limb); (ii)
printf/log formats of masks (%llx silently truncates — audit even if only
logs); (iii) universe-mask constants (A19mask-style `~` complement partners)
must be built at 128-bit width or complements silently gain phantom high
bits... (verify each universe constant's construction); (iv) any
hash/comparison assuming 8-byte masks.

Minor: §3's record sizes are off — BucketRecordGen widened stays 32B (the
current 8B padding absorbs the growth: {u128,u128}), FM goes 32→48B (16-align)
not 40; harmless since the admission gate uses sizeof, but fix the doc's
memory-projection text. §4 specifies TWO CERTPOS mechanisms (dynamic formula
AND per-range table) — implement the formula, keep the table as
documentation only. §10's MAXB reference: the second-method caps live in
a113028_v4.c/v15.c (their own files), not carrytrie.cpp; certauto-only >64
endorsed. Cosmetic, non-engine: the README digit alphabet ends at ת=70 —
bases ≥72 need a rendering-spec extension before their values are displayed
(decimal remains canonical). Line numbers throughout: grep, don't trust
(the plan itself says so).
