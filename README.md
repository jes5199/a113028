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
time* = certified by the autonomous driver in that wall-clock — one uniform
measurement basis across 41–49 (hand-tuned per-branch bests, where they
exist, are labeled separately); *bucket ✗* = honest bucket failure (reason +
scan-fallback time in RESULTS.md). Sweep total: 8/9 bases bucket-certified
in 47m 57s combined, 1 memory-declined base rescued by scan.

Bases 50–64 are **first-ever computed values** (the published OEIS b-file
ends at 48) at graded confidence — read the labels carefully: *certified
×N methods* = independent methods concordant (proof of maximality);
*STRONG* = single-method exhaustive; ***WEAK lower bound*** = a valid
completion whose **maximality is unproven** (refutations were
window-bounded, and such refutations have provably hidden larger answers
three times — see the b50/b56/b58 supersede stories). Canonical per-base
detail, evidence-class definitions, and the hardness taxonomy:
**[FRONTIER-STATUS.md](FRONTIER-STATUS.md)**.

**The maximality-proof engine** (2026-07-24,
[HIGHER-BASE-CERTIFICATION-STRATEGY.md](HIGHER-BASE-CERTIFICATION-STRATEGY.md)):
an exact outer lexicographic branch-and-bound wraps the bucket engine as a
terminal oracle — every branch above the incumbent is bound-pruned,
exhaustively refuted, or exact-searched, so **window width is a performance
knob, never a soundness boundary**, and discovery heuristics can never
justify a refutation. Validated by reproducing and *proving* the known
a(56)/a(58)/a(60) end-to-end (upgrading all three to ×2-method certified);
live in the unified production binary with resumable, shard-parallel proof
manifests.

**Bases 65–89** are open territory unlocked by the 128-bit mask widening
(2026-07-24): fast passes at b65/b73/b81 all came back honest NO-VALUE
(band-deep — the machinery is validated, the answers sit below fast-pass
windows; the proof engine is the path). **Base 89 is the exact arithmetic
ceiling** (lcm(1..88) fits in 128 bits; lcm(1..89) does not — bases ≥90
refuse cleanly and await a bignum arithmetic epic).

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
| 48 | `μκιθηζεδγβαZYXVUTSRQPNMKL1J92B3HI8C675DEAF4O` | 10.35s (hand-tuned bucket best) · bucket ✓ 18.3s | 20 | 9.7 |
| 49 | `νμλκιθηζεδγβαZYXWVUTSRQPNMK9CJ23BFAE6GI81DHL547` | ~4.4s (hand-tuned bucket best) · bucket ✓ 16.3s | 21 | 9.9 |
| 50 | `א‎νμλκιθηζεδγβαZYXWVUTSRQNMKDC6I897L4HBJEG2F531A` | 49.0s · certauto certified ×3 engines (see RESULTS: autopsy story) | 21 | 9.9 |
| 51 | `ב‎א‎νμλκιθηζεδγβαZXWVUTSRQPNLJDE758GKC2M43BA6F19I` | 50.1s · certauto certified, ×2 methods (v4 scan concordant, 5h51m) | 21 | 11.0 |
| 52 | `ג‎ב‎א‎νμλκιθηζεγβαZYXWVUTSRPNLBCFJH765K9E1MAI2438G` | 21.5s · certauto certified, ×2 methods (v15 candidate concordant) | 21 | 11.0 |
| 53 | `ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWVUTSRPONM8EG4FC75BLJK29A3DI6H1` | 279s · STRONG (single-method exhaustive) | 22 | 11.3 |
| 54 | `ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWVUTSPOMG75A1EHFC4K289D6B3NLJI` | **STRONG** — |D|=51 (forced set), maximality: zero lex-greater prefixes (see below) | — | — |
| 55 | `ו‎ה‎ד‎ג‎ב‎א‎νμλκθηζεδγβαZYWVUTSRQOI1N8532AHG64EC9LKJD7F` | 7.1s · STRONG (single-method exhaustive) | 22 | 11.3 |
| 56 | `ז‎ו‎ה‎ג‎ב‎א‎μλκιθηζδγβαZYXVUTRQPLCN5B967DA2JK4FMHE1I3S` | 677s · **certified, ×2 methods** (engine + outer-B&B maximality proof) | 22 | 11.3 |
| 57 | `ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδβαZYXWVUTSQPONFB2DI1MK86GA349E7CH5L` | 46s · STRONG (single-method exhaustive) | 22 | 11.3 |
| 58 | `ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWVURQPONLCKADM763I9JFB85124HEG` | 236s · **certified, ×2 methods** (engine + outer-B&B proof; 8× via planner calibration) | 22 | 11.3 |
| 59 | `י‎ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWVUSRQPOM1G76E9AB43I8KLNJ2HDF5C` | **STRONG** — |D|=57 (forced set), maximality: zero lex-greater prefixes | — | — |
| 60 | `כ‎י‎ט‎ח‎ו‎ה‎ד‎ג‎א‎νμλιθηζδγβαYXWVTNB7Q19SI648RHEL23DGMJC` | 13.9s · **certified, ×2 methods** (engine + outer-B&B proof; post-churn-fix) | 23 | 11.5 |
| 61 | `ל‎כ‎י‎ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νλκιηζεδγβαZYXWVUTSRQPONM56197GHKFCED834ILJ2AB` | **WEAK lower bound** (window-bounded at W=21; 1504s) | 22 | 11.3 |
| 62 | `מ‎ל‎כ‎י‎ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXWTSRQPOE57NBD4J6GHI9LM38CK21FA` | **STRONG** — |D|=59 (forced set), maximality: zero lex-greater prefixes | — | — |
| 63 | `נ‎מ‎ל‎כ‎י‎ט‎ח‎ז‎ה‎ד‎ג‎ב‎א‎νμλιθηζεδγβZYXWVUOGEHA8K5NC74PFDQJ6T31MB2L` | **STRONG** (single-method exhaustive; complete maximality argument — see below) | — | — |
| 64 | `ס‎נ‎מ‎ל‎כ‎י‎ט‎ח‎ז‎ו‎ה‎ד‎ג‎ב‎א‎νμλκιθηζεδγβαZYXVUTSRQPNHO6E72IM4BC83LAD1F9GJK5W` | **STRONG** — |D|=63 (full alphabet), maximality by arithmetic obstruction (see below) | — | — |


### b63 — first-ever value, complete maximality argument (STRONG, single method)

Base 63 is the live frontier run. The forced digit set is uniquely
determined — {1..62} minus {9,18,27,28,36,45,54}, so |D| = 55 — and the
outer-lexicographic branch-and-bound maximality proof is grinding the
arrangement space on 3 parallel shards.

Current best survivor (99 decimal digits):

```
919638671548642431083440200815272686323739054438990208528434484026726330816976573347252565881592800
```

Independently verified from scratch (not the engine's own report): base-63
decode matches, 55 distinct nonzero digits all in 1..62, digit set is
exactly the forced complement, lcm(digits) | N, 63 ∤ lcm (gcd = 21),
N ≡ digitsum (1736) mod 62.

Because verified survivors exist, the forced 55-set cannot fully refute — so
b63 yields a first-ever *value*. What remained open was only whether this
survivor is the maximum.

#### The cutoff argument: the proof is already covered

Exhaustively traversing the whole arrangement space is unnecessary. Only
prefixes that could still produce a **larger** completion matter.

At terminal width W = 22, terminals are entered after 55 − (22+1) = **32**
prefix digits (confirmed: every manifest record carries a 32-digit prefix).
The incumbent's first 30 digits are exactly the descending top-30 of the
forced set, so any prefix deviating before position 31 is lexicographically
smaller and every completion beneath it is dominated — no proof record
needed. The prefixes that could still win are therefore:

- position 31 ∈ {29, 26, 25}, each leaving 24 choices at position 32 → 72
- position 31 = 24 with position 32 ∈ {29,26,25,23,22,21,20,19,17} → 9
- the incumbent's own prefix → 1

**82 lex-relevant terminal prefixes.** Note this is the *combinatorial*
count, not an engine counter range: in the current shard run the engine
assigned terminal counters only to the 40 of them it actually executed —
counters 0..39 are exactly the prefixes lexicographically ≥ the incumbent,
with **the incumbent at counter 39** — while the other 42 already carried
definitive records from the earlier serial run and were skipped without
being counted (the banked manifest's depth-32 records carry no `counter`
field at all). The two runs share no counter space, so the union is sound
only when keyed by exact prefix, which is how it is checked below.
Checking the union of the three shard manifests and the banked
manifest **by exact prefix** (aggregate counts prove nothing):

| check | result |
|---|---|
| lex-relevant prefixes covered | **82 / 82**, 0 missing |
| disposition mix | **81 REFUTED + 1 FOUND** |
| the 1 FOUND | the incumbent's own branch, maxSurvivor = the incumbent |
| conflicting duplicate records | 0 |
| RESOURCE_DECLINED gaps | 0 — 8 of the 82 were once declined, **all later re-run to definitive** |
| global max survivor, all records | equals the incumbent |
| parameter compatibility | all 4 files: every prefix a distinct 32-subset of the forced 55-set |

Every prefix that could lexicographically beat the incumbent is exhaustively
refuted, and the incumbent's own branch has it as the branch maximum. **That
is the maximality proof**, assembled from work already completed.

**Honest boundary:** this independent verification checked the *coverage* and
the *cutoff logic* from scratch; it trusts the engine-produced REFUTED
records rather than re-running 81 exhaustive terminal searches. Two
independent verifiers (boss-clod's and this session's, each derived from
scratch) agree on the cutoff and the coverage; neither re-executed a
terminal.

**ENGINE CONFIRMATION LANDED (2026-07-25).** An earlier revision of this
section claimed the engine-level check was unobtainable from the current
binary. That was wrong, and the correction matters: **the lex-domination
cutoff already exists** (`carrytrie.cpp` ~L5897: `upperBoundArrayBB` compared
against the pruning incumbent, returning `PROVED_BELOW_INCUMBENT`, plus a
per-child monotonic `break`). It never fired during the 17-hour shard grind
for a mundane reason — shard mode freezes the pruning incumbent once at
startup, the banked manifest then held **zero** FOUND records, so it froze at
`(none)` and pruning was disabled for the run's entire life. Discovering the
incumbent at counter 39 did not tighten it, by design.

Re-running plain `resume` against a manifest that actually contains the FOUND
records (isolated directory, repo files untouched, input manifest hashed):

```
resume: loaded 3767 lines -> 1246 proven-REFUTED (skip), 32 proven-FOUND (skip+seed)
resume-seeded incumbent from manifest: 9196386715486424310834...81592800
DFS DONE: nodesVisited=117 found=0 refuted=0 pruned=32 unfinished(declined)=0
          resume-skipped(refuted=81 found=1) wall=136.073s
base=63 CERTIFIED (zero unfinished branches)
```

**`found=0 refuted=0` — zero new terminal executions**, and
`resume-skipped(refuted=81 found=1)` is exactly the 82-prefix frontier
derived above, reached independently by the engine's own DFS. 32 bound-prunes,
zero declined, 136 seconds. The 24 branches the union reports as unfinished
are precisely the 24 counters `certbb-merge` flagged as uncovered (the ragged
tail at counter ≥1193) — the DFS **prunes** them as dominated rather than
needing them, so the merge's objection and the cutoff's answer are the same
branches.

Log: `b63_engine_confirmation/`.

This row is therefore **STRONG — single-method exhaustive, engine-confirmed**.
It is deliberately *not* labelled CERTIFIED: the project's ladder reserves
that for concordance from an independent *engine family*, and the check above
— however clean — is the same engine agreeing with itself. Two independent
verifiers derived the cutoff and coverage by hand, and the engine's own DFS
then reproduced the identical 81+1 frontier, which is strong evidence and not
a second method. Method and credit: `FASTER-PROVISIONAL-MAXIMUM-VALIDATION.md`.

Note b63 sits strictly below b64 by construction: 55 base-63 digits cap it
at 99 decimal digits, while b64 (which needs no forced drops, since no digit
≤ 63 is divisible by 2⁶) keeps 60 digits and already has a valid 109-digit
completion.

### b62 — a(62)

**Value (106 decimal digits, 59 base-62 digits; forced set = `{1,…,61}`
minus `{30,31}`):**

```
5636285065773651499796945616994676443519614972212770252490392293226928630689747617736444403004286825548000
```

Supersedes the previous `|D|=58`, 104-digit WEAK value on both counts. Found
by `certset` at **W=22 in 1844s**, the cheapest rung.

Maximality is the same structural argument as b54 and b59: the terminal
prefix length is `59 − 23 = 36`, the value's first 36 digits are exactly the
**descending top-36** of the forced set, so there are **zero lex-greater
prefixes** (0 sub-regions, confirmed), the window of 23 covers the entire
remainder, and only the equal-prefix branch exists. **STRONG**, resting on the
single `runWrongTurnSearch` exhaustiveness dependency.

This was the **third consecutive out-of-sample confirmation** of the `r0`
prediction — see the b59 section for what that prediction is and why it holds
by construction rather than by observation.

### b59 — a(59), and the first *prediction* this framework made and cashed

**Value (101 decimal digits, 57 base-59 digits; forced set = `{1,…,58}`
minus `{29}`):**

```
86783176769582410820652763941251198601388207643947455481175050246380721520450704821249718313022861600
```

Supersedes the previous `|D|=56`, 100-digit WEAK value on both counts. Found
by `certset` at **W=22 in 1774s**, the cheapest rung.

**Maximality, same shape as b54.** At `W=22` the terminal prefix length is
`57 − 23 = 34`, and the value's first 34 digits are exactly the **descending
top-34** of the forced set — the lexicographically greatest prefix available.
So there are **zero lex-greater prefixes** (confirmed computationally: 0
sub-regions), the window of 23 positions covers the entire remainder
(`57 − 34 = 23 = W+1`), and the case analysis collapses to the equal-prefix
branch alone.

**What makes this one methodologically different: it was predicted in
advance.** Before the run returned, `r0 = 1` — a by-product of feasibility
censuses that had *filtered nothing and were written off as wasted* — implied
that `certset` would search the descending-top prefix, hence that any hit
would be immediately maximal. The prediction was then strengthened to a
guarantee: for b59, `gcd(lcm(D), 59) = 1`, so `L_nil = 1` and `T = 0`, the
admissible-suffix DP has depth zero and returns 1 unconditionally, and
therefore **`r0 = 1` holds at every width by construction** rather than by
observation.

It held. This is the framework's first genuine *prediction* — made before the
result, on a base not yet solved — rather than a rule fitted to cases already
known. See `CERTBB-OPERATIONAL-FOOTGUNS.md` §13 for the general lesson about
measurements that fail at their stated purpose.

### b54 — a(54), maximal because there is nothing above it

**Value (89 decimal digits, 51 base-54 digits; forced set = `{1,…,53}` minus
`{26,27}`):**

```
22486771935366632201839109831632289737092127593455046754337233294898735154535035683832000
```

Supersedes the previous `|D|=50`, 87-digit WEAK value on both counts. Found by
`certset` at **W=22 in 522.7s** — the cheapest rung, no ladder required.

**Why it is maximal — the whole argument.** At `W = 22` the terminal prefix
length is `51 − 23 = 28`, and this value's first 28 digits are

```
53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32 31 30 29 28 25 24
```

which is **exactly the descending top-28 of the forced set** (26 and 27 are
dropped, so 28 is followed by 25). The lexicographically greatest prefix
available *is* the largest 28 digits in descending order — so this prefix is
that maximum, and **there are zero lex-greater prefixes**. There is no region
to dispose of because the region is empty.

The case analysis collapses to one branch:

1. **Lex-greater prefix** — none exist. Vacuous.
2. **Equal prefix** — `certset` searched that prefix's window exhaustively;
   the window is 23 positions and `51 − 28 = 23`, so it covers the entire
   remainder, and it returned this value as the maximum.
3. **Smaller digit set** — fewer digits, strictly smaller number.

So a(54) rests on **exactly one computational dependency**,
`runWrongTurnSearch`'s exhaustiveness — the same single dependency as b64's
equal-prefix branch, and without b64's seven regions to eliminate first. It is
labelled STRONG rather than CERTIFIED for the same reason: a second
independent method on that one terminal search is what is still missing.

Compare b63 (2 free positions above the prefix → 82 branches to refute) and
b64 (6 free positions → 7 regions, disposed of by an arithmetic obstruction).
**b54 has 0 free positions**, which is the cheapest shape this problem admits.

### b64 — a(64) determined by a one-line arithmetic obstruction

**Value (114 decimal digits, 63 base-64 digits — the entire alphabet
`{1,…,63}`):**

```
615501230581118093011161624139305632631631099715005717665003460230260844588863956324726850814214663290418203804000
```

This supersedes the previous `|D|=60`, 109-digit WEAK value on both counts:
more digits and numerically larger. Two independent completions were found —
one by `certset` at W=24, one by `certdisc` at W=22 from a different prefix —
and both pass all seven validity gates.

**Why the forced set is the whole alphabet.** `64 = 2⁶`, and the largest power
of two ≤ 63 is `32 = 2⁵`, so `64 ∤ lcm(1..63)` with **no drops at all**. The
digit-sum condition holds outright: `1+⋯+63 = 2016 = 63 × 32 ≡ 0 (mod 63)`.

**Why the value is maximal.** With `L = lcm(1..63)` we have `v₂(L) = 5`, so the
nilpotent part is `L_nil = 2⁵ = 32` and `B¹ = 2⁶ ≡ 0 (mod 32)` — that is,
**`T = 1`**. Every digit at position ≥ 1 therefore contributes `0 (mod 32)`, so

> `32 | N` **iff** 32 divides the units digit — and **32 is the only digit in
> `{1,…,63}` divisible by 32**. Hence *every* completion ends in the digit 32.

The value above matches plain descending order for its first 31 positions,
then places 31 where descending would place 32, exiling 32 to the final slot
exactly as the obstruction demands. Any competitor falls into one of four
cases: deviating before position 31 is lexicographically smaller (those
positions are forced maximal, and all completions have 63 digits so digit-lex
order *is* numeric order); deviating anywhere in positions 31–37 requires
placing **32 inside the prefix**, which starves the units position and kills
all seven lex-greater sub-regions at once; an equal prefix is settled by an
exhaustive terminal search (`survivors=1`, verified, no resource decline); and
any smaller digit set yields a shorter, smaller number.

**Three of those four cases are checkable on paper.** Only the equal-prefix
branch is engine-dependent, which is why the row is STRONG rather than
CERTIFIED — a second independent method on that one branch is what is still
missing. Full argument and controls: **[A64-MAXIMALITY.md](A64-MAXIMALITY.md)**.

**Computational corroboration.** A census of the lex-greater region returned
**0 feasible across 906,192 digit-subsets**, standing for 5.478 × 10⁸ ordered
prefixes. That census is simply this obstruction evaluated mechanically.

> ⚠️ **The census is a one-sided test.** `FEASIBLE = 0` is conclusive;
> **`FEASIBLE > 0` says nothing** — feasibility is necessary, not sufficient.
> Run above b63's engine-confirmed value it reports nonzero in most regions,
> yet every one of those was refuted by exhaustive search. And at **b59/b61**
> the base is prime, so `L_nil = 1` and there is no nilpotent constraint at
> all: the census is **vacuous there by construction**.

**The lens does not generalise.** `T = 1` requires `L_nil | B`, and b64 is the
only base in range where that holds:

| base | 54 | 56 | 58 | 59 | 60 | 61 | 62 | 63 | **64** |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `L_nil` | 288 | 196 | 32 | 1 | 864 | 1 | 32 | 147 | **32** |
| `T` | 5 | 2 | 5 | 0 | 3 | 0 | 5 | 2 | **1** |

Everywhere else `T ≥ 2`, so the constraint binds the last `T` digits jointly
and forces no single digit. Bases 54, 59, 61 and 62 still require real search.

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
superpolynomial. The 2026-07-23 frontier campaign rewrote the practical
picture (FRONTIER.md has the original analysis; FRONTIER-STATUS.md the
current one):

- bases ≤ 52: **certified** (multi-method) — the certified frontier moved
  from the b-file's 48 to 52 in one day;
- bases 53–64: **values computed at graded confidence** up to the
  implementation's digit-mask ceiling — certified-clean (56, 58, 60),
  strong (53, 55, 57), weak lower bounds (54, 59, 61, 62, 64), and 63 now a
  verified lower bound with its maximality proof mid-flight (was: no value
  at all). Hardness is not one wall but three separable modes: *discovery
  churn* (fixed by feasible-subset enumeration — b60 went from no-value at
  90 min to certified in 14s), *memory* (fixed by admission control), and
  *band depth* — the one real open problem: when the answer's divergence
  from descending order exceeds the affordable search window (~9× cost per
  extra position), refutations become window-bounded and maximality is
  unprovable by search alone;
- the real levers are the arithmetic of B−1 (smooth ⇒ strong subset
  filtering; prime ⇒ deep bands), band-depth certification theory, and —
  as always — the luck of the nilpotent structure.

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
