# a(64): a one-line arithmetic obstruction, with computational corroboration

**Date:** 2026-07-25
**Status:** argument complete; awaiting independent re-derivation before any
OEIS-facing claim.

## The value

```
615501230581118093011161624139305632631631099715005717665003460230260844588863956324726850814214663290418203804000
```

114 decimal digits; 63 base-64 digits using the **entire alphabet
`{1,…,63}`** exactly once. Digit sequence MSB-first:

```
63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39
38 37 36 35 34 33 31 30 29 28 27 26 25 23 17 24 6 14 7 2 18 22 4 11 12 8 3
21 10 13 1 15 9 16 19 20 5 32
```

## The obstruction

Let `L = lcm(1..63)` and `B = 64 = 2⁶`. The only prime shared by `B` and `L`
is 2, and `v₂(L) = 5` (since `32 = 2⁵ ≤ 63 < 64`), so the *nilpotent part* is

```
L_nil = 2⁵ = 32,   and   B¹ = 2⁶ ≡ 0  (mod 32)   ⇒   T = 1.
```

`T = 1` is the whole argument. Because every digit at position ≥ 1 is
multiplied by a power of `B` that is already `≡ 0 (mod 32)`,

> **`32 | N` if and only if `32` divides the units digit.**

The only digit in `{1,…,63}` divisible by 32 is **32 itself**. Therefore:

> **Every completion of the full alphabet ends in the digit 32.**

Both independently-found completions do (`N ≡ 0 mod 32`, last digit 32).

## Maximality

The incumbent agrees with plain descending order (`63,62,…,33`) for its first
**31** positions, then places **31** where descending would place 32 — exiling
32 to the final position, as the obstruction requires. At terminal width
`W = 24` the prefix length is `63 − 25 = 38`. Any competing completion falls
into exactly one case:

1. **Prefix deviates before position 31.** Those positions are forced maximal
   (the largest remaining digit each time), so any deviation is
   lexicographically *smaller*. All completions have 63 digits, so digit-lex
   order is numeric order. Not a threat.

2. **Prefix deviates at position j ∈ [31, 37].** At each such position the
   incumbent holds the largest unused digit ≤ 31 (namely 31,30,29,28,27,26,25
   in turn), so **the only larger digit still available is 32**. A competing
   prefix must therefore place 32 *inside the prefix* — leaving no digit
   divisible by 32 for the units position. By the obstruction, **no such
   arrangement is divisible by `L`, hence none completes.** All seven
   sub-regions die at once.

3. **Prefix equals the incumbent's 38-digit prefix.** `runWrongTurnSearch`
   is exhaustive over the entire remaining pool at the node (every pool digit
   tried as candidate, descending), and returned this value as the maximum
   with `survivors=1, verified 1 OK / 0 FAILED` — no `DECLINED`, so no
   resource outcome masquerading as completeness.

4. **A different digit set.** Any set of ≤ 62 digits yields a number with
   fewer base-64 digits, hence strictly smaller. `{1,…,63}` is the unique
   63-element subset, and it completes.

Cases 1–4 are exhaustive, so the value above is **a(64)**.

## Computational corroboration

The obstruction was found *after* a census had already produced the same
verdict by brute enumeration, which now serves as independent confirmation
rather than as the argument:

| region (deviation at j → 32) | subsets tested | feasible |
|---|---:|---:|
| j=31 | 736,281 | 0 |
| j=32 | 142,506 | 0 |
| j=33 | 23,751 | 0 |
| j=34 | 3,276 | 0 |
| j=35 | 351 | 0 |
| j=36 | 26 | 0 |
| j=37 | 1 | 0 |
| **total** | **906,192** | **0** |

Those subsets stand for **5.478 × 10⁸ ordered prefixes**. The collapse from
10⁸ to 10⁵ comes from the fact that **feasibility depends only on the prefix's
digit *set*, not its ordering** — verified both by reading
`countAdmissibleSuffixTuplesGen` (it consumes only the pool) and empirically,
by permuting prefixes and confirming identical verdicts.

The census is precisely a mechanical evaluation of the obstruction: with
`T = 1` it counts pool digits divisible by 32, and every one of those 906,192
subsets has 32 in the fixed prefix.

### The census is a ONE-SIDED test

This limitation matters and was demonstrated, not assumed:

- **`FEASIBLE = 0` ⇒ no completion in that region. Conclusive.**
- **`FEASIBLE > 0` ⇒ inconclusive.** Says nothing; terminals still required.

Feasibility is *necessary, not sufficient*. Run above **b63**'s
engine-confirmed maximal value, the census reports `FEASIBLE > 0` in most of
its 12 lex-greater sub-regions — yet the engine refuted all 81 prefixes
there. b63 is the case where the census would have proved nothing.

**Controls run (all consistent):**

| control | expectation | result |
|---|---|---|
| b56 (certified ×2) | 0 feasible above it | 2 regions, both **0** |
| b58 (certified ×2) | 0 feasible above it | **0 sub-regions** (vacuous) |
| b63 (engine-confirmed) | — | nonzero → **inconclusive**, no contradiction |
| falsification: regions *containing* a known completion | must be ≥ 1 | b63/b60/b64 → 276/14,949/… and **593,775** |
| order-invariance | verdict independent of ordering | 6 perms feasible, 4 perms infeasible — stable |

The decisive falsification line: b64 truncate-6 is **C(31,6) = 736,281
subsets → 593,775 feasible** — the *same* subset-space size, base, width and
code path as the j=31 lex-greater region that returned **0**. Same machinery,
593,775 versus 0. The zero is a signal, not a stuck test.

## Does this generalise? Mostly no — and that is worth knowing

`T = 1` is what makes the argument a one-liner, and it holds **only when
`L_nil | B`**. Across the campaign's live bases:

| base | \|D\| | L_nil | T | admissible last digits |
|---:|---:|---:|---:|---|
| 54 | 51 | 288 | 5 | — |
| 56 | 48 | 196 | 2 | — |
| 58 | 55 | 32 | 5 | — |
| 59 | 57 | 1 | 0 | **no constraint at all** |
| 60 | 47 | 864 | 3 | — |
| 61 | 59 | 1 | 0 | **no constraint at all** |
| 62 | 59 | 32 | 5 | — |
| 63 | 55 | 147 | 2 | — |
| **64** | **63** | **32** | **1** | **{32} — forced** |

**b64 is the only base in the range with `T = 1`**, because it is the only
prime power whose exponent exceeds `v_p(L)` (`2⁵ | 2⁶`). Everywhere else
`T ≥ 2`, so the constraint binds the last `T` digits *jointly* rather than
forcing a single digit, and the one-line argument does not transfer.

Worse for the prime bases: at **b59 and b61**, `B` is prime and `v_B(L) = 0`,
so `L_nil = 1` and there is **no nilpotent constraint whatsoever**. Every
prefix is "feasible" by this test, so the census returns nonzero everywhere
and is useless there by construction.

**Conclusion for §8:** the last-digit lens is decisive at b64 and nowhere
else. b54/b59/b61/b62 still need real search — the two-axis discovery
instrument (`certdisc`), not this shortcut.
