# b61: position-36 coverage — why the hit is maximal within the W=23 window

The 23 children are an **exact partition** of b61's W=23 terminal: the
descending top-35 prefix is fixed, and position 36 ranges over the 24
remaining digits. Each child is a W=22 terminal. Children are enumerated in
**strictly descending** position-36 order, so a hit at digit *d* means every
branch with a larger position-36 digit has already been refuted.

| child | pos36 | outcome |
|---|---:|---|
| *(pre-run)* | 24 | REFUTED — this is the W=22 descending terminal, wall 1414.970s |
| child1 | 23 | REFUTED |
| child2 | 22 | REFUTED |
| child3 | 21 | REFUTED |
| child4 | 20 | REFUTED |
| child5 | 19 | REFUTED |
| child6 | 18 | REFUTED |
| child7 | 17 | REFUTED |
| child8 | 16 | REFUTED |
| child9 | 15 | REFUTED |
| child10 | 14 | REFUTED |
| child11 | 13 | REFUTED |
| child12 | 12 | REFUTED |
| child13 | 11 | REFUTED |
| child14 | 10 | REFUTED |
| child15 | 9 | REFUTED |
| child16 | 8 | **FOUND** |
| child17 | 7 | REFUTED |
| child18 | 6 | REFUTED |
| *(not run)* | 5, 4, 3, 2, 1 | lex-smaller than the hit — cannot beat it |

**Coverage: position-36 digits 24 down to 6, contiguous, no gaps — true.**

The hit is at **pos36 = 8**. Every larger position-36 digit (24…9) was refuted
first, and the top-35 prefix is the lex-greatest 35-prefix available over the
forced set. So no arrangement inside the W=23 terminal exceeds this value, and
the five unrun children are all lex-smaller. **Stopping on the hit lost nothing.**

## Scope — UPGRADED 2026-07-26: the maximality is exhaustive, not window-bounded

The scope note here originally read *"maximum within the W=23 window, full
maximality open."* **That was too conservative**, and a cutoff-aware count run
the next day showed why — using no new compute at all.

Frame it at **W=22**, where `terminalPrefixLen = 59 − 23 = 36`, rather than as
a partition of the W=23 terminal:

1. Every arrangement has a 36-prefix, and all completions use all 59 digits, so
   **digit-lex order is numeric order**.
2. The incumbent's 36-prefix is **descending-top-35 + [8]**, and its first 35
   digits are the **lex-greatest 35-prefix available** — so nothing can deviate
   earlier and be larger.
3. Therefore the lex-greater 36-prefixes are exactly `top-35 + d` for pool
   digits `d > 8` — the pool after top-35 is `{24,…,1}`, so **exactly 16
   prefixes: d = 24 … 9.**
4. **All 16 already have definitive REFUTED records** at identical parameters
   (`W=22`, drops `{30}`, prefix length 36, verified against the ledger):
   `d=24` is the W=22 descending terminal (1414.970 s); `d=23…9` are children
   1–15. A W=22 terminal is exhaustive over the entire remaining pool, so each
   refutation means **no completion exists with that 36-prefix**.
5. The equal-prefix case is the search that found the value, exhaustive over
   its window.
6. Everything else is lex-smaller, hence numerically smaller.

> **No arrangement of the forced 59-set exceeds this value.** a(61)'s
> maximality is **exhaustive**, established entirely by work already done —
> **zero additional compute.**

This lifts b61 from *verified lower bound* to **STRONG — single-method
exhaustive**, the same class as b54/b59/b62/b64. It is **not CERTIFIED**: that
requires agreement from a second, structurally independent engine family,
which b61 has no more than the other eight STRONG rows do.
