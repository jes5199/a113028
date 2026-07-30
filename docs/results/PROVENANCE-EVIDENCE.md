# Provenance evidence record — the 2006/2024 investigation (2026-07-26)

**Status: verified working material for the pass-2 provenance rewrite.**
Everything here was established with at least two independent
derivations on 2026-07-26 (this session + boss-clod, each checking the
other; four claims were overturned in the process and are recorded
below as overturned). Artifacts: [`evidence/2006-era/`](../../evidence/2006-era/SOURCE.md).
The polished narrative belongs in [PROVENANCE.md](PROVENANCE.md); this
file is the evidence behind it and should survive that rewrite.

## The three eras (attribution rules — use these everywhere)

There is no single "2006 program." Attribute precisely:

- **February 2006 — Ruby** (the run log, its timings, YARV, the
  mid-evening "Seamus inversion"): one evening, bases 2→36, ~7.5 h.
  Solved b6.
- **April 2006 — C rewrite** (`finite.c`, GMP): faster, better
  structured, **regressed** — lost the digit-set fallback and fails b6
  (picks the unpermutable {5,4,1}; the true a(6) = 412₆ = 152 uses the
  next set {4,2,1}). b6 is the **unique** base in 2..64 where
  no-fallback fires. The April email's values (through b40): producing
  program **unknown — stated as unknown, never guessed**.
- **April 2024 — Python** (OEIS "even faster python program"):
  **regained the fallback**, weakened the Nine Rule (fires only when
  digit B−1 present), added a "Three Rule" that is provably subsumed by
  2006's `sum mod gcd(B−1, lcm)` (d | lcm and B ≡ 1 mod d ⇒
  d | gcd(B−1, lcm)), and regressed the search to naive `n -= lcm`
  single-stepping. Its k-major descending-lex complete-subset
  enumeration is the direct ancestor of the modern engine's OLD-mode
  enumerator — the one that churned at b60.

**The capability wandered: each rewrite lost something the previous one
had, and no version had everything until 2026.**

## Selection was never the gap (measured, reproducible)

`evidence/2006-era/make_subset_2006_port.py` (self-gating: must
reproduce the February log's own b36 answer set before making any other
claim) shows the April C's digit-set selection picks the **exact true
answer set at all 29 solved bases 36–64** — including b63's seven drops
and b64's full alphabet. Worst case b60: 1,460,568 pruned steps.

Three-way enumerator comparison on b60's selection:

| enumerator | work to reach the true 47-set |
|---|---:|
| 2026 family-cover (`SUBSETENUM=new`) | **21 subsets emitted** |
| 2006 prefix-pruned DFS (`make_subset`) | 1,460,568 steps |
| modern OLD-mode (default; complete C(n,k) subsets, filtered) | ~6×10¹⁰ churned, NO-VALUE at 90 min |

So the enumerator **regressed between April 2006 and the churn fix**
(2024's simplification, inherited), and the 2026 family-cover fix is
strictly stronger than both. Note the modern default is still
`SUBSETENUM=old`; flipping it is a jes decision with two stated
preconditions (correctness argument at `carrytrie.cpp:4801` + the A_p
theorem at `:4571`; regression suite green with `new` as default — the
suite run is still owed).

## Filter equivalences (each proved, not assumed)

- 2006's nine rule `sum mod gcd(B−1, lcm)` is **CRT-equivalent** to the
  modern per-prime-power digit-sum gates (`carrytrie.cpp:4512`). Two
  earlier "coarser/weaker" claims (one from each reviewer) were **wrong
  and are withdrawn**.
- 2006's `five = base/2` survives, generalized, as **`SB_D1`**
  (`carrytrie.cpp:4164` derivation, `:4515` set-level gate): D1 = 5 at
  B = 10, D1 = 32 at B = 64. The b64 ends-in-32 capstone is the same
  orbit shape (`1, 0, 0, …`) as the 2006 variable — see
  [../theory/ORBITS.md](../theory/ORBITS.md).
- 2024's Three Rule: strictly weaker than 2006's form in general (a
  binding prime power assembled from several digits — e.g. mod 8 via
  digit 24 at B=33 — is invisible to per-digit tests).
- The eleven rule: **jes knew it in 2006 and could not implement it.**
  `carrytrie.cpp` contains no named eleven rule because the general
  `Bⁱ mod q` positional treatment subsumes order 2 with the nameless
  orders 3+. b6's obstruction (q=4, 6 ≡ 2 mod 4) sits in the gap
  between the two named rules. Framing rule: *he had located the right
  frontier and named the right rule* — never "he didn't know about
  arrangement."

## The a(46) question (narrowed, still open)

Published and corrected a(46) use the **identical digit set** (both
drop {22,23}; first divergence at digit 17 of 43). Therefore:

- 2024's break-on-first-completing-subset issue is **not** the
  mechanism (proposed by boss-clod, killed by this check) — it remains
  theoretical, zero known bites;
- the 2024 program is doubly exonerated: its downward lcm-scan's first
  hit at b46 **is the corrected value** (5.566×10¹¹ steps — feasible in
  C, weeks in CPython; an earlier "computationally infeasible" claim by
  this session was **wrong and is withdrawn**);
- so the published a(46)'s producer initialized below the corrected
  value, skipped a region, or searched upward stopping on first-found.
  **Which program was pointed at b46 for the 2024 b-file is an open
  question only jes can answer.**

## Where the April 2006 C stops on modern hardware (bound, not projection)

Run verbatim on this box (range-patched copy, single core, nice −19),
values char-exact against `b113028.txt` everywhere it completed:

| base | wall | iterations |
|---:|---:|---:|
| 2–36 | < 1 min total | (the 2006 evening took 7.5 h) |
| 37 | 44 s | 12,866,520 |
| 38 | 179 s | 52,379,727 |
| 39 | 112 s | 33,432,429 |
| 40 | 43 s | 12,587,009 |
| 41 | 85 s | 22,594,994 |
| 42 | 188 s | 55,028,681 |
| 43 | **> 420 s** | > ~1.2×10⁸ |
| 44 | **> 420 s** | — |

Rate: 292,000 ± 300 iterations/s at five of six bases (b41: 266k,
unexplained — recorded rather than dropped). **There is no wall, only
an exponent**: roughly one doubling per base, irregular; the program
never becomes wrong, only slow, and selection is never the limiter.

Iteration calibration against the February log: the C matches the
post-upgrade February algorithm to ±2 iterations on 10 of 11
post-upgrade bases; the pre-upgrade rows differ because the log records
its own mid-evening algorithm change; the b32 outlier (C 3.4× fewer) is
the `five`-reservation on the one base whose answer ends in the
reserved digit. Every deviation independently accounted for — which is
what a genuine reconstruction looks like.

**Consistency, not proof** (state it as agreement among independent
things): b40 is anomalously cheap in this curve, the April 2006 email
has b40 in hand, and b48 — which that email calls unsolved — lies far
beyond the b43 bound. The b40-is-easy anomaly spans twenty years and
three engine families (modern canonical best 5.81 s); **why b40 is easy
is an open question** — if it has a structural reason it belongs in a
theory doc, and if it has none, saying so is worth a line.

## Open items this file deliberately does not resolve

Only jes: whether Peter Boothe is told directly; the OEIS comments
field; whether the 2020 Ruby return was solo; which program produced
the published b-file terms 40–48 (the a(46) erratum hangs off this);
the `SUBSETENUM` default flip. Open technical: why b40 is easy; whether
any answer can omit digit B−1 (no known one does; April 2006 pinned it
by assumption at `finite.c:118`, unproven).
