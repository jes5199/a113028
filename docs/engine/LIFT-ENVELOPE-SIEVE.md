# Rearrangement-envelope lift sieve

**Status:** exact pruning rule, implemented in a diagnostic and measured  
**Scope:** peeled base-49 carry-trie join  
**Baseline commit:** `f8ffae3`  
**Date:** 2026-07-22

## Executive summary

Before walking the carry trie for an `(x, lift)` query, bound the ordinary
integer value of the twelve-digit leaf using only its available digit pool.
If the lift would require the stored `y` residue to lie outside
`[0, L_c)`, reject the entire query at the root.

The test is exact, requires no additional trie storage, and costs only a
small sort or two scans of the live digit mask per `x`.

On the candidate-21 refutation:

| split | baseline visits | root-sieved visits | visit reduction | baseline wall | root-sieved wall |
|---|---:|---:|---:|---:|---:|
| `3+4` | 60,100,471 | 32,713,540 | 45.6% | 4.447 s | 2.547 s |
| `2+5` | 23,950,821 | 13,080,525 | 45.4% | 12.385 s | 6.385 s |

For the proposed `1+6` split, the test rejects 35 of the 76 possible
`(x, lift)` roots: every lift `j = 3`, and every `j = 2` except those with
`x` equal to 8, 17, or 18. The projected ten million trie visits should
therefore fall to roughly 5–5.5 million before any internal-node
strengthening.

The strategic consequence is important: root-sieved `2+5` now has only about
13.1 million visits while building 1.40 million `y` records. The projected
root-sieved `1+6` walk is smaller, but it builds 19.54 million records.
Benchmark the root-sieved `2+5` implementation before paying the roughly
14-fold construction increase of `1+6`.

## 1. Carry-join setting

After nilpotent peeling, candidate 21 leaves nineteen free digits in
positions 1 through 19. The leaf horizon is

```text
P = 12.
```

The seven positions above the leaf are divided into:

- `NX` ordered `x` digits;
- `NY` ordered `y` digits;
- `NX + NY = 7`.

For a fixed ordered `x`, let `c_x` be the required coprime-core residue after
subtracting the contribution of `x`. For an ordered `y`, define

```text
u_y = B^12 y mod L_c,       0 <= u_y < L_c.
```

The leaf must then satisfy

```text
leaf = c_x - u_y mod L_c.
```

Because an ordinary twelve-digit base-49 leaf can exceed `L_c`, the
implementation enumerates the small number of integer lifts

```text
W_j = c_x + j L_c
```

and searches for

```text
leaf = W_j - u_y.
```

At base 49,

```text
ceil(49^12 / L_c) = 4,
```

so the existing walker enters the trie four times for every ordered `x`.
The lift-envelope sieve proves that almost half of those entries cannot
possibly decode to the required digit multiset.

## 2. The leaf rearrangement envelope

Fix `x`, and let

```text
U = A19 \ x_mask
```

be the digits not consumed by `x`. The twelve leaf positions use twelve
distinct digits from `U`; the remaining `NY` digits are consumed by `y`.

Define `leaf_min(U)` as follows:

1. take the twelve smallest digits in `U`;
2. place them in increasing order from most significant to least
   significant position.

Define `leaf_max(U)` similarly:

1. take the twelve largest digits in `U`;
2. place them in decreasing order from most significant to least
   significant position.

Then every possible leaf satisfies

```text
leaf_min(U) <= leaf <= leaf_max(U).
```

### Proof

For any fixed twelve-digit set, an inversion in which a larger digit occupies
a more significant position than a smaller digit increases the represented
integer. Repeatedly exchanging inversions proves that increasing
most-significant-first order is minimal and decreasing order is maximal.

Among all twelve-element subsets of `U`, replacing a selected digit by a
smaller unselected digit cannot increase the minimum arrangement. Therefore
the global minimum uses the twelve smallest digits. The dual argument proves
that the global maximum uses the twelve largest digits. This establishes the
envelope without making any assumption about `y`.

The envelope deliberately ignores which digits `y` ultimately takes. That
makes it an over-approximation, which is exactly what a sound pruning rule
requires.

## 3. Root rejection criterion

For lift `j`, a completion requires some `u_y` satisfying both

```text
leaf_min <= W_j - u_y <= leaf_max
```

and

```text
0 <= u_y < L_c.
```

Equivalently, the two intervals

```text
[W_j - leaf_max, W_j - leaf_min]
```

and

```text
[0, L_c)
```

must intersect.

Thus the whole trie walk is impossible if either:

```text
W_j < leaf_min
```

or

```text
max(0, W_j - leaf_max) >= L_c.
```

This is an ordinary-integer interval test. It is not a heuristic density
gate, a marginal congruence condition, or an assumption about residue
uniformity.

## 4. Minimal implementation

The root envelope can be computed once per ordered `x`, outside the lift
loop.

```cpp
struct LeafEnvelope {
    u128 lo;
    u128 hi;
};

static LeafEnvelope leafEnvelope(uint64_t available, int P = 12) {
    std::vector<int> digits;
    while (available) {
        int d = __builtin_ctzll(available);
        available &= available - 1;
        digits.push_back(d);
    }
    std::sort(digits.begin(), digits.end());

    LeafEnvelope out{0, 0};

    // Minimum: twelve smallest, increasing from MSD to LSD.
    for (int i = 0; i < P; i++)
        out.lo = out.lo * B + digits[i];

    // Maximum: twelve largest, decreasing from MSD to LSD.
    for (int i = 0; i < P; i++)
        out.hi = out.hi * B + digits[digits.size() - 1 - i];

    return out;
}

static bool liftCouldContainLeaf(u128 W, u128 Lc,
                                 const LeafEnvelope &env) {
    if (W < env.lo)
        return false;

    u128 minimumRequiredU = W > env.hi ? W - env.hi : 0;
    return minimumRequiredU < Lc;
}
```

The existing query loop becomes:

```cpp
LeafEnvelope env = leafEnvelope(A19mask & ~xmask);

for (int j = 0; j < lifts; j++) {
    u128 W = c_x + (u128)j * L_c;
    if (!liftCouldContainLeaf(W, L_c, env))
        continue;
    walkOne(trie, W, ...);
}
```

For this fixed 19-digit instance, sorting is immaterial. A production helper
can extract the twelve lowest and highest set bits directly.

## 5. Candidate-21 behavior

For candidate 21,

```text
A19 = {1,2,3,4,5,6,8,9,10,11,12,13,14,15,16,17,18,19,20}.
```

### `1+6`

There are nineteen possible one-digit `x` values and four lifts, for 76
roots. The envelope leaves:

- `j = 0` for every `x`;
- `j = 1` for every `x`;
- `j = 2` only for `x` in `{8,17,18}`;
- no `j = 3` root.

Therefore:

```text
41 roots retained,
35 roots rejected,
46.1% rejected before trie entry.
```

This count uses only the canonical range `0 <= u_y < L_c`. A narrower exact
root range for the stored residues could only reject more.

### Existing splits

The measured root counts were:

| split | total roots | roots rejected | rejected |
|---|---:|---:|---:|
| `3+4` | 23,256 | 10,597 | 45.6% |
| `2+5` | 1,368 | 621 | 45.4% |

The visit reductions closely track the rejected-root fractions, showing that
the discarded lifts were not unusually cheap walks.

## 6. Stronger internal-node envelope

The same argument applies after some low leaf digits have been decoded.

At trie depth `d <= 12`, let:

- `leaf_fixed` be the ordinary value of the digits fixed in positions
  `0 .. d-1`;
- `R = 12 - d`;
- `U_live = A19 \ x_mask \ used_leaf_mask`.

Ignoring the still-unknown `y` mask, construct:

- the minimum completion by taking the `R` smallest digits of `U_live` and
  arranging them minimally in positions `d .. 11`;
- the maximum completion from the `R` largest digits arranged maximally.

If a node stores the minimum and maximum full `u_y` residue among its
descendants, reject when the node's residue interval does not intersect

```text
[W - leaf_max_completion, W - leaf_min_completion].
```

Measured visit counts:

| split | baseline | root only | envelope at every node | every-node envelope plus exact depth-3 masks |
|---|---:|---:|---:|---:|
| `3+4` | 60,100,471 | 32,713,540 | 29,753,377 | 28,398,966 |
| `2+5` | 23,950,821 | 13,080,525 | 11,991,871 | 11,122,534 |

The root obtains most of the gain. Checking every ordinary-trie node adds
arithmetic and requires residue-range metadata, so it is not automatically
a wall-clock win. In a Patricia implementation, evaluate the envelope only
at structural nodes or selected checkpoint depths.

## 7. Joint mask-and-envelope certificate

The depth-3 exact-mask proposal can be made genuinely target-specific.
Instead of asking two separate questions,

1. does some descendant `y` mask avoid the digits already used?
2. does the node's broad residue interval intersect the broad leaf envelope?

ask one joint question for every distinct descendant `y` mask:

1. reject the mask if it intersects `x_mask | used_leaf_mask`;
2. its complement now fixes the remaining leaf digit set exactly;
3. compute the exact rearrangement envelope of that remaining set;
4. retain the node only if at least one compatible mask's envelope intersects
   the node residue range.

Using the node's global residue range for every mask is still a sound
over-approximation; storing a separate `(minU,maxU)` per mask would be
stronger but larger.

The root sieve plus this joint depth-3 query measured:

| split | visits | reduction from baseline |
|---|---:|---:|
| `3+4` | 29,488,731 | 50.9% |
| `2+5` | 11,286,344 | 52.9% |

This nearly matches the every-node envelope while concentrating the
additional work at one selected depth.

## 8. Soundness

The root test cannot remove a completion:

1. every real `y` record has canonical residue `u_y` in `[0,L_c)`;
2. every real leaf lies in its rearrangement envelope;
3. therefore its `u_y = W_j - leaf` lies in both tested intervals;
4. an empty intersection is incompatible with a real completion.

The internal-node and joint-mask forms use the same argument with narrower
over-approximations.

A full candidate-20 diagnostic applied the stronger every-node envelope:

```text
known completion direct verification: PASS
survivors:                              1
directly verified survivors:           1
failed direct verifications:           0
known completion found:                YES
soundness gate:                         PASS
```

The direct check reconstructed the full 47-digit number and evaluated it
modulo the original `L`, independently of the trie arithmetic.

Candidate 21 continued to return zero survivors under every measured
variant.

## 9. Why this escapes the failed factor-filter program

The sieve does not ask whether the remaining permutation is feasible modulo
one or several factors of `L_c`. Those marginal conditions saturate below
the critical band.

Instead, it uses:

- the canonical full residue representative `u_y`;
- the selected lift `j`;
- the ordinary integer order on the target interval;
- the extremal geometry of a distinct-digit base-49 number.

It is therefore target-specific and uses the full residue through an
inequality rather than a bounded-order congruence. The Irreducibility Law
does not apply to this condition class.

The win is constant-factor, not a polynomial decision procedure, but it
removes almost half of the actual refutation search for essentially no
memory.

## 10. Recommended implementation order

1. Add the root-only envelope to the ordinary `2+5` carry trie.
2. Run the candidate-20 direct-verification gate.
3. Reproduce the candidate-21 `3+4` and `2+5` measurements.
4. Benchmark root-sieved `2+5` end to end.
5. Add the joint mask-and-envelope query at one selected depth.
6. Only then build the `1+6` Patricia representation.

The decision between `2+5` and `1+6` should use total build-plus-search wall
time:

```text
2+5:
    1,395,360 y records
    about 13.1 million root-sieved visits

1+6:
    19,535,040 y records
    projected 5–5.5 million root-sieved visits
```

The larger split buys roughly another 2.5-fold search reduction at the cost
of roughly fourteen times as many stored records. Path compression may make
that trade favorable, but the root-sieved `2+5` baseline is now the result it
must beat.

