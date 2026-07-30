# Resume note — A113028 worker wind-down, 2026-07-30

*(Written cold-start-first: if you are a fresh session picking this up,
read this file, then `docs/results/PROVENANCE-EVIDENCE.md`, then the
memory directory's `a113028-project-state.md`. Delete this file when
pass 2 lands.)*

## Where things stand

**The mathematics is closed through base 64** — every base 2–64 STRONG
or CERTIFIED, bases 65–89 swept with zero completions (b74/b82
budget-bounded, b86 structurally out of reach), published a(46)
corrected. Canonical: `README.md` (results list),
`docs/results/FRONTIER-STATUS.md` (per-base ledger), `b113028.txt`
(n=2..64, machine-checked by `scripts/validate_bfile.py`).

**Pass 1 of jes's reorg+README mandate is pushed and verified**
(commits `62897c1..e2ca8d2`, 2026-07-26): stale-prose retractions,
docs/ reorg, README cut 630→209 lines, b-file, provenance. Two
authorized additive commits followed (`docs/theory/ORBITS.md` — jes-
requested teaching prose, **do not compress it into a reference table;
its closing line stays**).

**The repo was frozen at `e2ca8d2` for an external critique by Sol Max
that, as of 2026-07-30, never arrived.** The workflow was: my pass →
jes routes to Sol Max → outsider critique → next pass. Whether that
critique is still coming is jes's to say.

## Pass-2 queue (all verified, none applied — repo was frozen)

1. **Soften the README's a(46) bullet**: the published value is
   *valid, just not maximal* ("a larger valid answer exists", never
   "was wrong") — and use that wording in any OEIS erratum.
2. **LESSONS additions**: (a) 13d addendum — search first where a claim
   is *most prominently asserted*, not where you're working (both
   correctors missed the README top bullet); (b) the self-correction
   pattern — removing your own supporting argument can strengthen the
   conclusion (the a(46)-infeasibility case).
3. **Provenance rewrite** of `docs/results/PROVENANCE.md` from
   `docs/results/PROVENANCE-EVIDENCE.md` (the verified dossier) and
   `evidence/2006-era/` (vendored artifacts + self-gating port —
   run `python3 evidence/2006-era/make_subset_2006_port.py b113028.txt`,
   must print 29/29 MATCH). Three-era arc, attribution rules, the
   capability-wandered story, the b16 10.14s-vs-0.021s pairing next to
   the README's a(16) opener.
4. **Theory-doc line**: can any answer omit digit B−1? (2006 pinned it
   by assumption; no known answer violates it; nobody has shown it.)
5. **Capstone lineage** in the a(64) material: finite.c:147 →
   carrytrie.cpp:4191 (same `// ten rule` comment, twenty years), and
   the three-step heuristic→proof arc (`five` → odd/even guard → Sol
   Max's 2-adic argument).

## Decisions that are jes's alone (placeholders exist; do not draft)

Boothe contact · OEIS comments field · 2020 solo-or-joint · which
program produced the published b-file terms 40–48 (the a(46) erratum
question) · flipping `SUBSETENUM` default to `new` (correctness
argument done; **regression suite with new-as-default still unrun** —
run it capped, off market hours, before any flip).

## Open technical questions (recorded, unresolved)

Why is b40 easy? (20-year, 3-engine anomaly — structural reason or
honest "none found".) · B−1 omission (above) · v2 candidates:
proof-producing SAT (DRAT/LRAT) as an independent engine family
(`docs/reviews/SHOULD-THE-PROJECT-STOP-AT-BASE-64.md` §SAT), bignum
epic for bases ≥ 90 (finite.c is the existence proof the algorithm
tolerates arbitrary precision).

## Working-tree audit at wind-down (2026-07-30)

The untracked working tree was classified file-by-file before exit:

- **Committed as evidence** (wind-down commit 2): the b63 shard
  manifests — final state; the loose shard2 has 3 records *more* than
  the timestamped checkpoint, whose copy is an exact byte-prefix of it
  — plus the hashed checkpoint bundle (its SHA256SUMS re-verified OK
  at commit time), `merged_60.jsonl`, the b60 pre-merge backup,
  `scripts/fit_planner.py` (source of the planner-calibration numbers
  in FRONTIER-STATUS), and `sol-reference/` (contains the
  proof-bearing nilpeel certificate verifiers AND a defective
  reference engine — read its README before touching).
- **Gitignored as regenerable**: `regression_logs/` (38 MB raw gate
  logs; every outcome is summarized in-repo; regenerate via
  `scripts/regression_suite.sh`), and the v10–v15-era gate/run outputs
  (`gate_*`, `longrun*`, `v1?_*`, `g?_*` etc.). Nothing in them exists
  only there.
- **Uncertain, labeled as such**: the ELF binaries `a113028_v13b` /
  `a113028_v13c` are interim "fixed v13" builds; the fixes are
  recorded as landed in the tracked v13/v14 sources, but these exact
  binaries were never byte-matched to a tracked source. They are
  gitignored, still on disk; if bit-exact reproduction of the 07-23
  long runs ever matters, that's the gap.

## Box constraints (verify, don't assume — they were true at wind-down)

Hermes (live-money) shares these four cores — market hours are
sacrosanct, solver runs `nice -n 15+`, detached, capped. Agents are the
*preferred* OOM victims by design (adj +350) — `ulimit -v` big jobs.
The pre-commit hook needs `git config core.hooksPath .githooks`
per-clone; the ladder checker guards README ↔ FRONTIER-STATUS (the only
two status-bearing tables — keep it that way).
