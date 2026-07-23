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
| 60–64 | *(fast-mode climb in progress)* | | | |

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

## Deferred option: deep window escalation

The CERTPOS knob widens the divergence window (validated: width 21
reproduces a(49) and found the true a(50)). Cost grows roughly ×9 per
width step. Certifying b54+ properly means re-running the refuted subsets
at widths 22–24 (hours each) or engine work on the band-depth mechanism —
**deferred by jes's decision (2026-07-23) in favor of the fast-mode
climb**. The fast climb's WEAK values remain honest lower bounds until
that (or an independent method) runs.
