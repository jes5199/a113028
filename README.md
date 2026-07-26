# A113028 — solved through base 64

[OEIS A113028](https://oeis.org/A113028): **a(B) is the largest number whose
base-B representation uses distinct nonzero digits and is divisible by every
digit it contains** — equivalently, divisible by the lcm of its digits.

The flavor of the problem in one value: **a(16) = `0xFEDCB59726A1348`** —
fifteen distinct nonzero hex digits whose lcm, 360360, divides the value
exactly. It is the one answer below you can check by eye, and it was first
found in 2006, unsubmitted, while the sequence's author was advising this
repository's author ([Provenance](#provenance)). Today the engine returns
it in 0.021 s — and a(64), the deliberate stopping point, is ~95 orders of
magnitude larger.

This repository computes and proves **a(B) for every base 2 ≤ B ≤ 64**:

- **Every value below carries a maximality proof** — no conjectured or
  window-bounded entries remain. Labels: **CERTIFIED** (independent methods
  concordant) or **STRONG** (single-method exhaustive); definitions below.
- **Bases 49–64 are first-ever computed values.** The published OEIS b-file
  ends at base 48.
- **The published a(46) is wrong** — this project found a strictly larger
  arrangement of the same digit set. Corrected below.
- **Bases 65–89 were swept and no completion was found** — 22 bases refuted
  at their minimum widths, two bounded by budget, one structurally out of
  reach. Not a proof that none exist; see
  [docs/results/HIGH-BASE-RANGE-65-89.md](docs/results/HIGH-BASE-RANGE-65-89.md).
- Decimal expansions for all 63 terms: **[b113028.txt](b113028.txt)** (OEIS
  b-file format, n = 2…64), machine-checked against this table by
  `scripts/validate_bfile.py`.

## Reading the values

Digit values map to symbols (full spec, including the right-to-left
handling of Hebrew digits, in
[docs/results/DIGIT-ALPHABET.md](docs/results/DIGIT-ALPHABET.md)):

| values | symbols |
|--------|---------|
| 0–9 | `0`–`9` |
| 10–35 | `A`–`Z` |
| 36–48 | Greek lowercase `α β γ δ ε ζ η θ ι κ λ μ ν` (α=36 … ν=48) |
| 49–70 | Hebrew `א‎ ב‎ ג‎ ד‎ ה‎ ו‎ ז‎ ח‎ ט‎ י‎ כ‎ ל‎ מ‎ נ‎ ס‎ ע‎ פ‎ צ‎ ק‎ ר‎ ש‎ ת‎` (א=49 … ת=70) |

Most significant digit first; every Hebrew letter is followed by an
invisible left-to-right mark so the string renders in place-value order.

## Results

| base | a(base), base-B digits | status |
|-----:|------------------------|--------|
| 2 | `1` | verified — matches OEIS |
| 3 | `2` | verified — matches OEIS |
| 4 | `312` | verified — matches OEIS |
| 5 | `413` | verified — matches OEIS |
| 6 | `412` | verified — matches OEIS |
| 7 | `65142` | verified — matches OEIS |
| 8 | `7625134` | verified — matches OEIS |
| 9 | `8271536` | verified — matches OEIS |
| 10 | `9867312` | verified — matches OEIS |
| 11 | `A98762413` | verified — matches OEIS |
| 12 | `B9352176` | verified — matches OEIS |
| 13 | `CBA95847213` | verified — matches OEIS |
| 14 | `DCBA8513492` | verified — matches OEIS |
| 15 | `EDCB8219473` | verified — matches OEIS |
| 16 | `FEDCB59726A1348` | verified — matches OEIS |
| 17 | `GFEDCB93652741A` | verified — matches OEIS |
| 18 | `HGFEDCAB2514376` | verified — matches OEIS |
| 19 | `IHGFEDCB2671A3854` | verified — matches OEIS |
| 20 | `JIHGE9137B264DC` | verified — matches OEIS |
| 21 | `KJIHGFDBC286A4153` | verified — matches OEIS |
| 22 | `LKJIHGFED981C456732` | verified — matches OEIS |
| 23 | `MLKJIHGFEDC87521A6943` | verified — matches OEIS |
| 24 | `NLKJIHFEA679541B32DC` | verified — matches OEIS |
| 25 | `ONMLKJIHGFDB51284E3976A` | verified — matches OEIS |
| 26 | `PONMLKJIHGFB97461E325A8` | verified — matches OEIS |
| 27 | `QPONMLKJIHGFC6B72A85E3149` | verified — matches OEIS |
| 28 | `RQPONMKJIHF1352B69A8GD4` | verified — matches OEIS |
| 29 | `SRQPONMLKJIHGFDC2619485BA37` | verified — matches OEIS |
| 30 | `TSRQONMLJI2B1E4H8G397D6` | verified — matches OEIS |
| 31 | `UTSRQPONMLKJIHGE89A265D41BC37` | verified — matches OEIS |
| 32 | `VUTSRQPONMLKJIHF1758A9BC324E6DG` | verified — matches OEIS |
| 33 | `WVUTSRQPONLKJIHG7C813D59AE426` | verified — matches OEIS |
| 34 | `XWVUTSRQPONMLKJIEB72963C458F1DA` | verified — matches OEIS |
| 35 | `YXWVUTRQPONMKJIBCG16H59328D4A` | verified — matches OEIS |
| 36 | `ZYXWVUTSQPONMLKJF586A4E2B13D7HC` | verified — matches OEIS |
| 37 | `αZYXWVUTSRQPONMLKJHDB7A3G562E8F1C49` | verified — matches OEIS |
| 38 | `βαZYXWVUTSRQPONMLKGFDE2986BH3C4157A` | verified — matches OEIS |
| 39 | `γβαZYXWVUTSRPONMLKJ54H31E72B9FAC8G6` | verified — matches OEIS |
| 40 | `δγαZYXVUTSRQPNMLJIFEC6AB13D9254H7K` | verified — matches OEIS; divergence flag cleared by direct solve |
| 41 | `εδγβαZYXWVUTSRQPONMLJIFGC8574AED6H1932B` | verified — matches OEIS |
| 42 | `ζεδγβαYXWVUTRQPONM6KDF8JA239G5HB14C` | verified — matches OEIS; bucket-certified |
| 43 | `ηζεδγβαZYXWVUTSRQPONMKIAD5BCHFE78369421GJ` | verified — matches OEIS; bucket-certified |
| 44 | `θηζεδγβαZYWVUTSRQPONGJ5H198CD2I76FE3AL4` | verified — matches OEIS; bucket-certified |
| 45 | `ιθηζεδγβZYXWVUTSQPONLM1D4GCH287EBAJ563F` | verified — matches OEIS; bucket-certified |
| 46 | `κιθηζεδγβαZYXWVUTSRQPOLKJ628BID1G45F3CAH79E` | **corrected** — published value is suboptimal; see below |
| 47 | `λκιθηζεδγβαZYXWVUTSRQPOMLKHGF7D46JI1ACE958B23` | verified — matches OEIS; bucket-certified |
| 48 | `μκιθηζεδγβαZYXVUTSRQPNMKL1J92B3HI8C675DEAF4O` | verified — matches OEIS; bucket-certified; peeling flag cleared |
| 49 | `νμλκιθηζεδγβαZYXWVUTSRQPNMK9CJ23BFAE6GI81DHL547` | **CERTIFIED** — first value beyond the published b-file |
| 50 | `א‎νμλκιθηζεδγβαZYXWVUTSRQNMKDC6I897L4HBJEG2F531A` | **CERTIFIED ×3 engines** |
| 51 | `ב‎א‎νμλκιθηζεδγβαZXWVUTSRQPNLJDE758GKC2M43BA6F19I` | **CERTIFIED ×2 methods** |
| 52 | `ג‎ב‎א‎νμλκιθηζεγβαZYXWVUTSRPNLBCFJH765K9E1MAI2438G` | **CERTIFIED ×2 methods** |
| 53 | `ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWVUTSRPONM8EG4FC75BLJK29A3DI6H1` | **STRONG** (single-method exhaustive) |
| 54 | `ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWVUTSPOMG75A1EHFC4K289D6B3NLJI` | **STRONG** (zero lex-greater prefixes) |
| 55 | `ו‎ה‎ד‎ג‎ב‎א‎νμλκθηζεδγβαZYWVUTSRQOI1N8532AHG64EC9LKJD7F` | **STRONG** (single-method exhaustive) |
| 56 | `ז‎ו‎ה‎ג‎ב‎א‎μλκιθηζδγβαZYXVUTRQPLCN5B967DA2JK4FMHE1I3S` | **CERTIFIED ×2 methods** |
| 57 | `ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδβαZYXWVUTSQPONFB2DI1MK86GA349E7CH5L` | **STRONG** (single-method exhaustive) |
| 58 | `ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWVURQPONLCKADM763I9JFB85124HEG` | **CERTIFIED ×2 methods** |
| 59 | `י‎ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWVUSRQPOM1G76E9AB43I8KLNJ2HDF5C` | **STRONG** (zero lex-greater prefixes) |
| 60 | `כ‎י‎ט‎ח‎ו‎ה‎ד‎ג‎א‎νμλιθηζδγβαYXWVTNB7Q19SI648RHEL23DGMJC` | **CERTIFIED ×2 methods** |
| 61 | `ל‎כ‎י‎ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWVTSRQP83BENAICGJ21OLHD64F7M59K` | **STRONG** (single-method exhaustive) |
| 62 | `מ‎ל‎כ‎י‎ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWTSRQPOE57NBD4J6GHI9LM38CK21FA` | **STRONG** (zero lex-greater prefixes) |
| 63 | `נ‎מ‎ל‎כ‎י‎ט‎ח‎ז‎ה‎ד‎ג‎ב‎א‎νμλιθηζεδγβZYXWVUOGEHA8K5NC74PFDQJ6T31MB2L` | **STRONG** (single-method exhaustive, engine-confirmed) |
| 64 | `ס‎נ‎מ‎ל‎כ‎י‎ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXVUTSRQPNHO6E72IM4BC83LAD1F9GJK5W` | **STRONG** (arithmetic maximality) |

## What the labels mean

- **CERTIFIED** — independent engine families or methods produced the same
  value with exhaustive coverage; a proof of maximality defended by
  concordance.
- **STRONG** — one engine family produced an exhaustive maximality argument
  (every prefix that could beat the value is refuted, exhausted, or
  bound-pruned). The residual risk is implementation error, not an
  unsearched region.
- **verified** — matches the published OEIS value; independently recomputed
  by this project's engines (bases 41–49 additionally re-certified by the
  autonomous bucket driver, and bases 2–39 corroborated exactly by an
  independent 2020 Ruby solver — see
  [docs/results/PROVENANCE.md](docs/results/PROVENANCE.md)).

Per-base evidence, proof artifacts, and the full ladder definitions:
**[docs/results/FRONTIER-STATUS.md](docs/results/FRONTIER-STATUS.md)**.
The complete maximality arguments for the hard bases (54, 59, 61, 62, 63,
64) — including a(64)'s one-line arithmetic obstruction — are in
**[docs/results/MAXIMALITY-ARGUMENTS.md](docs/results/MAXIMALITY-ARGUMENTS.md)**.
Wall-clock records and the hardness metrics per base:
[docs/results/HARDNESS-AND-TIMINGS.md](docs/results/HARDNESS-AND-TIMINGS.md).

## The corrected a(46)

The published OEIS a(46) is **suboptimal**: it uses the correct (uniquely
forced) digit set but drops digit 28 out of its descending slot. Verified
independently twice, and re-certified by a second engine:

- published: `315044747190120671695735975284033252460559821155925276163089767538975200`
- corrected: `315044747190120671695735975284412123404260147529994283460952247723479200`

Details: [docs/results/MAXIMALITY-ARGUMENTS.md](docs/results/MAXIMALITY-ARGUMENTS.md#correction-to-the-published-a46).

## Above base 64

Bases 65–89 (the exact ceiling of 128-bit arithmetic — lcm(1..89) overflows
u128) were swept in a closed campaign: **22 bases refuted at their derived
minimum widths, zero completions, b74/b82 bounded by budget, b86 out of the
engine's structural reach.** Every negative is bounded on both the width and
release axes, so this is a statement about the campaign, not a proof about
the range. Full record:
[docs/results/HIGH-BASE-RANGE-65-89.md](docs/results/HIGH-BASE-RANGE-65-89.md).
An external review of where the project should stop:
[docs/reviews/SHOULD-THE-PROJECT-STOP-AT-BASE-64.md](docs/reviews/SHOULD-THE-PROJECT-STOP-AT-BASE-64.md).

## Provenance

The sequence began as *Enigma 1343: Digital Dividend* (New Scientist,
4 June 2005) and was submitted to OEIS by **Peter Boothe** in January 2006.
Francis Carr extended it to a(13) within a month — and Jes Wolfe, working
on the problem under Boothe's advice, reached base 16 that year without
submitting it. The public trail then rested for sixteen years until
Michael S. Branicky independently found a(14)–a(17) in 2022. Wolfe returned
in 2024 with a(18)–a(21) and the published b-file through base 48, and in
2026 carried the sequence to base 64 with maximality proven rather than
assumed — which makes the a(46) correction an erratum against our own
earlier b-file, not a dispute with anyone. A 2020 Ruby solver by the same
author independently corroborates bases 2–39 (38/38 exact). Full lineage
and cross-check record:
[docs/results/PROVENANCE.md](docs/results/PROVENANCE.md).

## Repository map

| where | what |
|-------|------|
| [docs/results/](docs/results/) | values, evidence ladder, maximality arguments, campaign records |
| [docs/theory/](docs/theory/) | the mathematics: forced sets, theorems, the open threshold conjecture |
| [docs/engine/](docs/engine/) | how the solvers work; design docs and optimization history |
| [docs/process/](docs/process/) | [LESSONS.md](docs/process/LESSONS.md) and operational footguns |
| [docs/reviews/](docs/reviews/) | external reviews, verbatim |
| `scripts/`, `logs/`, `evidence/`, `run_ledgers/`, `manifest_archive/` | campaign drivers and proof-bearing artifacts |

## Build & run

```sh
gcc -O2 -march=native -o a113028_v8 a113028_v8.c
./a113028_v8 2 48 1        # [lo] [hi] [verbose]
```

`carrytrie.cpp` is the production certification engine. Method overview:
[docs/engine/METHOD.md](docs/engine/METHOD.md); development setup (the
**required** git-hooks configuration) and engine history:
[docs/process/DEVELOPMENT.md](docs/process/DEVELOPMENT.md).

After cloning, this is **required, not optional**:

```sh
git config core.hooksPath .githooks
```

## License

CC0 1.0 Universal — public domain dedication. See LICENSE.
