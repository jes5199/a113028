# Maximality arguments for the hard bases (54, 59, 61, 62, 63, 64)

These six bases are where maximality was expensive: each spent time as a
WEAK lower bound (or, for 63, a NO-VALUE) before its proof landed. This file
holds the complete arguments, moved verbatim from the README when it was cut
down to a results list (2026-07-26). Per-base status lives in
[FRONTIER-STATUS.md](FRONTIER-STATUS.md); this file is the *why* behind each
STRONG label.

**The maximality-proof engine** (2026-07-24,
[HIGHER-BASE-CERTIFICATION-STRATEGY.md](../engine/HIGHER-BASE-CERTIFICATION-STRATEGY.md)):
an exact outer lexicographic branch-and-bound wraps the bucket engine as a
terminal oracle — every branch above the incumbent is bound-pruned,
exhaustively refuted, or exact-searched, so **window width is a performance
knob, never a soundness boundary**, and discovery heuristics can never
justify a refutation. Validated by reproducing and *proving* the known
a(56)/a(58)/a(60) end-to-end (upgrading all three to ×2-method certified);
live in the unified production binary with resumable, shard-parallel proof
manifests.

## b64 — a(64) determined by a one-line arithmetic obstruction

**Value (114 decimal digits, 63 base-64 digits — the entire alphabet
`{1,…,63}`):**

```
615501230581118093011161624139305632631631099715005717665003460230260844588863956324726850814214663290418203804000
```

This supersedes the previous `|D|=60`, 109-digit WEAK value on both counts:
more digits and numerically larger. Two independent completions were found —
one by `certset` at W=24, one by `certdisc` at W=22 from a different prefix —
and both pass all seven validity gates.

**Why the forced set is the whole alphabet.** `64 = 2⁶`, and the largest power
of two ≤ 63 is `32 = 2⁵`, so `64 ∤ lcm(1..63)` with **no drops at all**. The
digit-sum condition holds outright: `1+⋯+63 = 2016 = 63 × 32 ≡ 0 (mod 63)`.

**Why the value is maximal.** With `L = lcm(1..63)` we have `v₂(L) = 5`, so the
nilpotent part is `L_nil = 2⁵ = 32` and `B¹ = 2⁶ ≡ 0 (mod 32)` — that is,
**`T = 1`**. Every digit at position ≥ 1 therefore contributes `0 (mod 32)`, so

> `32 | N` **iff** 32 divides the units digit — and **32 is the only digit in
> `{1,…,63}` divisible by 32**. Hence *every* completion ends in the digit 32.

The value above matches plain descending order for its first 31 positions,
then places 31 where descending would place 32, exiling 32 to the final slot
exactly as the obstruction demands. Any competitor falls into one of four
cases: deviating before position 31 is lexicographically smaller (those
positions are forced maximal, and all completions have 63 digits so digit-lex
order *is* numeric order); deviating anywhere in positions 31–37 requires
placing **32 inside the prefix**, which starves the units position and kills
all seven lex-greater sub-regions at once; an equal prefix is settled by an
exhaustive terminal search (`survivors=1`, verified, no resource decline); and
any smaller digit set yields a shorter, smaller number.

**Three of those four cases are checkable on paper.** Only the equal-prefix
branch is engine-dependent, which is why the row is STRONG rather than
CERTIFIED — a second independent method on that one branch is what is still
missing. Full argument and controls: **[A64-MAXIMALITY.md](A64-MAXIMALITY.md)**
(independently re-derived by an outside reviewer, 2026-07-26 — see
[../reviews/SHOULD-THE-PROJECT-STOP-AT-BASE-64.md](../reviews/SHOULD-THE-PROJECT-STOP-AT-BASE-64.md)).

**Computational corroboration.** A census of the lex-greater region returned
**0 feasible across 906,192 digit-subsets**, standing for 5.478 × 10⁸ ordered
prefixes at W=24. That census is simply this obstruction evaluated
mechanically.

**And the scale is worth stating plainly, because it is the clearest
illustration in this project of why structure beats compute.** At W=22 the
lex-greater region above this value contains **328,688,069,455 terminal
prefixes**. This machine completes roughly **1.5 terminals per hour**. Every
one of those 3 × 10¹¹ regions is disposed of by a single arithmetic
observation — *every completion must end in 32, so no prefix consuming 32 can
complete* — an argument that fits in one sentence and takes no compute at all.
The same lesson as the forced-set audit: **the structure was derivable in
advance; the search never had to be run.**

> ⚠️ **The census is a one-sided test.** `FEASIBLE = 0` is conclusive;
> **`FEASIBLE > 0` says nothing** — feasibility is necessary, not sufficient.
> Run above b63's engine-confirmed value it reports nonzero in most regions,
> yet every one of those was refuted by exhaustive search. And at **b59/b61**
> the base is prime, so `L_nil = 1` and there is no nilpotent constraint at
> all: the census is **vacuous there by construction**.

**The lens does not generalise.** `T = 1` requires `L_nil | B`, and b64 is the
only base in range where that holds:

| base | 54 | 56 | 58 | 59 | 60 | 61 | 62 | 63 | **64** |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `L_nil` | 288 | 196 | 32 | 1 | 864 | 1 | 32 | 147 | **32** |
| `T` | 5 | 2 | 5 | 0 | 3 | 0 | 5 | 2 | **1** |

Everywhere else `T ≥ 2`, so the constraint binds the last `T` digits jointly
and forces no single digit. Bases 54, 59, 61 and 62 still required real search.

## b63 — first-ever value, complete maximality argument (STRONG, single method)

Base 63 was the last base to fall — the only one in 50–64 that ever stood at
NO-VALUE. The forced digit set is uniquely determined — {1..62} minus
{9,18,27,28,36,45,54}, so |D| = 55 — and the outer-lexicographic
branch-and-bound ground the arrangement space on 3 parallel shards.

The value (99 decimal digits):

```
919638671548642431083440200815272686323739054438990208528434484026726330816976573347252565881592800
```

Independently verified from scratch (not the engine's own report): base-63
decode matches, 55 distinct nonzero digits all in 1..62, digit set is
exactly the forced complement, lcm(digits) | N, 63 ∤ lcm (gcd = 21),
N ≡ digitsum (1736) mod 62.

Because verified survivors exist, the forced 55-set cannot fully refute — so
b63 yields a first-ever *value*. What remained open was only whether this
survivor is the maximum.

### The cutoff argument: the proof is already covered

Exhaustively traversing the whole arrangement space is unnecessary. Only
prefixes that could still produce a **larger** completion matter.

At terminal width W = 22, terminals are entered after 55 − (22+1) = **32**
prefix digits (confirmed: every manifest record carries a 32-digit prefix).
The incumbent's first 30 digits are exactly the descending top-30 of the
forced set, so any prefix deviating before position 31 is lexicographically
smaller and every completion beneath it is dominated — no proof record
needed. The prefixes that could still win are therefore:

- position 31 ∈ {29, 26, 25}, each leaving 24 choices at position 32 → 72
- position 31 = 24 with position 32 ∈ {29,26,25,23,22,21,20,19,17} → 9
- the incumbent's own prefix → 1

**82 lex-relevant terminal prefixes.** Note this is the *combinatorial*
count, not an engine counter range: in the current shard run the engine
assigned terminal counters only to the 40 of them it actually executed —
counters 0..39 are exactly the prefixes lexicographically ≥ the incumbent,
with **the incumbent at counter 39** — while the other 42 already carried
definitive records from the earlier serial run and were skipped without
being counted (the banked manifest's depth-32 records carry no `counter`
field at all). The two runs share no counter space, so the union is sound
only when keyed by exact prefix, which is how it is checked below.
Checking the union of the three shard manifests and the banked
manifest **by exact prefix** (aggregate counts prove nothing):

| check | result |
|---|---|
| lex-relevant prefixes covered | **82 / 82**, 0 missing |
| disposition mix | **81 REFUTED + 1 FOUND** |
| the 1 FOUND | the incumbent's own branch, maxSurvivor = the incumbent |
| conflicting duplicate records | 0 |
| RESOURCE_DECLINED gaps | 0 — 8 of the 82 were once declined, **all later re-run to definitive** |
| global max survivor, all records | equals the incumbent |
| parameter compatibility | all 4 files: every prefix a distinct 32-subset of the forced 55-set |

Every prefix that could lexicographically beat the incumbent is exhaustively
refuted, and the incumbent's own branch has it as the branch maximum. **That
is the maximality proof**, assembled from work already completed.

**Honest boundary:** this independent verification checked the *coverage* and
the *cutoff logic* from scratch; it trusts the engine-produced REFUTED
records rather than re-running 81 exhaustive terminal searches. Two
independent verifiers (boss-clod's and this session's, each derived from
scratch) agree on the cutoff and the coverage; neither re-executed a
terminal.

**ENGINE CONFIRMATION LANDED (2026-07-25).** An earlier revision of this
section claimed the engine-level check was unobtainable from the current
binary. That was wrong, and the correction matters: **the lex-domination
cutoff already exists** (`carrytrie.cpp` ~L5897: `upperBoundArrayBB` compared
against the pruning incumbent, returning `PROVED_BELOW_INCUMBENT`, plus a
per-child monotonic `break`). It never fired during the 17-hour shard grind
for a mundane reason — shard mode freezes the pruning incumbent once at
startup, the banked manifest then held **zero** FOUND records, so it froze at
`(none)` and pruning was disabled for the run's entire life. Discovering the
incumbent at counter 39 did not tighten it, by design.

Re-running plain `resume` against a manifest that actually contains the FOUND
records (isolated directory, repo files untouched, input manifest hashed):

```
resume: loaded 3767 lines -> 1246 proven-REFUTED (skip), 32 proven-FOUND (skip+seed)
resume-seeded incumbent from manifest: 9196386715486424310834...81592800
DFS DONE: nodesVisited=117 found=0 refuted=0 pruned=32 unfinished(declined)=0
          resume-skipped(refuted=81 found=1) wall=136.073s
base=63 CERTIFIED (zero unfinished branches)
```

**`found=0 refuted=0` — zero new terminal executions**, and
`resume-skipped(refuted=81 found=1)` is exactly the 82-prefix frontier
derived above, reached independently by the engine's own DFS. 32 bound-prunes,
zero declined, 136 seconds. The 24 branches the union reports as unfinished
are precisely the 24 counters `certbb-merge` flagged as uncovered (the ragged
tail at counter ≥1193) — the DFS **prunes** them as dominated rather than
needing them, so the merge's objection and the cutoff's answer are the same
branches.

Log: [`../../b63_engine_confirmation/`](../../b63_engine_confirmation/).

This row is therefore **STRONG — single-method exhaustive, engine-confirmed**.
It is deliberately *not* labelled CERTIFIED: the project's ladder reserves
that for concordance from an independent *engine family*, and the check above
— however clean — is the same engine agreeing with itself. Two independent
verifiers derived the cutoff and coverage by hand, and the engine's own DFS
then reproduced the identical 81+1 frontier, which is strong evidence and not
a second method. Method and credit:
[`../reviews/FASTER-PROVISIONAL-MAXIMUM-VALIDATION.md`](../reviews/FASTER-PROVISIONAL-MAXIMUM-VALIDATION.md).

Note b63 sits strictly below b64 by construction: 55 base-63 digits cap it
at 99 decimal digits, while b64 (which needs no forced drops, since no digit
≤ 63 is divisible by 2⁶) keeps 63 digits and has a 114-digit maximum.

## b61 — the discovery-side base, and a decomposition that found it

**Value (106 decimal digits, 59 base-61 digits; forced set = `{1,…,60}` minus
`{30}`):**

```
2159432391576551378277658181546813434552677183954783130085487155062653805766432381530957181202633703075200
```

Supersedes the previous `|D|=58`, 104-digit WEAK value: **more digits, hence
strictly larger.**

**Why b61 resisted every technique that worked on the others.** Its previous
incumbent used **58** digits while the forced set has **59**. Every 59-digit
base-61 number exceeds every 58-digit one, so **no lex cutoff could fire at
all** — the incumbent was one digit too short to prune anything. b61 was never
"harder"; it was still on the **discovery** side of the pipeline while b54,
b59, b62 and b64 had crossed to the proof side. Restoring the two missing
digits multiplies the effective modulus by exactly `43 × 47 = 2021`, and the
found value satisfies that larger modulus: `lcm(digits) = 9690712164777231700912800`,
the forced 59-set's `L_eff`.

**How it was found.** A single W=23 terminal was projected to take ~10 h
against a 6 h cap — likely to burn its budget and return nothing. Instead it
was **decomposed exactly**: fix the descending top-35 prefix, let position 36
range over the 24 remaining digits, and each child becomes an independent
W=22 terminal whose union is precisely the parent. 23 children (the 24th being
an already-refuted terminal), checkpointed, sharded three ways, stopping
globally on the first hit. **Child 16 hit at 1245 s**, after 15 refutations.

**Maximality within the window comes for free from the ordering.** Children ran
in strictly descending position-36 order, so the hit at `pos36 = 8` is
preceded by refutations at **24, 23, …, 9 — contiguous, no gaps**. The top-35
prefix is the lex-greatest 35-prefix available, so nothing inside the W=23
terminal beats this value and the five unrun children are all lex-smaller.
Coverage table: [`../../b61_decomposition/POS36_COVERAGE.md`](../../b61_decomposition/POS36_COVERAGE.md).

> **Scope — upgraded 2026-07-26, with no new compute.** Framed at W=22 the
> terminal prefix is 36 digits, and the incumbent's is *descending-top-35 +
> [8]*. Its first 35 digits are the lex-greatest 35-prefix available, so the
> only lex-greater 36-prefixes are `top-35 + d` for `d > 8` — **exactly 16**,
> and **all 16 already carry definitive REFUTED records** at identical
> parameters. A W=22 terminal is exhaustive over its remaining pool, so no
> completion exists under any of them. **a(61)'s maximality is therefore
> exhaustive, not window-bounded**, and b61 joins b54/b59/b62/b64 at
> **STRONG**. Still **not CERTIFIED** — that needs a second independent engine
> family. Count and coverage: [`../../b61_decomposition/POS36_COVERAGE.md`](../../b61_decomposition/POS36_COVERAGE.md).

## b62 — a(62)

**Value (106 decimal digits, 59 base-62 digits; forced set = `{1,…,61}`
minus `{30,31}`):**

```
5636285065773651499796945616994676443519614972212770252490392293226928630689747617736444403004286825548000
```

Supersedes the previous `|D|=58`, 104-digit WEAK value on both counts. Found
by `certset` at **W=22 in 1844s**, the cheapest rung.

Maximality is the same structural argument as b54 and b59: the terminal
prefix length is `59 − 23 = 36`, the value's first 36 digits are exactly the
**descending top-36** of the forced set, so there are **zero lex-greater
prefixes** (0 sub-regions, confirmed), the window of 23 covers the entire
remainder, and only the equal-prefix branch exists. **STRONG**, resting on the
single `runWrongTurnSearch` exhaustiveness dependency.

This was the **third consecutive out-of-sample confirmation** of the `r0`
prediction — see the b59 section for what that prediction is and why it holds
by construction rather than by observation.

## b59 — a(59), and the first *prediction* this framework made and cashed

**Value (101 decimal digits, 57 base-59 digits; forced set = `{1,…,58}`
minus `{29}`):**

```
86783176769582410820652763941251198601388207643947455481175050246380721520450704821249718313022861600
```

Supersedes the previous `|D|=56`, 100-digit WEAK value on both counts. Found
by `certset` at **W=22 in 1774s**, the cheapest rung.

**Maximality, same shape as b54.** At `W=22` the terminal prefix length is
`57 − 23 = 34`, and the value's first 34 digits are exactly the **descending
top-34** of the forced set — the lexicographically greatest prefix available.
So there are **zero lex-greater prefixes** (confirmed computationally: 0
sub-regions), the window of 23 positions covers the entire remainder
(`57 − 34 = 23 = W+1`), and the case analysis collapses to the equal-prefix
branch alone.

**What makes this one methodologically different: it was predicted in
advance.** Before the run returned, `r0 = 1` — a by-product of feasibility
censuses that had *filtered nothing and were written off as wasted* — implied
that `certset` would search the descending-top prefix, hence that any hit
would be immediately maximal. The prediction was then strengthened to a
guarantee: for b59, `gcd(lcm(D), 59) = 1`, so `L_nil = 1` and `T = 0`, the
admissible-suffix DP has depth zero and returns 1 unconditionally, and
therefore **`r0 = 1` holds at every width by construction** rather than by
observation.

It held. This is the framework's first genuine *prediction* — made before the
result, on a base not yet solved — rather than a rule fitted to cases already
known. See [../process/CERTBB-OPERATIONAL-FOOTGUNS.md](../process/CERTBB-OPERATIONAL-FOOTGUNS.md)
§13 for the general lesson about measurements that fail at their stated purpose.

## b54 — a(54), maximal because there is nothing above it

**Value (89 decimal digits, 51 base-54 digits; forced set = `{1,…,53}` minus
`{26,27}`):**

```
22486771935366632201839109831632289737092127593455046754337233294898735154535035683832000
```

Supersedes the previous `|D|=50`, 87-digit WEAK value on both counts. Found by
`certset` at **W=22 in 522.7s** — the cheapest rung, no ladder required.

**Why it is maximal — the whole argument.** At `W = 22` the terminal prefix
length is `51 − 23 = 28`, and this value's first 28 digits are

```
53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32 31 30 29 28 25 24
```

which is **exactly the descending top-28 of the forced set** (26 and 27 are
dropped, so 28 is followed by 25). The lexicographically greatest prefix
available *is* the largest 28 digits in descending order — so this prefix is
that maximum, and **there are zero lex-greater prefixes**. There is no region
to dispose of because the region is empty.

The case analysis collapses to one branch:

1. **Lex-greater prefix** — none exist. Vacuous.
2. **Equal prefix** — `certset` searched that prefix's window exhaustively;
   the window is 23 positions and `51 − 28 = 23`, so it covers the entire
   remainder, and it returned this value as the maximum.
3. **Smaller digit set** — fewer digits, strictly smaller number.

So a(54) rests on **exactly one computational dependency**,
`runWrongTurnSearch`'s exhaustiveness — the same single dependency as b64's
equal-prefix branch, and without b64's seven regions to eliminate first. It is
labelled STRONG rather than CERTIFIED for the same reason: a second
independent method on that one terminal search is what is still missing.

Compare b63 (2 free positions above the prefix → 82 branches to refute) and
b64 (6 free positions → 7 regions, disposed of by an arithmetic obstruction).
**b54 has 0 free positions**, which is the cheapest shape this problem admits.

## Correction to the published a(46)

This solver found a value **strictly larger** than the published OEIS entry,
using the same (uniquely forced) digit set {1..45}\{22,23} and the same
lcm = 409547311252279200 — the published arrangement drops digit 28 out of
its descending slot; the corrected one keeps it. Verified independently
(divisibility + digit multiset) twice.

- published (suboptimal): `315044747190120671695735975284033252460559821155925276163089767538975200`
- corrected: `315044747190120671695735975284412123404260147529994283460952247723479200`

The divergence-depth law initially flagged the published a(40) and a(48)
for the same reason — both were subsequently CLEARED (a(40) by direct
solve, a(48) by a nilpotent-peeling certificate): their divergence
outliers are *forced nilpotent-suffix effects* (the last digits are pinned
to 20 resp. 24 by suffix arithmetic), now fully explained. Final audit:
3 flags → 1 real error (a46, corrected above), 2 cleared. a(49), beyond
the published b-file, is certified: see
[../theory/NILPOTENT-PEELING.md](../theory/NILPOTENT-PEELING.md).
