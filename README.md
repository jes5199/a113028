# A113028 — largest pandigital-style multiples of their own digit lcm

[OEIS A113028](https://oeis.org/A113028): **a(B) is the largest number
whose base-B representation uses distinct nonzero digits and is divisible by
every digit it contains** — equivalently, divisible by the lcm of its digits.

Past any practical numeral system, "base B" is scaffolding: with n = B−1 the
problem is purely combinatorial — *choose a subset D ⊆ {1..n} and arrange it
as weights B⁰..B^{|D|−1} so that lcm(D) divides the total, maximizing the
value.* The subset turns out to be forced by elementary number theory; all
the difficulty is in the arrangement.

## Digit alphabet

Values below are written in the extended digit alphabet (digit values map to
symbols):

| values | symbols |
|--------|---------|
| 0–9 | `0`–`9` |
| 10–35 | `A`–`Z` |
| 36–48 | Greek lowercase `α β γ δ ε ζ η θ ι κ λ μ ν` (α=36 … ν=48) |
| 49–70 | Hebrew `א‎ ב‎ ג‎ ד‎ ה‎ ו‎ ז‎ ח‎ ט‎ י‎ כ‎ ל‎ מ‎ נ‎ ס‎ ע‎ פ‎ צ‎ ק‎ ר‎ ש‎ ת‎` (א=49 … ת=70; plain consonants — no niqqud, no final forms) |


### Right-to-left digits (bases > 49)

Hebrew is a right-to-left script, so a Hebrew digit dropped naively into a
value string triggers Unicode bidirectional reordering — adjacent Hebrew
letters display reversed and the number falls out of place-value order.
The notation therefore mandates: **every Hebrew digit is immediately
followed by U+200E LEFT-TO-RIGHT MARK** in rendered values. Each letter
then forms a singleton bidi run, so logical order = display order, in code
spans and plain text alike, with no HTML wrappers needed.

Demonstration (digit values 53 down to 46, crossing the Hebrew/Greek
boundary — both lines contain the same logical digit sequence `ה ד ג ב א
ν μ λ`):

- naive (renders out of order): `הדגבאνμλ`
- with LRM after each Hebrew letter (correct left-to-right place order): `ה‎ד‎ג‎ב‎א‎νμλ`

No computed value uses these yet (they begin at base 50); the spec is
forward-looking so the notation is total.

## Known values

Wall-clock times are single-core canonical bests measured on this box
(quiet, nice'd; full record sweep 2026-07-22 — winning engine varies per
base: v4 incremental scan on most, v12/v13 peeling where nilpotent
structure pays); m\* and log₁₀ W are the hardness metrics defined
in the *Method* section — m\* is the divergence depth (how many trailing
positions differ from the plain descending arrangement) and W ≈ m\*!/P! is
the predicted size of the irreducible search band.

Rows for bases 41–49 also carry the verdict of the 2026-07-23 autonomous
bucket-cert sweep (carrytrie `cert` mode — subset discovery + shallow-bucket
join + direct verification, self-checked against the known value): *bucket ✓
time* = certified by the bucket engine in that wall-clock; *bucket ✗* =
honest bucket failure (reason + scan-fallback time in RESULTS.md).

| base | value (alphabet) | wall-clock | m\* | log₁₀ W |
|-----:|------------------|-----------:|----:|--------:|
| 2 | `1` | 0.000s | 1 | 0.0 |
| 3 | `2` | 0.000s | 1 | 0.0 |
| 4 | `312` | 0.000s | 2 | 0.0 |
| 5 | `413` | 0.000s | 3 | 0.5 |
| 6 | `412` | 0.000s | 3 | 0.8 |
| 7 | `65142` | 0.000s | 4 | 0.6 |
| 8 | `7625134` | 0.000s | 5 | 1.3 |
| 9 | `8271536` | 0.000s | 5 | 0.7 |
| 10 | `9867312` | 0.000s | 5 | 1.3 |
| 11 | `A98762413` | 0.000s | 6 | 1.5 |
| 12 | `B9352176` | 0.000s | 6 | 1.5 |
| 13 | `CBA95847213` | 0.000s | 7 | 2.3 |
| 14 | `DCBA8513492` | 0.000s | 7 | 1.6 |
| 15 | `EDCB8219473` | 0.000s | 8 | 2.5 |
| 16 | `FEDCB59726A1348` | 0.021s | 8 | 2.5 |
| 17 | `GFEDCB93652741A` | 0.003s | 9 | 3.5 |
| 18 | `HGFEDCAB2514376` | 0.001s | 9 | 2.7 |
| 19 | `IHGFEDCB2671A3854` | 0.002s | 10 | 3.7 |
| 20 | `JIHGE9137B264DC` | 0.005s | 10 | 3.7 |
| 21 | `KJIHGFDBC286A4153` | 0.002s | 10 | 3.7 |
| 22 | `LKJIHGFED981C456732` | 0.006s | 10 | 3.7 |
| 23 | `MLKJIHGFEDC87521A6943` | 0.014s | 11 | 3.9 |
| 24 | `NLKJIHFEA679541B32DC` | 0.009s | 12 | 5.0 |
| 25 | `ONMLKJIHGFDB51284E3976A` | 0.16s | 12 | 5.0 |
| 26 | `PONMLKJIHGFB97461E325A8` | 0.035s | 12 | 5.0 |
| 27 | `QPONMLKJIHGFC6B72A85E3149` | 0.084s | 13 | 5.2 |
| 28 | `RQPONMKJIHF1352B69A8GD4` | 0.045s | 12 | 5.0 |
| 29 | `SRQPONMLKJIHGFDC2619485BA37` | 0.032s | 13 | 5.2 |
| 30 | `TSRQONMLJI2B1E4H8G397D6` | 0.795s | 13 | 5.2 |
| 31 | `UTSRQPONMLKJIHGE89A265D41BC37` | 0.125s | 14 | 5.4 |
| 32 | `VUTSRQPONMLKJIHF1758A9BC324E6DG` | 3.53s | 16 | 6.8 |
| 33 | `WVUTSRQPONLKJIHG7C813D59AE426` | 0.059s | 15 | 6.6 |
| 34 | `XWVUTSRQPONMLKJIEB72963C458F1DA` | 1.0s | 15 | 6.6 |
| 35 | `YXWVUTRQPONMKJIBCG16H59328D4A` | 0.498s | 15 | 6.6 |
| 36 | `ZYXWVUTSQPONMLKJF586A4E2B13D7HC` | 1.5s | 15 | 6.6 |
| 37 | `αZYXWVUTSRQPONMLKJHDB7A3G562E8F1C49` | 16.8s | 16 | 6.8 |
| 38 | `βαZYXWVUTSRQPONMLKGFDE2986BH3C4157A` | 11.3s | 16 | 6.8 |
| 39 | `γβαZYXWVUTSRPONMLKJ54H31E72B9FAC8G6` | 46.4s | 16 | 6.8 |
| 40 | `δγαZYXVUTSRQPNMLJIFEC6AB13D9254H7K` | 5.81s | 15 | 6.6 |
| 41 | `εδγβαZYXWVUTSRQPONMLJIFGC8574AED6H1932B` | 24.9s · bucket ✗ (mem), scan 44.1s | 17 | 8.0 |
| 42 | `ζεδγβαYXWVUTRQPONM6KDF8JA239G5HB14C` | 1m 12.6s · bucket ✓ 5m 58.3s | 18 | 8.2 |
| 43 | `ηζεδγβαZYXWVUTSRQPONMKIAD5BCHFE78369421GJ` | **13m 51.5s** · bucket ✓ (record — beat the 16m 8s scan even under concurrent load) | 18 | 8.2 |
| 44 | `θηζεδγβαZYWVUTSRQPONGJ5H198CD2I76FE3AL4` | **2m 0.4s** · bucket ✓ (record — was 20m 40.7s scan; 10.3×) | 19 | 9.5 |
| 45 | `ιθηζεδγβZYXWVUTSQPONLM1D4GCH287EBAJ563F` | **1m 10.3s** · bucket ✓ (record — was 5m 43.8s scan; 4.9×) | 19 | 9.5 |
| 46 | `κιθηζεδγβαZYXWVUTSRQPOLKJ628BID1G45F3CAH79E` **(corrected — see note)** | 6m 28.2s · bucket ✓ 21m 11.4s (independently re-certifies the corrected value) | 19 | 9.5 |
| 47 | `λκιθηζεδγβαZYXWVUTSRQPOMLKHGF7D46JI1ACE958B23` | **3m 10.5s** · bucket ✓ (record — was 4m 1.9s scan) | 20 | 9.7 |
| 48 | `μκιθηζεδγβαZYXVUTSRQPNMKL1J92B3HI8C675DEAF4O` | 10.35s · certified, bucket join | 20 | 9.7 |
| 49 | `νμλκιθηζεδγβαZYXWVUTSRQPNMK9CJ23BFAE6GI81DHL547` | ~4.4s · certified, bucket join | 21 | 9.9 |


### Correction to the published a(46)

This solver found a value **strictly larger** than the published OEIS entry,
using the same (uniquely forced) digit set {1..45}\{22,23} and the same
lcm = 409547311252279200 — the published arrangement drops digit 28 out of
its descending slot; the corrected one keeps it. Verified independently
(divisibility + digit multiset) twice.

- published (suboptimal): `315044747190120671695735975284033252460559821155925276163089767538975200`
- corrected: `315044747190120671695735975284412123404260147529994283460952247723479200`

The divergence-depth law initially flagged the published a(40) and a(48)
for the same reason — both were subsequently CLEARED (a(40) by direct
solve, a(48) by a nilpotent-peeling certificate): their divergence
outliers are *forced nilpotent-suffix effects* (the last digits are pinned
to 20 resp. 24 by suffix arithmetic), now fully explained. Final audit:
3 flags → 1 real error (a46, corrected above), 2 cleared. a(49), beyond
the published b-file, is certified: see NILPOTENT-PEELING.md.

## Method (Engine C)

Three observations reduce the problem to a narrow core:

1. **The subset is forced.** If B | lcm(D) no arrangement works (the last
   digit would have to be ≡ 0 mod B), and for every prime power q | B−1 the
   value ≡ digit-sum (mod q) regardless of arrangement — so the digit *set*
   is pinned by cheap arithmetic before any search. (Example: for B=49,
   49 ≡ 1 mod 16 and mod 3 force the removal of exactly digit 24 from
   {1..48}.)
2. **The divergence-depth law.** Only L_eff = lcm(D)/(order-1 part)
   constrains the arrangement. Permuting the last m positions reaches a
   target residue only when m! ≳ L_eff, so the answer equals the plain
   descending arrangement except in its last **m\* = min{m : m! ≥ L_eff}**
   positions. This held within ±2 for every known value — and its violation
   is what exposed the a(46) error.
3. **The leaf horizon.** Below P = ⌈log_B lcm(D)⌉ positions, the suffix
   *value* is fully determined mod lcm(D): at most a handful of candidates
   exist, each checkable in O(P) by decomposing and comparing digit sets.
   No search is needed below P.

**Engine C** is then: walk the descending arrangement from the top (free,
by law #2); inside the critical band around m\*, run a largest-digit-first
DFS where each node is pruned by sound per-prime-power feasibility DPs
(exact partition dynamic programs for multiplicative orders 1–3, plus
order-2 split checks); at depth P, decode the forced suffix directly.
The first hit in descending order is provably the maximum — the algorithm
is deterministic, with no budgets or restarts.

The irreducible cost is refuting "wrong turns" in the band — prefixes
lex-above the answer with no completion, where every cheap test necessarily
passes. That sweep has size **W ≈ m\*!/P!**, which is the hardness score in
the table.

## The tractability frontier

W grows like exp(Θ(n·ln ln n / ln n)) — subexponential but far
superpolynomial. Consequences (see FRONTIER.md for the full analysis):

- bases ≤ 47: seconds to minutes (all validated against the b-file);
- **bases 48–53: the current frontier** (hours–days) — exactly where the
  published b-file stops;
- bases 54–64: days to months, the practical endpoint under any realistic
  budget (and 64 is this implementation's digit-mask ceiling);
- bases ≥ 80: effectively never (>10¹⁷ refutation steps). A thousandfold
  more compute moves the frontier by only ~4–6 bases; the only real levers
  are the arithmetic luck of B±1 and better theory.

## Build & run

```
gcc -O2 -march=native -o a113028_v8 a113028_v8.c
./a113028_v8 [lo] [hi] [verbose]     # e.g. ./a113028_v8 2 48 1
```

`a113028.c` is the original two-engine solver (Engine S multiple-scan +
Engine D DFS with budget doubling); `a113028_v3..v8.c` are the successive
Engine C generations — see NOTES.md and ALGORITHMS.md for the full design
history and the near-optimality analysis.

## License

CC0 1.0 Universal — public domain dedication. See LICENSE.
