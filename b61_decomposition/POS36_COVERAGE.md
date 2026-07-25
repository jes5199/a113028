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

> **Scope.** Each REFUTED carries the engine's own caveat that only *that
> prefix's window* is exhausted. The children partition the **W=23 terminal**,
> not the whole arrangement space. Established: **a(61) ≥ this value, and it is
> the maximum within the W=23 window.** Full maximality remains open.
