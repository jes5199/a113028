# Frontier status: bases 50–64

**Date:** 2026-07-23 (updated live during the fast-mode climb)
**Context:** the published OEIS b-file for A113028 ends at base 48; a(49) was
jes's laptop computation. Everything from base 50 upward on this page is a
**first-ever computed value**, and none of it is published. The distinction
between PROVEN and CONJECTURED below is therefore the most important thing
in this file.

## Evidence classes (what each label means)

- **CERTIFIED** — an exhaustive search argument covers the whole space:
  the winning subset is the first-scanned (descending-lex maximal) subset,
  the fixed prefix is the naive descending arrangement, candidates at the
  divergence position were tried largest-first with each refutation and the
  find inside one exhaustively-searched window, and every survivor was
  directly verified mod L. Where noted, multiple independent engines
  concur. **This is a proof of maximality** (modulo engine correctness,
  which is defended by multi-engine concordance and known-base regressions).
- **STRONG (single-method exhaustive)** — same exhaustive structure, but
  only one engine family has produced it so far. Held short of certified
  per project policy after the b50 incident (below).
- **WEAK candidate** — the reported value is a *valid* completion
  (direct-verified: distinct nonzero digits, exact digit subset,
  divisible by the digit-lcm), **but its maximality is UNPROVEN**: the
  driver refuted one or more larger-subset or larger-candidate branches
  only within its fixed 21-position divergence window (CERTPOS=20). Such
  window-bounded refutations have been wrong before — see the b50 case
  below, where exactly this situation concealed a larger 47-digit answer.
  A WEAK value is a lower bound on a(B), nothing more.
- **NO-VALUE** — the fast pass produced nothing within its budget
  (timeout / admission decline); the base is open.

## The b50 cautionary tale (why WEAK ≠ answer)

The first b50 run refuted the forced 47-digit subset within its fixed
window and "found" a 46-digit completion. Window escalation (CERTPOS=21)
then discovered a **larger 47-digit completion** — and the autopsy showed
the original run had actually *found* that completion and lost it to a
record-packing bug (uint32 truncation at NY=6, fixed in commit "FIX FM
false negative"). a(50) was subsequently certified on three concordant
engines. Every WEAK label below carries exactly this risk profile:
**window-bounded refutations can hide larger answers.**

## The hardness onset: base 54

jes's question — "keep trying larger bases and see when/if it gets hard" —
has a sharp answer: **the onset is at b54, and the mode is window/band
depth** (not memory, not raw wall-clock).

Progression of the fast certauto pass (single core, nice-19):

| base | wall | anatomy |
|-----:|-----:|---------|
| 50 | 49s | candidate 20 wins, 1 wrong turn |
| 51 | 50s | candidate 21 wins, 1 wrong turn |
| 52 | 21s | candidate 21 wins, 1 wrong turn |
| 53 | 279s | candidate **8** wins, **13** wrong turns — the strain begins |
| 54 | 725s | **10 subsets refuted (window-bounded)**, candidate **1** wins, 20 wrong turns |

Mechanism: 54−1 = 53 is prime, so the only arrangement-independent subset
filter is the mod-53 digit-sum rule — weak filtering lets many subsets
survive to full scans, and the divergence band is deep (the winning
candidate at the bottom of the pool means the answer deviates from plain
descending order almost maximally within the window). Larger bases whose
B−1 is prime or nearly-prime should behave like b54; smooth B−1 like b52
(=51=3·17) stays easy longer.

## Status table (updated as the climb lands)

| base | value | status | fast wall | mode notes |
|-----:|-------|--------|----------:|------------|
| 50 | 71024679959360285134972854735006247396917902186058529912810323948019000719382560 | **CERTIFIED ×3 engines** | 49s | full-modulus + peeled-width-21 + v4 scan concordant |
| 51 | 180145958793036691752603389680297418249529280174180771266050386968626019795153600 | **CERTIFIED ×2 methods** | 50s | certauto + v4 scan complete |
| 52 | 448735208793063714451606009674691709006633117645639135533102744646118644150575200 | **CERTIFIED ×2 methods** | 21s | certauto + v15 candidate concordant |
| 53 | 8667796530759171030732652761285454124185037606953473954566810515107048776076588809114400 | **STRONG** (single exhaustive; v15 2nd method in flight) | 279s | prime base, drop {26} |
| 54 | 416421702506789485219242774659857217353557918404448765237845114648269563412650818364000 | **WEAK candidate — needs >+1** | 725s | HARDNESS ONSET; 10 window-bounded refutations. ⚠️ The W=21 probe stayed window-bounded AND found a *smaller* completion than the fast pass (non-monotone across widths — different subsets survive prefix-feasibility at different widths). The fast-pass value stands as the best-known lower bound. Deep-ladder base. |
| 55 | 18950593883712953094852355579302043793829907380935165052003989360731470319629320169600 | **STRONG** (single exhaustive) | 7.1s | smooth B−1=54=2·3³ → strong filtering (drop 5: {11,22,25,33,44}); prediction confirmed |
| 56 | 818241795167031571216125942108524047248252714836991418093245122512858064867506776900 | **CERTIFIED (clean at W=21)** — SUPERSEDED the fast-pass weak value | 2132s fast + 677s @W21 | +1 probe found a strictly larger \|D\|=48 completion (drop 7: {8,16,24,32,40,48,52}) with clean exit-0 — the fast pass's drop-8 weak value was submaximal, b50-style |
| 57 | 1151406937346438856563169238699639272012360683309171137238477150432093884638244390036253366400 | **STRONG** (single exhaustive) | 46s | B−1=56=2³·7; \|D\|=53 drop {19,27,38}, candidate 15, 6 wrong turns |
| 58 | 9736569208044009047215982390641931224449891752961186859894784030413187509184301962164357195502400 | **CERTIFIED (clean at W=21)** — SUPERSEDED the fast-pass weak value | 1588s fast + 2394s @W21 | +1 probe found a strictly larger \|D\|=55 completion (drop {28,29}) with clean exit-0 — third instance of weak=submaximal |
| 59 | 1470901301179362895265301083750020315277766231253346714280007293140176130090413579681261985958564000 | **WEAK candidate — superseded at W=21, still window-bounded, needs >+1** | 1372s fast + 927s @W21 | B−1=58=2·29. The +1 probe found a strictly larger \|D\|=56 completion (drop {14,15} vs fast's {43,44} — different subset!) but 1 earlier subset stayed window-bounded. Fourth weak=understated instance. Deep-ladder base alongside b54. |
| 60 | 374096795553866901593729675694543662806191148614834420420657978999648347496776816352 | **CERTIFIED (clean @W21, post-churn-fix engine)** | 13.9s (was NO-VALUE at >5400s) | The discovery-churn thesis fully vindicated: with the feasible-subset enumerator (Sol doc #24, corrected k-major merge, all order/value gates byte-identical), discovery is instant and the base certifies in 14 seconds — a 390× turnaround. Dropped digits = exactly the 11 multiples of 5 (kills the ten-rule's 5-factor) + 24 (digit-sum ≡ 0 mod 59): the mechanism's prediction verbatim. Original NO-VALUE stands in the verdicts log as the pre-fix data point. |
| 61 | 35400531009451661938977999331213310176526542300021623090439321414194700171315894915223503137438256132800 | **WEAK candidate @W21 — needs >W21** | 1504s | **Churn prediction CONFIRMED**: discovery instant (61 prime, planner traces at log top); all cost was search-side band depth (refute-descend). Refinement: smooth B−1 ≠ automatically easy — Pc=14 keeps the band deep. \|D\|=58 drop {43,47}; 104-digit value. Deep-ladder queue. |
| 62 | 90907435844214195686349330991958807391545410997397217149380812995505328431077789672450548846308497831600 | **WEAK candidate @W21 — needs >W21** | 4122s | B−1=61 prime → band-deep as predicted; refute-descend to \|D\|=58 (drop {31,32,59} — 31 kills the ten-rule's 31-factor), candidate 11, 11 wrong turns. Deep-ladder queue. |
| 63 | *(none — no value known)* | **NO-VALUE @W21 — mode = BAND DEPTH (genuine)** | >5400s | Unlike b60's churn timeout, discovery worked (plans + subset scans throughout the log, ~40s each): the refute-descend ladder simply never reached a completing subset in 90 min. First base with no lower bound at all from pure search depth. Prime-heavy structure (B−1=62=2·31). Top of the deep-ladder queue. |
| 64 | 2347950861286613819164892670209135837212469936650502927594341338211603354685587057891666543095590619828487200 | **WEAK candidate @W21 — needs >W21** | 3139s | Mask ceiling reached. \|D\|=60 drop {37,43,46}; 109-digit value — the campaign's largest. Discovery instant (2⁶ needs no drops, as predicted) but refute-descend on the band axis. Deep-ladder queue. |

**CLIMB COMPLETE (21:11 UTC 2026-07-23), b53→b64, the engine's digit-mask
ceiling.** Final tallies: 2 clean-certified on the climb engine (b56, b58 via
+1 probes; plus b60 post-churn-fix), 3 STRONG single-exhaustive (b53, b55,
b57), 5 WEAK lower bounds needing the deep ladder (b54, b59, b61, b62, b64),
1 genuine no-value (b63, band depth). Second-method arms: a(53) and a(56)
v15 arms both capped with no candidate — their single-method statuses stand
honestly. The deep-ladder / band-depth class (b54, b59, b61, b62, b63, b64)
is the campaign's open problem: discovery is instant everywhere post-churn-fix,
but divergence deeper than the affordable window defeats certification at
~9×/width brute cost — an algorithmic gap, not a compute gap.

## Structural finding: the W=20 window undershoots past the onset

Every rough-B−1 base past b54 that the W=20 fast pass labeled WEAK turned
out, under its +1 probe, to be understating the answer: b56 and b58 both
yielded **strictly larger certified values at W=21** (drop-8→drop-7 and
drop-3→drop-2 respectively), and b54's probe exposed **non-monotonicity**
(different subsets survive prefix-feasibility at different widths, so
refute-descend outcomes aren't even comparable across widths). Conclusion
adopted 2026-07-23 ~15:45: **W=21 is the fast-pass width from b60 up**
(climb3.sh); the W=20 attempts of 60–64 were abandoned mid-b60. The W=20
data through b59 stands as the uniform onset-discovery series.

## Planner calibration (band-depth plan Phase 1+3, landed 2026-07-24)

Telemetry (planner_telemetry.csv, always-on) + a 6-base regression harness
(regression_suite.sh: b50/51/52 default, b56/58 W=21, b60 W=21+new-enum;
pass = char-exact value + exit 0) are now production. Calibration outcome,
honestly told: the full per-K median fit (K=2: 115,000× underestimate!,
K=3: 21×, K=4: 14×, from 402 pairs) passed soundness (6/6 char-exact) but
FAILED the performance gate — small-sample cross-K ratios invert plan
rankings (b60 flipped to a 19×-slower K=4 plan). **Lesson: calibration must
model plan RANKING, not per-K scaling.** The shipped revision keeps only
the unambiguous K=2 disqualifier + legacy K=3/K=4 constants. Result:
postcal2 6/6 char-exact, b52/b60 at baseline walls, and **b58 8× faster
(1921s → 235.9s)** — its certified W=21 run had itself been on the
pathological K=2 plan; the disqualifier re-planned it to full-modulus
NX=3/NY=5/K=3, same value char-exact. The K=2 misplan class is closed.

## The mask range 65–89 and the outer proof engine (2026-07-24)

The u128 mask widening is live (validated byte-identical + exact counts on
all six known bases; ~3× constant-factor cost where subset churn is hot —
b51 46→156s — is the known price). The checked-lcm guard pins the exact
arithmetic ceiling: **bases ≤ 89 attemptable, ≥ 90 refuse cleanly**
(lcm(1..89) overflows u128 at d=89). Fast-pass stepping stones all came
back honest NO-VALUE at 90-min caps (b65, b73, b81 — band cost at the
auto-scaled windows W=23–25, machinery sane throughout): the mask range
will be won by the proof engine, not fast passes.

**The outer lexicographic branch-and-bound (HIGHER-BASE-CERTIFICATION-
STRATEGY.md) is now validated in practice**: it reproduced and PROVED
a(56) (314s: 1 found / 25 refuted terminals / 25 pruned / 0 unfinished) and
a(60) (30.7s: 1 found / 24 pruned) char-exact — complementary disposition
mixes, valid aggregates, zero declined-refuted conflation. **b56 and b60
are hereby 2-method certified** (original engine + outer proof). b58's
outer proof is performance-bound (90-min cap insufficient; 6h rerun in
flight). Window width is now provably a performance knob, not a soundness
boundary — the fixed-window era's WEAK labels are attackable branch by
branch.

## Honest negative: grouped cyclotomic DPs (band-depth plan Phase 2, 2026-07-24)

Built per the plan's §2.2 with the review-note corrections (equal-order-only
grouping, Q_e ≤ 2^24 with sound splitting, per-subset derivation excluding
p|B, and a permanent contradiction oracle that aborts if a precheck ever
disagrees with the exact search). Verdict from 14 gated runs: **sound but
valueless** — zero contradiction firings ever (soundness ✓), and zero
subsets filtered / zero candidates pruned on all 6 known bases AND both
deep probes (b54: still no verdict at a 40-min cap; b61: identical value,
identical exit-4, wall within noise of baseline). The existing order-1/2
filters plus the divergence structure already exhaust what these
necessary conditions can see on real instances. Disposition: module stays
compiled with its oracle, **default GROUPDP=off**; joins the honest-negative
ledger (with the MITM-α and cheap-conditions arcs). The band-depth open
problem stands unmoved; priority passes to the 128-bit-mask extension
(bases > 64) per jes's directive.

## Deferred engine fixes (distinct remedies for distinct modes)

- **Band/window depth (b54, b59):** the CERTPOS widen ladder (autowiden.sh),
  ~9× per width step. This is the only mode the ladder addresses.
- **Discovery churn (b60):** a *different* fix — generate only
  ten-rule-feasible subsets (enumerate which prime power to eliminate, then
  descend over the surviving alphabet) instead of filtering the full
  descending-k stream. Widening does nothing here; the time is lost before
  any search begins.
- **Memory (b41-class):** already fixed in production by the admission
  planner (decline-before-allocate).

## Deferred option: deep window escalation

The CERTPOS knob widens the divergence window (validated: width 21
reproduces a(49) and found the true a(50)). Cost grows roughly ×9 per
width step. Certifying b54+ properly means re-running the refuted subsets
at widths 22–24 (hours each) or engine work on the band-depth mechanism —
**deferred by jes's decision (2026-07-23) in favor of the fast-mode
climb**. The fast climb's WEAK values remain honest lower bounds until
that (or an independent method) runs.
