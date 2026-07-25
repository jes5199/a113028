# LESSONS

General lessons from the A113028 work. **If it would still be true on a
different project, it belongs here. If it is about a flag, a file path or an
exit code, it belongs in
[CERTBB-OPERATIONAL-FOOTGUNS.md](CERTBB-OPERATIONAL-FOOTGUNS.md)**, which
holds the operational detail behind most of these.

Each entry: the lesson, the incident that produced it, and what it cost —
because the cost is the part that makes it stick. Entries are marked
**[MECHANISM]** where something now enforces them automatically, and
**[VIGILANCE]** where they still depend on somebody remembering.

**Provenance: twelve of the thirteen entries below come from a single day —
2026-07-25** — during which four first-ever values were computed and roughly a
dozen wrong beliefs were retracted. The errors belong to both the agent doing
the work and the agent reviewing it; they are attributed, because a lessons
file containing only one party's mistakes would be a dishonest document and a
less useful one.

That concentration is worth stating explicitly, because a reader three months
from now will otherwise assume these accumulated slowly. They did not. **The
rate is a property of how the work was being run, not of the difficulty of the
mathematics**: fast iteration, an adversarial review layer that re-derived
claims independently rather than accepting them, and a standing expectation
that any result could be retracted. Most of these errors were caught within
minutes or hours of being made, several by mechanisms built earlier the same
day. A slower or less contested process would not have produced fewer
mistakes — it would have produced the same mistakes, found later, in the
published record.

**How to read the tags.** Each entry is marked [MECHANISM] or [VIGILANCE].
The [VIGILANCE] entries are a **worklist, not a settled state**: if one of
them recurs, that recurrence is the signal to build the thing that retires it
(lesson 1, applied to this file).

---

## 1. A lesson noted twice without a mechanism is a missing mechanism

If you write a lesson down, and the same failure happens again, the problem is
not that you need to remember harder. **Build the thing that makes the failure
visible without anyone having to look.**

**Incident.** Four separate times in one day, a long-running job finished
quietly and its result went unread — twice the supervising agent noticed
before the agent running it did. The lesson had already been written down
twice (a "watch for completion, not just failure" note, then an "absence is
not evidence" note) and behaviour had not changed.

**What it cost.** Two cores idle for ~25 minutes on one occasion; a completed
b89 result unread; repeated wrong guesses about what the box was doing.

**The fix, and what happened next.** Two small mechanisms were built:
`runlog.sh`, which brackets every run with `START`/`END` records in a ledger
so a dead run leaves a permanent open entry; and a **width guard** that
refuses to start a run whose configuration cannot possibly produce a result.

- The ledger answered the next vanished-process question in one line —
  `runs started=10 ended=10 UNFINISHED=0` — after the same question had been
  guessed wrong twice.
- The width guard, **within ninety seconds of existing**, retracted a claim
  that had already been committed to the repository as a finding — **and not
  the error it was built to catch.**
- Later the same day a `ps`-based status filter went stale (it was keyed to a
  version-numbered binary that had since been rebuilt) and reported a running
  job as absent. The ledger showed it correctly: **it was the only monitoring
  surface that had not silently narrowed**, because it records what was
  launched rather than pattern-matching the present.

> Every mechanism built that day found a real error within minutes. No amount
> of additional documentation had.

**[MECHANISM]** — this lesson is now self-enforcing: it generates the others'
fixes.

---

## 2. A run configured from a belief cannot test that belief

If a belief determines how a run is set up, its outcome cannot be evidence for
that belief — it can only echo it. The output looks exactly like confirmation.

**Incidents (three, in one day, on three different objects):**

1. A base believed probeable at width 22 was probed at width 22 and returned
   `49/49 DECLINED`. The declines were caused by the wrong width, not by the
   mathematics.
2. Two bases believed **UNREACHABLE** — from a ceiling that had been wrongly
   generalised — were run **at a width already derived to be below their own
   minimums**. They declined, and the declines were recorded as *"UNREACHABLE
   confirmed by run"*.
3. That false confirmation was then **scored by the reviewing agent
   (boss-clod) as the arithmetic ledger "succeeding again"**, and relayed
   onward as verified.

**What it cost.** A wrong finding published in the repository for roughly
three hours, a headline count wrong by two bases, and a ledger entry counted
as evidence for the premise it was derived from.

**The question that exposes it:** *what did I assume in order to configure
this run, and could the run have contradicted that assumption?*

**[MECHANISM]** — when a parameter is derived, the program now **refuses**
configurations inconsistent with the derivation rather than executing them. A
refusal is honest; a decline looks like a result.

---

## 3. Verification must not share a premise with what it verifies

Two people computing the same quantity and agreeing proves nothing if both
computed it from the same understanding. That is one computation run twice.

**Incident.** A constant was derived as *"minimal `t ≥ 0` such that …"*, which
was true as stated. The reviewing agent **independently verified it and got
the same answer** — by re-deriving from the same shared misunderstanding. The
implementation actually started its loop at 1, and had been printing the
correct value in its own logs all along.

**What it cost.** Nothing, by luck — the affected quantity was
soundness-neutral. Had it been accepted, a **correct document would have been
"fixed" into a wrong one**. A neighbouring quantity would have flipped a
classification.

**The question that exposes it:** *did my check go back to the source — the
code, the artefact, the raw value — or to my own restatement of how it is
defined?*

**[VIGILANCE]** — the working rule is: verify values by re-deriving them from
the artefact itself, verify constants against the implementation or its
output, and treat *"I recomputed it and agree"* as a consistency check, never
an independent one.

---

## 4. Validation drawn from one regime cannot detect regime-dependence

Confirmations only count if the sample **could have disconfirmed**.

**Incident.** A counting formula was adopted after matching exactly on three
test cases. All three sat in a regime where the quantity it predicted and the
quantity actually needed **coincide by construction**, so the sample was
structurally incapable of revealing the difference. The disconfirming case was
not merely available — it was *adjacent*, printed side by side in a table that
had been read aloud and quoted from repeatedly.

**What it cost.** A cost model that was wrong by up to 28× on the bases where
it mattered most; several hours of planning built on it.

**The question that exposes it:** *what regimes does my sample span, and could
any member of it have failed?*

**[VIGILANCE]** — where a predictor's confirmations all come from one regime,
record it as **"unearned outside that regime"** rather than as a hit count,
and state what evidence would move it.

---

## 5. Never conflate a resource outcome with a mathematical one

"We ran out of budget", "the configuration was invalid" and "no solution
exists" must be different, visibly, at every layer. This is the single
highest-value rule in the project.

**Incident.** A planner change mapped "the optimiser declined" onto "this
branch is refuted". A base that was **certified** silently became
**incomplete**: 25 branches that the previous code refuted outright turned
into 25 unfinished ones, at identical runtime, with nothing gained.

**What it cost.** Nothing — *because the rule held one level down.* Since
declines are never folded into refutations, the run reported an honest
`INCOMPLETE` with a lower bound instead of a confident, wrong `CERTIFIED`. The
regression was visible in the disposition mix and was caught before it
shipped.

**The part worth keeping.** Later the same day, a completely different
mistake — running a search against a digit set that does not exist — produced
`INCONCLUSIVE` rather than a refutation, **through a path never designed for
that error at all**. A nonexistent configuration had no way to express a false
negative.

> The rule does not prevent bugs. It converts invisible ones into visible
> ones, including bugs nobody anticipated — which is the only kind that
> matters.

**[MECHANISM]** — separate dispositions and distinct exit codes for *no
result* (4), *out of budget* (3/124) and *invalid configuration* (8).

---

## 6. An optimiser's failure must cost performance, never capability

When a heuristic — a planner, a cache, a cost model, an index — sits in front
of an exhaustive search, its failure path must fall back to the thing that
always works. "Give up on this branch" turns an optimisation into a capability
limit.

**Incident.** A planner was routed in front of the terminal search. When it
declined, the terminal declined. See lesson 5 for the damage.

**What it cost.** Caught pre-commit by an A/B against the engine it replaced.
Had it shipped, two certified bases would have quietly dropped out of the
certified set on their next re-run.

**[MECHANISM]** — the planner path now falls back to the previous split, so it
can never do *less* work than the code it replaced, only the same or better.

---

## 7. Absence is not evidence of failure — it is evidence of absence

A process that is gone has either finished or died. From outside they are
identical: no error, no marker, just a gap. The failure is not noticing the
gap; it is **naming** it before reading the log.

**Incident.** A sweep driver was found missing with cores idle and was
reported by the reviewing agent as *"the driver died and took the remaining
work with it"* — accompanied by advice to add per-item logging, because *"a
dead driver looks exactly like a completed one."* The logs already said
`SWEEP HALF COMPLETE`. It had finished its entire list normally. **The
diagnosis was the exact mirror of the principle stated in the same message.**

**What it cost.** A wrong status relayed upward; a brief scramble to
re-establish what had actually run.

**The deeper point:** the logging convention did not prevent the misdiagnosis
— **the log already said COMPLETE and nobody read it.** Instrumentation only
helps if absence triggers *reading* rather than *inference*.

**[MECHANISM]** — the run ledger makes "started but never ended" a queryable
state rather than an inference from `ps`.

---

## 8. Acting on your model of a thing instead of the thing

The recurring root of most operational errors here. In every case something
authoritative was available and something remembered was consulted instead.

| what was consulted | what should have been |
|---|---|
| "I niced that job" | the process's actual nice value |
| "that job is a probe for the abandoned plan" | its actual command line — it was the *new* plan's first step |
| "the watch will tell me when it ends" | whether the process still exists |
| "that measurement was wasted" | what the measurement actually contained |
| "this base has 5 drops" *(own summary)* | the derivation — the drops were entirely different digits |
| "the engine's ceiling is 24" *(from one base's log)* | the source — the ceiling is per-base |

**What it cost.** A probe run for minutes against a digit set that does not
exist; a published finding retracted; a job nearly killed and relaunched from
zero that was already doing exactly the wanted work.

**The rule:** a derived value must be read from the derivation, never
reconstructed from a summary of it — **especially your own summary, because it
carries your confidence without your working.**

**[VIGILANCE]** — partially mechanised: the width guard and the run ledger
each remove one instance.

---

## 9. A destructive operation must name its target exactly

The cost of a wrong match is unbounded, so pattern-matching is never
acceptable for anything irreversible. Resolve to explicit identifiers, verify
each one against something authoritative, act, then confirm the thing you were
protecting is still there.

**Incident.** A `pkill -f <pattern>` aimed at some dead watcher loops matched
the very shell issuing it — **twice in one session.** Harmless both times.

**What it cost.** Nothing. But the same box runs a live trading process, and
the same carelessness pointed at a busier pattern is how an unrelated
production service dies to a maintenance command.

**[VIGILANCE]** — the working form is: resolve to explicit PIDs, check each
process's identity, signal, then re-verify the protected process is alive.

---

## 10. Structure derives; cost does not

On this problem every quantity derivable from the problem statement has held,
and every quantity extrapolated from observed behaviour has failed.

**Incident.** Eight derived predictors held across dozens of cases. Seven cost
predictions failed — every one of them crossing a boundary: one base's
behaviour applied to another, one width's ratio applied to the next,
descending prefixes applied to non-descending ones.

**The refinement that makes it usable.** "All cost prediction fails" was too
strong. A projection *within* an identical configuration, from a measured
sample of the very population being projected over, was accurate to **5.5%**
over 2,081 cases.

> **Cost is unpredictable across configurations and predictable within one,
> once measured.** A projection is legitimate only when the measurement and
> the prediction share every parameter.

**The distinction, stated exactly.** Every one of the seven failures was a
**cross-configuration extrapolation** — a different base, a different width,
a different mode. The licensed case is narrower and rarer: **sampling from the
very population you are projecting over.**

The clearest instance came late the same day. A single long computation was
decomposed into 23 independent children — same base, same width, same prefix
class, one exact partition. Three of the 23 completed:

```
1635.249 s   1632.779 s   1639.988 s
mean 1636.0 s, spread 7.2 s = 0.44 %
```

Projecting the remaining 20 from those three is **interpolation, not
forecasting**: the sample and the target are members of one population that
differs in no parameter. That projection was trusted, and it is the only
runtime claim made all day that carried no hedge.

The practical test: *is my sample drawn from the same population as my target,
or merely from something that resembles it?* Resemblance has failed seven
times out of seven.

**What it cost.** A budget mis-estimated by 7×, and one width-ladder cap set
so low that a resource timeout would have been indistinguishable from a
mathematical refutation.

**[VIGILANCE]** — caps are **insurance, not forecasts**: size them
pessimistically, because an idle core costs nothing and a truncated run costs
the whole result.

---

## 11. Write facts continuously; withhold judgement until the evidence is in

**The results most at risk of being lost are the ones that still feel
unfinished** — because unfinished is exactly what makes you defer recording
them.

**Incident.** In one afternoon, seven short lessons reached the repository
within minutes each, because they felt like conclusions. Meanwhile a 25-base
classification, a derived engine limit and 17 verdicts with their timings sat
in a scratch directory and a chat log for hours — until someone outside the
work asked whether the repo was up to date. Three processes had exited
silently that same day.

**What it cost.** Nothing, narrowly. A session restart would have destroyed
several hours of measurements that existed nowhere else.

**The conflation to avoid:** the deferral came from waiting to know what the
results *meant* before recording what they *were*. Those are separable acts.

**Practical trigger:** when a campaign produces a **table**, that table is
already the durable artefact. Commit it, marked in progress, before knowing
what it shows.

**[VIGILANCE]**

---

## 12. State the denominator, and keep outcome classes separate

A count is meaningless without the population it is drawn from, and merging
outcome classes manufactures claims nobody made.

**Incident.** A sweep produced 17 refutations across a 25-base range. Reporting
"17 of 25" would have been wrong in **both directions at once** —
understating coverage, because six of those bases were never attemptable at
the width used, and overstating the negatives, because two were resource
timeouts and not refutations at all. The honest statement was **17 of the 19
bases attemptable at that width**.

**What it cost.** Nothing — caught before publication. Had it shipped, it
would have entered the record as a fact about the mathematics rather than
about our engine.

> The worst available error in this project is reporting *"the instrument
> could not be pointed at this case"* as *"we looked and found nothing."*

**[VIGILANCE]** — four classes are maintained explicitly: real negative,
out-of-budget, invalid-configuration, and out-of-reach.

---

## 13. Validate a new instrument against a known answer before trusting it

A discovery tool that never finds anything is indistinguishable from a broken
one. An empty result from an unvalidated searcher carries **no information at
all**.

**Incident.** Before pointing a new search mode at an unsolved base, it was
run against one whose answer was already known; it reproduced that answer
exactly, in 18 seconds. Later, a new primitive for running a computation at a
caller-specified starting point was gated the same way: pointed at a known
result, it reproduced it character-for-character with identical internal
counters.

**What it cost.** Minutes. Both instruments then produced genuine results on
unsolved cases — and one of them found a previously unknown answer.

**The corollary:** the validation must happen *before* the unknown run, not
after a disappointing one.

**[VIGILANCE]** — but cheap, and it has never not been worth it.

---

*All incidents recorded here occurred on 2026-07-25 unless stated otherwise.
Entries are added when a lesson is learned, not when it is resolved — a
[VIGILANCE] tag means the failure can still happen.*
