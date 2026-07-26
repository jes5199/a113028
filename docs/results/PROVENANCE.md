# Provenance and prior work

## OEIS lineage

Pulled from the live OEIS record 2026-07-26
(`oeis.org/search?q=id:A113028&fmt=text`):

- **Origin:** the puzzle *Enigma 1343: Digital Dividend*, New Scientist,
  4 June 2005, p. 28.
- **Author:** Peter Boothe, submitted 3 January 2006.
- **Extensions:** a(11)–a(13) Francis Carr, 8 Feb 2006 · a(14) Michael S.
  Branicky, 17 Jan 2022 · a(15)–a(17) Branicky, 20 Jan 2022 ·
  **a(18)–a(21) Jes Wolfe, 26 Apr 2024**.
- **The published b-file (n = 2..48) is Jes Wolfe's**, as is the entry's
  "even faster Python program" link. Entry last edited 8 May 2024. The
  inline terms stop at a(21); the b-file carries 22–48.

**The public record understates the continuity.** jes worked on this
problem **with Peter Boothe in 2006**, at the time of the original
submission (stated by jes, 2026-07-26; none of this is in the OEIS entry).
So the story is not a stranger picking up an abandoned sequence eighteen
years later — it is a problem worked on with its original author, left at
base 21 when the first effort stopped in February 2006, and carried to
base 64 with maximality proven rather than assumed.

> **PLACEHOLDER — awaiting jes.** How the 2006 work was divided, whether
> Peter Boothe should be told or credited beyond his authorship line, and
> whether the 2020 Ruby return was solo or joint are facts only jes has.
> Do not fill this in by inference, and draft nothing for the OEIS
> submission's comments field until he decides.

The corrected a(46) is in any case an **erratum against the author's own
2024 b-file** — a correction of the record, not a dispute with anyone.

## Independent 2020 cross-check (bases 2–39)

An older repository by the same author,
[`jes5199/a113028-2020`](https://github.com/jes5199/a113028-2020)
(December 2020, Ruby: `solve.rb` + `exhaustive.sh`), computed the sequence
through base 39. Its `solutions.out.txt` is vendored at
[`../../evidence/a113028-2020/solutions.out.txt`](../../evidence/a113028-2020/solutions.out.txt)
(fetched from GitHub `master`, 2026-07-26) so this claim stays reproducible.

**Comparison method** (performed independently twice — two sessions, two
parsers — on 2026-07-26, not assumed): parse both of the file's formats
(quoted base-36 digit strings for bases ≤ 36, bracketed digit lists above);
independently re-decode every digit sequence and require it to equal the
file's own decimal (self-consistency); check digits distinct, nonzero,
< B, and lcm(digits) | N for every term; then diff the 38 decimals against
`b113028.txt`.

**Result: 38/38 exact agreement, bases 2–39. Zero disagreements.**

**What this is:** corroboration from a genuinely independent engine family —
a 2020 Ruby brute-forcer and the 2026 C/C++ engines share no code and are
six years apart.

**What this is not:** it reaches only base 39, so it does not touch the
bases where the STRONG/CERTIFIED distinction bites, and bases 2–39 were
never in doubt. It certifies nothing currently labelled STRONG; it is
recorded because independent-family concordance is exactly the evidence
class the project's ladder values, and it previously went unmentioned.
