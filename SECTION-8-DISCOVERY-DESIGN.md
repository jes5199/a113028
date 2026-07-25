# §8 completion searches: why the prover is the wrong instrument, and what to build

**Date:** 2026-07-25
**Status:** design proposal, nothing launched. Written after the b64 outer
proof had to be killed for filling 2.2 GB of disk in 4.5 minutes.

## What happened

`FORCED-SET-CARDINALITY-AUDIT.md` confirmed §8: for b54/59/61/62/64 the
forced maximal-cardinality set is larger than the set each provisional value
uses, so *any* completion of the forced set supersedes the current value on
cardinality alone. b64 is the extreme case — the forced set is the entire
alphabet `{1,…,63}`, 63 digits against the current 60.

The obvious move was to point `certbb` (the outer lexicographic B&B) at that
set. It produced **7.9 M `EXACT_TERMINAL_REFUTED` records in 4.5 minutes,
2.2 GB of manifest, every terminal at `wall=0.000`**, and was killed.

The first line of that manifest diagnosed it:

```json
{"record":"run_header","base":64,...,"terminalPrefixLen":40,"pruningActive":false,...}
```

## Why the prover is the wrong instrument

`certbb` is a **maximality prover**. Its efficiency comes entirely from
bound-pruning against an incumbent: it asks *"can this branch beat the value
I already hold?"* and answers in O(1) for the overwhelming majority.

§8 asks the opposite question — *"does any completion exist at all?"* — and
in that regime there is no incumbent, so `pruneIncumbentPtrBB` returns
`nullptr` and **nothing prunes**. The DFS then enumerates a 40-deep prefix
space exhaustively, writing a record per terminal. It is not a long run; it
is an unbounded one. The same failure awaits all five §8 bases: none of them
has a forced-set completion yet, so all five would hit this identically.

This is the same root cause as the b63 17-hour burn (`FROZEN at (none)`,
footguns §1). There it cost CPU. Here it costs disk, on a volume shared with
a live trading process.

## The correction to §8's promise

§8 says it "converts five maximality grinds into five completion searches."
That is true, and it is still a good trade — but the corollary was not
stated, and it matters:

> **The completion searches are themselves band-depth-limited. §8 relocates
> the campaign's open problem; it does not dissolve it.**

Evidence: b64's forced set was *already attempted*. The W=21 fast pass
(`b64_certauto_w21.log`, `subsetsChecked=17105 subsetsScanned=287`) descended
k = 63, 62, 61 and only landed at `|D|=60`. Those higher-k failures were
**window-bounded refutations, not proofs** — precisely the class that has
concealed larger answers three times in this campaign (b50, b56, b58; see
FRONTIER-STATUS.md). So "find a completion of the b64 forced set" is exactly
"beat a W=21 window-bounded refutation", which is the band-depth problem.

What §8 *does* buy, and it is genuinely large: it changes **where** the cost
sits. The pipeline becomes

```
discovery (hard: band-depth-limited)  ->  maximality proof (easy: minutes, given an incumbent)
```

The b63 result is the proof of the second half — 136 seconds, zero terminal
executions, once seeded. So effort should go to discovery and nothing else.
This aligns with the independent observation that for bases 65–89 the binding
constraint is finding a completion at all, not proving maximality afterward.

## Proposal A — manifest-growth guard (build first, it is cheap)

"Unbounded proof writes unbounded manifest" is a footgun independent of b64.
Key it on **absolute free space remaining**, not bytes written: the danger is
not 2.2 GB, it is 2.2 GB when 20 GB remains on a volume shared with a live
trading process.

- Sample `statvfs` on the manifest's filesystem at open, then every N records.
- **Refuse to start** if free space is below a floor (default ~5 GB).
- **Downgrade or abort loudly** when free space crosses the floor mid-run —
  never degrade silently. Same shape as the unseeded-shard guard: fail early
  and loudly.
- A `counters-only` mode (record dispositions without prefix arrays) is the
  natural downgrade, but aborting is acceptable for v1.
- Env override for deliberate large runs, consistent with
  `CERTBB_ALLOW_UNSEEDED_SHARD`.

Independent of any §8 decision, so it should land regardless.

## Proposal B — a discovery instrument, not a prover

The promising signal from the killed run: **all 7.9 M refutations were at
`wall=0.000`**. A cheap infeasibility test is doing the work — no expensive
search ran. So the *feasible* prefix set is plausibly minute relative to the
raw branch count, and the cost is enumerating the infeasible ones one at a
time and journalling each.

That is the same shape as the **b60 discovery-churn fix** (Sol doc #24), which
took b60 from a >5400 s NO-VALUE to a 13.9 s certification — a 390×
turnaround — by *generating only feasible candidates* instead of filtering a
full descending stream. The instrument wanted here is that one, aimed at
arrangements rather than subsets:

1. Enumerate **feasible prefixes directly** for the fixed forced set, rather
   than descending the full prefix tree and rejecting.
2. **Do not journal refuted branches** during discovery. Discovery is not a
   proof and its records are not proof-bearing; writing them is what
   generated 2.2 GB. Emit only survivors, plus aggregate counters.
3. Stop at the **first** completion — for §8 any completion wins on
   cardinality, so this is a satisfiability search, not an optimisation.
4. Only then hand the completion to `certbb` as a seeded incumbent, where the
   maximality proof is the minutes-long operation b63 demonstrated.

Cheapest first experiment, before any of that is built: re-run the existing
`certset` probe on b64's forced set at **increasing `CERTSET_W`** (22 → 23 →
24). It is one terminal call per width, bounded, writes nothing large, and it
directly tests whether the W=21 refutation was window-bounded. `certset` at
W=22 already came back `REFUTED at the heuristic prefix` in 46.8 s, with the
engine's own caveat that this refutes *one prefix's window*, not the set.
Note the ~9×/width cost growth: W=24 is the practical ceiling.

## Recommended order

1. Manifest-growth guard (Proposal A) — cheap, general, unblocks safe running.
2. `certset` width ladder on b64's forced set — cheap, bounded, informative,
   needs no new code.
3. Feasible-prefix discovery enumerator (Proposal B) — the real instrument,
   scoped by what step 2 reveals.

**Nothing is relaunched at b64 until a decision on this document.**

---

## Addendum (same day): the existing code already contains most of Proposal B

Read of `buildFeasiblePrefix` (carrytrie.cpp ~L3717) while W=24 ran. Two
findings that make Proposal B smaller than it looked, and one that explains
why the ladder alone was never going to be enough.

### Finding 1 — a second, unexplored axis: `maxReleases`, hardcoded 3 everywhere

`buildFeasiblePrefix(c, targetWY, maxReleases, ...)` starts from the naive
descending prefix; if that prefix's pool admits no suffix tuple it *releases*
r prefix digits and *promotes* the r boundary-adjacent pool digits — a
size-preserving swap, since `runWrongTurnSearch` hard-requires
`pool.size() == windowSize`. It searches r = 1..`maxReleases`.

**Every one of the six call sites passes `maxReleases = 3`** (L4891, L5066,
L5253, L5255, L5662, L6307). It is a hardcoded constant, never an argument,
never an env knob.

So the campaign has explored two things very unevenly:

| axis | what it controls | how far we've gone |
|---|---|---|
| `W` (`CERTPOS`/`CERTSET_W`) | depth of the terminal window | laddered 20→24, engine ceiling 24, **~16.5×/width** |
| `maxReleases` | how far the *starting prefix* may deviate from descending | **fixed at 3, forever, everywhere** |

These are orthogonal. Widening `W` searches deeper *below a given prefix*;
raising `maxReleases` reaches *different prefixes*. Every "window-bounded
refutation" in this campaign is bounded on **both** axes, but only one has
ever been laddered.

### Finding 2 — `certset` tries exactly ONE prefix, and that is the real limit

`buildFeasiblePrefix` returns the **first** feasible prefix it finds and
stops. `certset` runs one terminal on that one prefix. So
`REFUTED at the heuristic prefix` means precisely: *the first prefix within 3
release/promote swaps of descending order has no completion inside a
width-W window.*

This also corrects a tempting move: simply raising `maxReleases` does **not**
help `certset`, because when a feasible prefix already exists at r ≤ 3 the
function returns that same prefix regardless of the cap. The cap only matters
when nothing is feasible within it.

### Consequence — the precise shape of Proposal B

Not "write a new enumerator". Generalise the existing one:

1. Change `buildFeasiblePrefix` from *"return the first feasible prefix"* to
   *"enumerate feasible prefixes in order"* (callback or iterator). The
   release/promote loop already emits them in a principled order — fewest
   releases first, i.e. nearest to descending order first, which is the right
   order for a satisfiability search.
2. Drive a terminal per feasible prefix, **stopping at the first completion**
   (§8 wins on cardinality; there is nothing to maximise).
3. Journal **survivors only**. The refuted-branch records are not
   proof-bearing during discovery and are exactly what wrote 2.2 GB.
4. Expose `maxReleases` as an env knob so *this* axis can be laddered — it is
   plausibly far cheaper per step than `W`'s measured ~16.5×, since each
   additional release costs a `countAdmissibleSuffixTuplesGen` call
   (cheap) and only the *feasible* prefixes draw an expensive terminal.

The existing machinery — feasibility test, size-preserving swap, terminal
runner, planner — is all reusable. The change is control flow: enumerate
instead of return-first, and stop on success instead of on the first failure.

### Standing caveat, restated

A refutation from this instrument is bounded on both axes. "No completion
found within `maxReleases` swaps and a width-W window" is **never** "no
completion exists". Record both bounds with every negative result.
