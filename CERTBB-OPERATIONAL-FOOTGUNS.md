# certbb operational footguns

Failure modes of the outer branch-and-bound proof driver that are **silent** —
they produce plausible-looking output while doing the wrong amount of work.
Each one below cost real time before it was understood. Read this before
launching a long proof run.

## 1. Shard mode frozen at `(none)` disables pruning for the whole run

**Symptom in the log:**

```
[certbb] shard 0/3: pruning incumbent FROZEN at (none)
```

**What it means.** Shard mode snapshots the pruning incumbent exactly once,
after resume-loading and discovery-seeding, before the DFS starts. The freeze
is deliberate and *necessary*: if survivors found mid-traversal could tighten
the bound, two shards would prune different subtrees, the deterministic
terminal counter would desynchronise, and the merge step's prefix alignment
would silently corrupt.

But freezing at `(none)` is the pathological case. `pruneIncumbentPtrBB`
then returns `nullptr` for the entire session, so **every** bound-pruning
test is skipped and the DFS grinds the whole arrangement space — including
every branch lexicographically below a survivor it finds later, because the
freeze forbids that survivor from ever tightening the bound.

**What it cost.** The b63 run (2026-07-25) burned ~17 CPU-hours across three
shards this way. It found its incumbent at terminal counter 39 and then
executed ~1,200 further terminals, every one of them provably dominated by
that incumbent. Nothing in the output said pruning was off. Re-run with the
incumbent seeded, the same proof completed in **136 seconds with zero terminal
executions** (`nodesVisited=117 found=0 refuted=0 pruned=32`).

**Guard (added 2026-07-25).** Shard mode now **refuses to start** (exit 6)
when the freeze would land on `(none)`, printing the fix. Override only when
no completion is known for the base at all — a genuinely unseeded first run:

```
CERTBB_ALLOW_UNSEEDED_SHARD=1
```

**Correct workflow:** run a non-shard discovery/resume pass first so the
baseline manifest holds at least one `EXACT_TERMINAL_FOUND` record, *then*
launch the shards with `resume`.

**Why the guard fires *after* the discovery-seed pass, not at manifest-load.**
It looks like the condition is knowable earlier — the manifest either has a
proven-FOUND record or it doesn't — but those are different predicates. The
discovery-seed pass can set the incumbent *itself* (on a FOUND seed it
assigns `ctx.incumbent` and sets `incumbentSet`), so a run with zero FOUND
records in the manifest may still be seeded by discovery and prune normally.
Refusing at load time would reject the **first proof run on every new base**,
which is exactly what the override exists to permit. The freeze point is the
only place the answer is actually known. The cost of the late check is one
discovery-seed terminal call — bounded, not a full-space grind.

## 2. `CERTBB_MANIFEST` is the OUTPUT path only — resume READS the baseline

`CERTBB_MANIFEST` overrides where records are *written*. Resume always
*reads* the hardcoded `certbb_<base>_manifest.jsonl` in the working
directory.

So pointing `CERTBB_MANIFEST` at a nicely-seeded manifest **does not seed the
incumbent**. The run reports `0 proven-FOUND`, freezes at `(none)`, and
(pre-guard) grinds the full space — reproducing footgun #1 exactly. This
fooled a session that was specifically trying to demonstrate footgun #1.

To seed, put the records at the baseline path. The clean way, which leaves
the repo untouched:

```sh
mkdir rundir && cd rundir
ln -s /path/to/carrytrie_cert.newNN .
cp /path/to/seeded_records.jsonl certbb_<B>_manifest.jsonl
./carrytrie_cert.newNN certbb <B> <drops> <rssKB> resume
```

## 3. `certbb-merge` refuses records whose prefix length ≠ terminal length

Exit 5, `FATAL: ... has a record with prefix length N != expected
terminalPrefixLen=M`. Manifests banked from earlier capped passes accumulate
`RESOURCE_DECLINED` records at *every* depth (b63's had 2,416 of them at
depths 1–31), and a single one aborts the merge. Filter to depth-`M` records
first. They are `RESOURCE_DECLINED` and therefore not proof-bearing, so
dropping them loses nothing — merge discards that class anyway.

## 4. `certbb-merge` coverage is whole-tree, not frontier

The merge demands a definitive disposition for **every** terminal counter
`0..max`, so it exit-4s on any unfinished tail — even when every branch that
could beat the incumbent is proven. On b63 it reported 24 uncovered counters,
all at counter ≥ 1193, i.e. ~1,150 branches past the incumbent's own branch
at counter 39, every one of them dominated. The merge's verdict was correct
about the whole tree and irrelevant to maximality.

Use the merge to check shard-split integrity. Do **not** read its exit code
as a verdict on whether the maximality proof is complete.

## 5. The engine's `CERTIFIED` is not the project's CERTIFIED

```
[certbb] base=63 CERTIFIED (zero unfinished branches)
```

This is the engine's internal completeness check: *this DFS left no branch
unfinished*. The project's evidence ladder (see FRONTIER-STATUS.md) reserves
**CERTIFIED** for concordance across an independent *engine family*. One
engine reporting `CERTIFIED` earns **STRONG**, not CERTIFIED. Do not promote
a row on the strength of this log line.

## 6. Terminal counters are per-run, not stable identifiers

Counters are assigned at the would-be-terminal execution point, *after*
resume-skip and bound-pruning — so a resumed or differently-seeded run
assigns different counters to the same prefixes, and skipped branches get no
counter at all. b63's banked serial manifest carries **no** `counter` field
on any depth-32 record, while the shard run numbered only the 40 branches it
executed.

**Always key manifest records on the exact prefix, never on the counter.**
A cross-run union keyed by counter silently compares different branches.

## 7. Design rule: an optimiser's failure must cost performance, never capability

Whenever a *heuristic* component — a planner, a cost model, a cache, a
precomputed index — sits in front of an exhaustive search, its failure path
must fall back to the thing that always works. If the fallback is instead
"give up on this branch", an optimisation has been silently converted into a
capability limit, and a proof that used to complete now doesn't.

**Worked example (2026-07-25, caught pre-commit by an A/B).** Phase 3 routed
outer-proof terminals through the calibrated bucket planner instead of the
hardcoded `NX=2 / K=3` split. `planBucket` declines whenever
`enumeratePeeledConfigs` yields no config, which includes the case where the
admissible-suffix-tuple DP itself declines (`suffixMult == UINT64_MAX`) — and
that happens on perfectly ordinary b56 terminals. The first implementation
mapped "planner declined" to a terminal `DECLINED`:

```
b56 legacy : nodesVisited=52 found=1 refuted=25 pruned=25 declined=0   CERTIFIED
b56 planner: nodesVisited=52 found=1 refuted=0  pruned=25 declined=25  INCOMPLETE
```

25 branches the legacy split **refutes outright** became 25 unfinished ones,
and b56 dropped from CERTIFIED to INCOMPLETE. Nothing was faster; the run
took the same 304s. The fix is a fallback to the legacy split, so the planner
path can never do *less* work than the engine it replaced — only the same or
better.

**Why this surfaced as a regression instead of a corrupted proof.** Because
`RESOURCE_DECLINED` is never folded into `EXACT_TERMINAL_REFUTED`, the run
reported an honest `INCOMPLETE` with its incumbent as a lower bound rather
than a confident, wrong `CERTIFIED`. Had the two dispositions been conflated
— the single most tempting simplification in this codebase — the same bug
would have produced a **false certification** of a base whose branches were
never actually searched, and no output would have looked wrong.

That is the entire argument for the never-conflate rule, demonstrated on live
code: it does not prevent bugs, it converts invisible ones into visible ones.
Keep it, and keep A/B-ing any change to a search path against the engine it
replaces — the regression was invisible in every aggregate except the
disposition mix.

## 8. An unpruned proof writes an unbounded manifest

`certbb` journals a record per terminal. With an incumbent that is a few
thousand lines; **without** one nothing bound-prunes, the DFS enumerates the
whole prefix space, and the manifest grows without limit.

**What it cost.** The b64 outer proof (`|D|=63`, `terminalPrefixLen=40`, no
known completion so no incumbent) wrote **2.2 GB in 4.5 minutes** — 7.9 M
`EXACT_TERMINAL_REFUTED` records, every one at `wall=0.000`. Roughly
30 GB/hour, against 20 GB free, on the volume that also hosts a live trading
process. The 8-hour cap set for it would have exhausted the disk in well
under an hour.

The run header diagnosed it in line one — `"pruningActive":false` — which is
what that field is for.

**Guard (added 2026-07-25).** Keyed on **absolute free space remaining**, not
bytes written: 2.2 GB is harmless with 200 GB free and an emergency with
20 GB free on a shared volume.

- Refuses to start (exit 7) when free space is below the floor.
- Re-samples every 4096 records and **aborts loudly** (exit 7) on crossing
  it. Records already written stay valid and resumable.
- Floor default 5 GiB, `CERTBB_MIN_FREE_GB` to change it,
  `CERTBB_ALLOW_LOW_DISK=1` to bypass deliberately.
- If free space can't be determined the guard disables itself and says so,
  rather than blocking a legitimate run.

Verified end-to-end on the run that caused the incident: startup passed at
21.17 GiB against a 20.87 GiB floor, then aborted at 20.86 GiB after 634,880
records / 161 MB. The same b64 run, bounded.

**Note the interaction:** hitting this guard usually means footgun #1 —
you are proving without an incumbent. Check `pruningActive` in the header
before raising the floor.

## 9. Never signal by pattern — resolve to exact PIDs and verify `comm` first

`pkill -f <pattern>` and `pgrep`-driven kill loops match **any** command line
containing the string, including the shell that is running the command
itself, unrelated tooling, and — on this box — a live-money Erlang VM
(`beam.smp`, pid 26593) that must never be touched.

**The rule:** resolve the intended targets to explicit PIDs, read
`/proc/<pid>/comm` for each, confirm it is what you expect, *then* signal —
and re-verify the protected process is alive afterwards.

```sh
for p in 713397 713398 713399; do
  c=$(cat /proc/$p/comm 2>/dev/null)
  if [ "$c" = "carrytrie_cert." ]; then kill -TERM "$p"; else echo "REFUSING $p: comm='$c'"; fi
done
[ -d /proc/26593 ] && echo "protected process still alive"
```

Prefer `SIGTERM`: the manifests are append-only JSONL and a graceful stop
flushes the in-flight line instead of truncating it mid-record.

**How this was learned, cheaply.** Stopping the b63 shards and the runaway
b64 prover both used the exact-PID + `comm`-check form above and were clean.
A later `pkill -f "until ! pgrep"`, aimed at some dead waiter loops, matched
the very shell issuing it and killed the session's own command (exit 144).
Harmless that time. The same carelessness pointed at a busier pattern is how
an unrelated production process gets killed by a maintenance command.

## 10. Nice the binary, not the launcher — script children outlive their parents

`nice -n 19 ./driver.sh` niceness applies to the shell; a compute binary it
spawns inherits that, but a binary spawned by a *Python* or *shell* driver
that was itself started unniced runs at **ni 0** — and killing the driver does
not kill the child. It is reparented to PID 1 and keeps running, unniced,
against whatever else is on the box.

**What it cost.** On 2026-07-25 a `certdisc` child of a killed control driver
ran **34 minutes at ni 0**, competing with a live-money `beam.smp`, while the
status reports for that window said the box was idle. Nobody noticed because
the *launcher* had been dealt with and the *process* had not.

**The rule:** apply `nice` to the compute binary itself, at the innermost
invocation — not to the wrapper that launches it. And after killing any
driver, re-check for surviving children:

```sh
ps -eo pid,ni,etimes,comm | grep <binary> | grep -v grep
```

Same family as §9: **the thing you think you controlled is not always the
thing that is actually running.** §9 is about signalling the wrong process;
this is about a process you forgot exists.

**Corollary for status reporting.** The orphan was invisible because "box
idle" had been measured earlier and then *carried forward* across later
reports without re-measuring. **A stale measurement asserted as current is a
wrong measurement.** Re-measure before every status claim, especially while
deep in analysis work — that is exactly when it feels safe to reuse the last
number and exactly when the box has changed underneath you.

## 11. When you relabel a job's purpose, re-read its invocation

A running job's *label* lives in your head; its *behaviour* lives in its
argv. Those drift apart whenever the plan changes underneath a job that is
still running — and the plan changing is exactly when you are most likely to
act on the label.

**What it nearly cost.** On 2026-07-25 two `certset` runs were launched to
measure per-terminal cost for a release-axis sweep. The plan then changed:
the sweep was abandoned in favour of a width ladder, and the jobs were
described — by me, in my own status report — as "single-terminal probes for a
sweep I'm no longer proposing," slated to be killed for their cores. Reading
the actual invocation first:

```
certset base=59 dropsArg=29  W_terminal=22
certset base=61 dropsArg=30  W_terminal=22
```

They *were* the width ladder's W=22 rung. Killing them would have discarded
8½ minutes of precisely the computation the new plan needed, then relaunched
it identically from zero.

**The rule:** before acting on a running job — killing, renicing, reasoning
about its output — read `/proc/<pid>/cmdline` or `ps -o args`, not your
memory of why you started it. Same family as §10's stale measurement: **the
thing you believe is running is not always the thing that is running.**

### The mirror case: a job that finishes quietly

The same drift has a second form, and watching for one blinds you to the
other. On 2026-07-25 a `certset` run at b61 was under a 3600s cap and being
watched *for a timeout*. It exited on its own at **1415s** — a real verdict,
with 36 minutes of budget unused — and the watch, armed for the wrong event,
said nothing. The result sat unread until someone checked whether the process
was still *there* rather than whether it had *reported*.

- Watching for a **hang** misses a job that **finished**.
- Watching for a **finish** misses a job that **died**.

So a completion watch must test the process's existence *and* the log's
content, and treat "process gone, no verdict written" as its own outcome —
that is a crash, not a silence. The general form covers both this and §11's
relabelling case:

> **Your model of the box drifts from the box.** Re-read the actual state —
> argv, `/proc`, the log — rather than the state you are expecting.

## 12. b64 is a systematic outlier: distrust anything measured there

Four separate quantities measured on b64 failed to transfer to other bases,
all in the same direction (b64 cheaper/stronger), and all from **one root
cause: `T = 1`**.

| quantity | b64 | elsewhere |
|---|---|---|
| feasibility-census cost | 0.015s | 404s (b54, `T=5`) |
| filter strength | 92.7% eliminated | **0.0%** (b54/b59/b61) |
| release axis vs width axis | release cheaper (907s vs 1788s) | release ≥4.4 days vs width ~hours |
| one W=22 terminal | 46.8s | **≥533s** (b59/b61) — ≥11× |

`T = 1` pins the nilpotent suffix to a **single forced digit**, which
simultaneously makes the feasibility filter a razor, makes it cheap to
evaluate, makes the release axis affordable, and prunes the terminal search
itself. One cause, four symptoms.

**The predictive form of the rule:**

> **Anything downstream of the nilpotent constraint does not transfer from
> b64.** Check `T` before reusing any b64-derived measurement, and re-measure
> locally when `T > 1`.

This is not "b64 is weird" — it says *in advance* which measurements to
distrust and why, and it is testable: compute `L_nil` and `T` for the target
base first (see A64-MAXIMALITY.md's table).

## 13. A measurement that fails at its stated purpose may still hold the answer to a different question

Before discarding a measurement as wasted, look at **what it actually
measured**, not at what you wanted it to measure.

**Worked example.** The feasibility censuses on b54 and b62 cost **1,270
CPU-seconds and filtered 0.0% of prefixes** — a total failure at their stated
job, and correctly abandoned on those grounds (see §7: an optimiser that
costs more than it saves should be bypassed).

But the same runs reported `r0` — whether the *naive descending* prefix is
feasible — and that single bit turns out to predict something far more
valuable than the filtering ever would have:

- `buildFeasiblePrefix` returns the **first** feasible prefix.
- So `r0 = 1` ⇒ `certset` searches the **descending top-N** prefix.
- The descending top-N is by construction the **lex-greatest** N-digit prefix
  available.
- ⇒ **zero lex-greater prefixes exist**, so a FOUND at that prefix is
  *immediately maximal* — no census, no proof run.

It retrodicts both settled bases: b54 (`r0=1`) found at the descending prefix
and was maximal on the spot; b64 (`r0=0`) fell through to an `r≥1` prefix and
therefore needed seven lex-greater regions disposed of.

So a census that filtered nothing told us, in advance and for free, **which
bases get their maximality for nothing if a completion is found** — turning a
discarded optimisation into a planning instrument.

**The rule:** when an instrument underperforms, separate *"this did not do
what I wanted"* from *"this produced no information."* They are different
claims, and the second is much rarer than it looks.

## 14. Acting on your model of a thing instead of the thing (the unifying failure)

Entries §10, §11 and §13 are the same mistake in different clothes, and a
fourth instance on 2026-07-25 makes the family explicit:

| # | what was consulted | what should have been consulted |
|---|---|---|
| §10 | "I niced that job" | `ps -o ni` on the actual process |
| §11 | "that job is a probe for the abandoned sweep" | `ps -o args` — it was the new plan's rung |
| §11b | "the watch will tell me when it ends" | whether the process still exists |
| §13 | "that census was wasted" | what the census actually measured (`r0`) |
| §14 | "b65 has 5 drops" *(my own summary)* | the derivation — the drops were `{13,26,30,39,52}`, not the 5-multiples I guessed |

The b65 case is the cleanest specimen because nothing was mistyped. A
summary table said "5 drops"; the drop *list* was never printed; the guess
`{5,10,15,20,25}` was plausible, self-consistent, and wrong — the forced set
takes the 13-multiples route. A probe then ran for minutes on a digit set
that does not exist.

**The rule:** a derived value must be read from the derivation, never
reconstructed from a summary of it — *especially* a summary you wrote
yourself, because it carries your own confidence without your own working.

### The architectural defence that caught it

The wrong digit set produced:

```
[certset] base=65: buildFeasiblePrefix found no feasible heuristic top prefix
  at W_terminal=22 -- INCONCLUSIVE (not a refutation of D)
```

**INCONCLUSIVE, not REFUTED.** The never-conflate-resource-with-mathematics
rule (§7) caught an error class it was never designed for: a nonexistent
digit set cannot produce a false negative through that path, because the path
has no way to express one. That is the strongest argument yet for keeping
resource outcomes and mathematical outcomes rigidly separate — **it defends
against mistakes nobody anticipated**, which is the only kind that matters.
