# Forced-set cardinality audit (verifying doc §8)

**Date:** 2026-07-25
**Purpose:** `FASTER-PROVISIONAL-MAXIMUM-VALIDATION.md` §8 claims that for
bases 54, 59, 61, 62 and 64 the *forced maximal-cardinality digit set* is
**larger** than the set each current provisional value actually uses — which
would mean any valid completion of the forced set supersedes the current
value on cardinality alone, converting five maximality grinds into five
completion searches. That is a load-bearing premise for days of planned
work, and it had not been checked. This is the check.

Motivation for checking at all: on 2026-07-25 the b63 maximality argument was
found to rest on the unverified premise that its 55-set is *forced* rather
than merely *assumed*. It held, but nobody had done the arithmetic. §8 is the
same shape of premise, so it gets the same treatment before, not after.

## Method

Two elementary necessary conditions, applied to `D ⊆ {1..B-1}`:

1. **`B ∤ lcm(D)`.** Writing `B = ∏ p^e`, it suffices to pick one prime power
   `p^e ‖ B` and delete every digit divisible by `p^e` — that forces
   `v_p(lcm(D)) < e`. The cheapest route is the `p^e` maximising `p^e`
   (minimising `⌊(B−1)/p^e⌋` deletions). Ties and competing routes are
   enumerated, not assumed.
2. **Digit-sum congruence.** `N ≡ digitsum(D) (mod B−1)`, and
   `g = gcd(lcm(D), B−1)` divides `N`. Hence `digitsum(D) ≡ 0 (mod g)`.
   If it is not, delete the smallest digit (or, failing that, the cheapest
   pair) restoring the congruence.

Both conditions are **necessary, not sufficient** — they bound `|D|` from
above and identify the candidate set; they do not prove a completion exists.
That is exactly why §8's five bases become *completion searches*.

## Self-validation

Run against the four bases whose values are already certified, the method
must reproduce their digit sets exactly. It does:

| base | derived forced drops | derived \|D\| | \|D\| in the certified value |
|-----:|---|---:|---:|
| 56 | {8,16,24,32,40,48,52} | 48 | 48 ✓ |
| 58 | {28,29} | 55 | 55 ✓ |
| 60 | {5,10,15,20,25,30,35,40,45,50,55} + {24} | 47 | 47 ✓ |
| 63 | {9,18,27,28,36,45,54} | 55 | 55 ✓ |

4/4 exact, including b60's characteristic "11 multiples of 5 kill the
ten-rule's 5-factor, plus 24 for the digit-sum" signature and b63's
"9-multiples route at 7 drops beats the 7-multiples route at 8".

## Result — §8 confirmed

| base | forced drops | forced \|D\| | \|D\| in use | gain |
|-----:|---|---:|---:|---:|
| 54 | {26,27} | 51 | 50 | **+1** |
| 59 | {29} | 57 | 56 | **+1** |
| 61 | {30} | 59 | 58 | **+1** |
| 62 | {30,31} | 59 | 58 | **+1** |
| 64 | **{} — none** | **63** | 60 | **+3** |

All five §8 claims reproduce. **b64 is the standout**: `64 = 2⁶` and no digit
below 64 is divisible by `2⁶` (the largest power of two ≤ 63 is `32 = 2⁵`),
so `64 ∤ lcm(1..63)` automatically and *no drop is required*. The digit-sum
condition is satisfied outright: `1+⋯+63 = 2016 = 63 × 32 ≡ 0 (mod 63)`.
The forced set is the **entire alphabet `{1,…,63}`**, 63 digits against the
60 the current provisional value uses.

## Consequence

Any valid completion of these forced sets beats the incumbent by cardinality
regardless of within-set arrangement, so for these five bases the ordering is
**completion search first, maximality proof second**. b64 is the cheapest
test of the whole thesis (no drop enumeration at all) and the largest payoff
(+3 digits), so it is the natural first target.

Caveat carried forward: a larger forced set is an *upper bound* that may have
no completion. If a forced set turns out to be uncompletable, the next
cardinality down is searched — the current provisional value is not
invalidated by a failed completion search, only superseded by a successful
one.
