# Development setup and repository conventions

## Setup after cloning

```sh
git config core.hooksPath .githooks
```

**Required, and not optional.** `core.hooksPath` is **per-clone configuration,
not repository state** — the hook file is committed, but a fresh clone will not
run it until this is set. Without it, `.githooks/pre-commit` silently does
nothing.

The hook enforces two things:

1. **No staged files ≥ 20 MB.** The repository already contains one 97 MB raw
   trace log that was committed because nothing checked, and a 585 MB one was
   stopped only by GitHub's 100 MB limit *after* a push had already failed.
   Raw planner traces are ~99.99 % `[bucket-plan]` lines around a handful of
   signal lines: **distil at write time, commit the summary, never the
   trace.** Deliberate override: `GIT_ALLOW_LARGE=1`.
2. **Ladder consistency** (`scripts/check_ladder_consistency.py`): the README
   results table and [../results/FRONTIER-STATUS.md](../results/FRONTIER-STATUS.md)
   must agree on every base's evidence status. Built after the same failure
   three times in two days: a correction reached the rows it was about and
   missed the neighbours (LESSONS 13d). Runs automatically when either file
   is staged. Deliberate override: `GIT_ALLOW_LADDER_SKEW=1`.

`scripts/validate_bfile.py` machine-checks `b113028.txt` against the README
table (alphabet decode, distinct-nonzero-digit and lcm-divisibility checks).
Run it from the repo root after touching either.

## Build & run

```sh
gcc -O2 -march=native -o a113028_v8 a113028_v8.c
./a113028_v8 [lo] [hi] [verbose]     # e.g. ./a113028_v8 2 48 1
```

`a113028.c` is the original two-engine solver (Engine S multiple-scan +
Engine D DFS with budget doubling); `a113028_v3..v15.c` are the successive
Engine C generations — see [../engine/NOTES.md](../engine/NOTES.md) and
[../engine/ALGORITHMS.md](../engine/ALGORITHMS.md) for the design history.
`carrytrie.cpp` is the production certification engine (bucket joins,
certauto/certset/certdisc/certbb modes); its operational lore is in
[CERTBB-OPERATIONAL-FOOTGUNS.md](CERTBB-OPERATIONAL-FOOTGUNS.md).

## Method notes

[LESSONS.md](LESSONS.md) collects the general lessons from this work — what
makes a confirmation real evidence, when a projection is legitimate, why
resource outcomes and mathematical outcomes must never be merged.
[CERTBB-OPERATIONAL-FOOTGUNS.md](CERTBB-OPERATIONAL-FOOTGUNS.md) keeps the
engine-specific detail behind them.
