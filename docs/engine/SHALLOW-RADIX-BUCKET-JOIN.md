# Shallow radix buckets with direct leaf decoding

**Status:** exact prototype implemented, measured, and independently gated  
**Scope:** peeled base-49 carry join for the candidate-21 refutation  
**Baseline commit:** `f8ffae3`  
**Date:** 2026-07-22

## Executive summary

The carry-join experiment does not need a fourteen-level residue trie.

For the `2+5` split, there are

```text
19P5 = 1,395,360
```

ordered `y` records. After their first three base-49 residue digits, these
records occupy `49^3 = 117,649` possible prefixes, giving only about 11.9
records per prefix. Below that point, the existing trie consists mainly of
private chains that carry individual records to depth 14.

Replace the deep trie with:

1. a flat array of `(u_y, y_mask)` records;
2. a counting-sort directory indexed by `u_y mod 49^3`;
3. enumeration of the first three leaf digits for each `(x, lift)` query;
4. a short scan of the selected bucket;
5. direct decoding and exact verification of the complete twelve-digit leaf.

Measured candidate-21 result:

| design | build | search | total | survivors |
|---|---:|---:|---:|---:|
| ordinary `2+5` trie | included | included | 12.385 s | 0 |
| root-sieved `2+5` trie | about 4.7 s | about 1.9 s | 6.4–6.6 s | 0 |
| depth-3 flat buckets | 0.336 s | 0.389 s | **0.724 s** | 0 |

The known-YES candidate-20 branch completed in 0.717 seconds, returned one
survivor, and that survivor passed an independent direct check modulo the
original `L`.

The best split is now `2+5`, not `1+6`:

| split | bucket depth | records | build | search | total |
|---|---:|---:|---:|---:|---:|
| `3+4` | 3 | 93,024 | 0.019 s | 0.992 s | 1.011 s |
| `2+5` | 3 | 1,395,360 | 0.336 s | 0.389 s | **0.724 s** |
| `1+6` | 3 | 19,535,040 | 5.806 s | 0.379 s | 6.185 s |

The `1+6` walk is no faster in practice because the depth-3 bucket method
already reduces the `2+5` search to a few tenths of a second. Its fourteen
times larger record construction is pure overhead on this instance.

## 1. Setting

After nilpotent peeling, candidate 21 leaves nineteen free digits in
positions 1 through 19. The leaf occupies positions 1 through 12, and the
seven positions above it are divided into ordered blocks:

```text
NX + NY = 7.
```

For a fixed ordered `x`, the carry join derives a target residue `c_x`. For
an ordered `y`, define

```text
u_y = B^12 y mod L_c,       0 <= u_y < L_c.
```

For each small integer lift `j`,

```text
W_j = c_x + j L_c
```

and a completion requires the ordinary integer identity

```text
leaf = W_j - u_y.
```

The leaf must be a twelve-digit base-49 number whose digit mask is exactly

```text
A19 \ x_mask \ y_mask.
```

The [rearrangement-envelope lift sieve](LIFT-ENVELOPE-SIEVE.md) rejects
almost half of the `(x, lift)` roots before any index query. The remaining
task is to find stored `u_y` values whose low subtraction digits can begin a
valid leaf.

## 2. What the depth profile revealed

With the root envelope enabled, the ordinary `2+5` trie produced this visit
profile on candidate 21:

| trie depth | visits |
|---:|---:|
| 0 | 747 |
| 1 | 12,699 |
| 2 | 203,184 |
| 3 | 3,047,760 |
| 4 | 9,083,911 |
| 5 | 680,167 |
| 6 | 45,372 |
| 7 | 5,922 |
| 8 | 671 |
| 9 | 85 |
| 10 | 7 |
| 11–14 | 0 |

Most depth-4 nodes were pushed only to fail immediately against their
mandatory descendant `y` mask. This is the signature of an index that has
already reached record granularity:

- depth 3 has `49^3 = 117,649` possible prefixes;
- 1,395,360 records divided among them gives 11.86 records per bucket;
- one more radix digit leaves substantially fewer than one record per
  possible depth-4 prefix.

A Patricia trie would compress the private tails, but it would still retain
a node-oriented search structure. At this occupancy, scanning a contiguous
bucket of about twelve records is simpler and faster than descending any
kind of trie.

## 3. Flat bucket construction

Choose a shallow radix depth `K`. For the measured optimum,

```text
K = 3,
bucket_count = 49^3 = 117,649.
```

For every ordered `y`:

1. compute its base-49 value `y`;
2. compute the canonical residue

   ```text
   u_y = B^12 y mod L_c;
   ```

3. compute its digit mask `y_mask`;
4. assign it to

   ```text
   key = u_y mod 49^K.
   ```

Use counting sort:

```text
counts[key]++
offsets = prefix_sum(counts)
scatter records into records[offsets[key] .. offsets[key+1])
```

The resulting index consists only of:

```text
offsets[49^K + 1]
records[19PNY]
```

No dynamic child vectors, terminal vectors, recursive aggregation, unary
nodes, or per-node summaries are required.

## 4. Deriving a bucket from leaf digits

At subtraction depth `i`, let:

- `w_i` be digit `i` of `W`;
- `b_i` be the borrow entering position `i`;
- `l_i` be the chosen leaf digit;
- `u_i` be digit `i` of `u_y`.

The ordinary subtraction relation is

```text
l_i = w_i - u_i - b_i mod 49.
```

Given `w_i`, `b_i`, and the proposed leaf digit `l_i`, the required residue
digit and next borrow are unique:

```cpp
int raw = w_i - b_i - l_i;
int u_i;
int nextBorrow;

if (raw < 0) {
    u_i = raw + 49;
    nextBorrow = 1;
} else {
    u_i = raw;
    nextBorrow = 0;
}
```

Therefore an ordered choice of the first `K` leaf digits determines exactly
one shallow bucket:

```text
key = u_0 + 49 u_1 + ... + 49^(K-1) u_(K-1).
```

For each surviving `(x, lift)` root:

1. enumerate ordered `K`-permutations from the digits not used by `x`;
2. reject repeated or unavailable leaf digits during enumeration;
3. propagate the subtraction borrow;
4. look up the resulting bucket.

For `2+5`, `x` consumes two digits, so seventeen digits are initially
available to the leaf. At `K = 3`, each root performs

```text
17P3 = 4,080
```

bucket lookups.

## 5. Direct record test

For every `(u_y, y_mask)` in the selected bucket:

1. reject if

   ```text
   y_mask & (x_mask | low_leaf_mask) != 0;
   ```

2. reject if `W < u_y`;
3. compute

   ```text
   leaf = W - u_y;
   ```

4. reject if `leaf >= 49^12`;
5. decode exactly twelve base-49 digits;
6. reject zero digits, unavailable digits, or duplicates;
7. require

   ```text
   leaf_mask == A19_mask & ~x_mask & ~y_mask.
   ```

Passing this final equality proves both:

- the leaf uses precisely the complementary twelve digits;
- the three digits used to select the bucket agree with the full decode.

No approximate terminal predicate remains.

## 6. Prototype pseudocode

```cpp
struct Record {
    u128 u;
    uint64_t yMask;
};

struct BucketIndex {
    std::vector<uint32_t> offsets; // size 49^K + 1
    std::vector<Record> records;
};

void searchRoot(const BucketIndex &index,
                u128 W,
                uint64_t xMask,
                uint64_t allowed) {
    enumerateLeafPrefix(
        /* depth = */ 0,
        /* borrow = */ 0,
        /* used = */ 0,
        /* key = */ 0,
        [&](uint64_t key, uint64_t lowLeafMask) {
            for (uint32_t p = index.offsets[key];
                 p < index.offsets[key + 1];
                 ++p) {
                const Record &r = index.records[p];

                if (r.yMask & (xMask | lowLeafMask))
                    continue;
                if (W < r.u)
                    continue;

                u128 leaf = W - r.u;
                if (leaf >= Bpow12)
                    continue;

                auto decoded = decodeDistinctLeaf(leaf, allowed);
                if (!decoded.valid)
                    continue;

                uint64_t required =
                    A19mask & ~xMask & ~r.yMask;
                if (decoded.mask == required)
                    reportSurvivor(...);
            }
        });
}
```

The production implementation should avoid `std::function` in the prefix
enumerator, but the diagnostic already completes in under one second with
that overhead present.

## 7. Choosing the bucket depth

There are two opposing costs:

- a shallow directory performs few lookups but scans long buckets;
- a deep directory scans short buckets but enumerates many empty prefixes.

For `R` surviving roots and `a` initially available leaf digits, the rough
costs are:

```text
lookups(K) ≈ R * aPK

record_scans(K) ≈ lookups(K) * |Y| / 49^K.
```

Measured `2+5` candidate-21 results:

| `K` | buckets | lookups | record scans | build | search | total |
|---:|---:|---:|---:|---:|---:|---:|
| 2 | 2,401 | 203,184 | 118,080,411 | 0.334 s | 1.086 s | 1.420 s |
| **3** | **117,649** | **3,047,760** | **36,149,788** | **0.336 s** | **0.389 s** | **0.724 s** |
| 4 | 5,764,801 | 42,668,640 | 10,329,836 | 0.427 s | 0.647 s | 1.074 s |

`K = 3` is the measured optimum:

- `K = 2` is scan-bound;
- `K = 4` is empty-lookup-bound;
- `K = 3` balances the two.

The optimum should be selected from the cost model and confirmed by a short
measurement for other bases or split sizes.

## 8. Split comparison

### Candidate 21

| split | `K` | `|X|` | `|Y|` | roots rejected by envelope | lookups | scans | build | search | total |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `3+4` | 3 | 5,814 | 93,024 | 10,597 / 23,256 | 42,534,240 | 33,634,418 | 0.019 s | 0.992 s | 1.011 s |
| **`2+5`** | **3** | **342** | **1,395,360** | **621 / 1,368** | **3,047,760** | **36,149,788** | **0.336 s** | **0.389 s** | **0.724 s** |
| `1+6` | 3 | 19 | 19,535,040 | 35 / 76 | 200,736 | 33,331,276 | 5.806 s | 0.379 s | 6.185 s |

All three returned zero survivors.

The number of record scans is remarkably stable across the splits. Moving
from `2+5` to `1+6` reduces bucket lookups, but it does not materially reduce
the scan work. It merely multiplies construction by fourteen.

### Candidate 20 soundness run

For `2+5`, `K = 3`:

```text
records:             1,395,360
bucket lookups:      3,113,040
record scans:       36,919,907
mask passes:         6,353,074
survivors:                   1
directly verified:           1
verification failures:       0
build:                  0.320 s
search:                 0.397 s
total:                  0.717 s
```

The survivor was reconstructed into the full 47-digit number and evaluated
modulo the original `L`, independently of the bucket test.

## 9. Soundness proof

Assume a valid completion exists for some ordered `x`, ordered `y`, and
lift `j`.

1. The root lift survives the rearrangement envelope because its actual leaf
   lies inside that envelope.
2. The prefix enumerator visits the actual first `K` leaf digits because they
   are distinct and unavailable to neither `x` nor the leaf prefix itself.
3. Applying the subtraction recurrence to those actual leaf digits produces
   the actual low `K` digits of `u_y`.
4. Construction stored the actual `y` record in precisely that bucket.
5. Its mask is disjoint from `x` and the leaf.
6. The ordinary subtraction `W - u_y` reconstructs the actual leaf.
7. Direct decoding produces exactly the complementary digit mask.

Therefore every valid completion reaches and passes the terminal record
test. The algorithm may scan irrelevant records, but it cannot omit a valid
one.

Conversely, every reported survivor satisfies:

```text
leaf + u_y = W = c_x + j L_c
```

and exactly partitions `A19` among `x`, `y`, and the leaf. It is therefore a
valid completion of the peeled congruence. The independent candidate-20
check additionally verifies the original unpeeled divisibility condition.

## 10. Memory layout

The diagnostic used a naturally aligned structure containing a 128-bit
residue and 64-bit mask. It is larger than necessary.

At base 49:

- `u_y < L_c < 2^66`, so a residue needs 66 bits;
- the free-digit universe has nineteen elements, so a relative mask needs
  19 bits;
- `|Y| < 2^32`, so directory offsets need 32 bits.

A production structure-of-arrays layout can use:

```text
u_low[|Y|]     : uint64_t
u_high[|Y|]    : uint8_t or packed two-bit field
y_mask[|Y|]    : uint32_t
offsets[49^3+1]: uint32_t
```

Approximate storage for `2+5`:

| component | size |
|---|---:|
| 64-bit residue lows | 10.6 MiB |
| unpacked residue highs | 1.3 MiB |
| 32-bit masks | 5.3 MiB |
| bucket offsets | 0.45 MiB |
| total before alignment/scratch | about 18 MiB |

Keeping masks separate is especially useful: only about 17% of scanned
records pass the cheap mask test, so rejected records need not pull their
residues into cache.

Construction can avoid retaining two complete record arrays by either:

- generating records twice, once for counts and once for scatter;
- storing compact temporary records;
- radix-sorting a single compact record array in place.

## 11. Related micro-optimization: hoist child summaries

If a trie remains useful on another instance, test a child's mandatory mask
before pushing it:

```cpp
uint64_t newUsed = used | bit(leafDigit);
if (child.andMask & (xMask | newUsed))
    continue;
stack.push(childState);
```

On root-sieved `2+5`, this reduced counted visits from

```text
13,080,525 to 5,160,918
```

and reduced measured walk time from about 2.09 seconds to 1.45 seconds.

This is sound because it is the same `andMask` condition formerly applied
immediately after popping the child. Hoisting avoids stack traffic and node
dispatch but does not avoid iterating the edge. The flat-bucket design is
still substantially faster for this instance.

## 12. Negative result: no envelope-only build filter

It is natural to ask whether the lift envelopes can delete `y` records
during index construction.

Two versions were tested:

1. union the admissible `u_y` intervals over every `(x, lift)`;
2. for each unordered `y` mask, union only intervals from disjoint `x`
   choices using the exact complementary leaf envelope.

For `2+5`, every one of the 11,628 unordered five-digit `y` masks produced
the full interval:

```text
[0, L_c).
```

Thus no ordered `y` record can be deleted by envelope information alone.
The envelope becomes selective only after a target `x`, lift, and low leaf
prefix are fixed.

## 13. Recommended implementation plan

### Phase A: exact reference implementation

1. Add a separate shallow-bucket mode; keep the trie for comparison.
2. Implement counting-sort construction at `K = 3`.
3. Apply the root lift-envelope sieve.
4. Enumerate three leaf digits and propagate the borrow exactly.
5. Scan the selected bucket and decode the full leaf.
6. Reproduce candidate 21 with zero survivors.
7. Reproduce candidate 20 with one directly verified survivor.

### Phase B: production packing

1. Replace absolute 64-bit digit masks with relative 19-bit masks.
2. Store masks and residues in separate arrays.
3. Pack the 66-bit residues.
4. Remove callback and `std::function` overhead from prefix enumeration.
5. Compute `u_y` from precomputed position contributions rather than the
   generic binary `mulmod` routine.
6. Record build, lookup, scan, mask-pass, and decode counts.

### Phase C: generalize

For each future instance:

1. estimate `lookups(K)` and `record_scans(K)`;
2. benchmark the two nearest integer values of `K`;
3. retain the deep trie only when shallow buckets become too large to scan;
4. use Patricia compression only in the intermediate regime where neither a
   short flat bucket nor a dense directory is appropriate.

## 14. Revised recommendation

For the peeled base-49 window:

```text
Use split:        2+5
Use index:        3-digit radix directory + flat records
Use root prune:   rearrangement-envelope lift sieve
Use terminal:     direct twelve-digit leaf decode
```

Do not construct the `1+6` Patricia trie for this refutation unless the
production shallow-bucket result differs radically from the diagnostic.

The Patricia avenue remains useful as a general middle-density data
structure, but this instance lies on the sparse side of its useful regime:
after three radix digits, direct record inspection is already cheaper than
continuing the tree.

