# Base 50: automatic full-modulus bucket fallback

## Executive summary

Base 50 is not intrinsically orders of magnitude harder than base 49. It
exposes a decomposition failure in the current solvers.

The uniquely forced maximal-size digit set is

```text
D = {1,...,49} \ {24,25}.
```

For this set,

```text
L      = 619808900849199341280
L_nil  = 160 = 2^5 * 5
L_c    = 3873805630307495883
T      = 5
P_full = ceil(log_50 L)   = 13
P_c    = ceil(log_50 L_c) = 11
```

The jump to `T=5` is the problem. The existing peeled Engine C cannot retain
the resulting suffix-witness family, while the generalized bucket certifier
handles it by running a separate bucket search for every admissible ordered
five-digit suffix. For the first base-50 window candidate there are 11,552
such suffixes, producing about 8.33 billion mostly empty bucket lookups.

The clean remedy is not to enlarge the suffix-tuple caps. It is to add an
automatic **full-modulus bucket plan**:

> When nilpotent peeling creates too many suffix families, skip peeling and
> run the shallow bucket join directly modulo the original `L`.

For base 50, one full-`L` bucket needs about 1.86 million records and roughly
39 million bucket lookups, plus approximately 0.6 billion very cheap record
mask checks. It should turn base 50 from a pathological hours-or-days run
into a plausible seconds-to-minutes computation. More importantly, it is a
general strategy selected by a cost model, not a base-50 special case.

---

## 1. Why base 50 forces this digit set

The full alphabet is `{1,...,49}`. Its digit LCM is divisible by 50, so the
ten-rule excludes it.

To make the LCM cease to be divisible by 50, the unique `5^2` provider,
digit 25, must be removed. Removing 25 alone is not enough, because

```text
50 == 1 (mod 49).
```

Consequently the represented number is congruent modulo 49 to the sum of its
digits, independently of their arrangement. The full digit sum is

```text
1 + ... + 49 = 1225 == 0 (mod 49).
```

After removing 25, the sum is `1200 == 24 (mod 49)`, so digit 24 must also be
removed. Hence the first possible 47-digit subset is uniquely

```text
{1,...,49} \ {24,25}.
```

Subset discovery is therefore not the base-50 bottleneck. The current solver
finds this subset immediately as its first filtered survivor at size 47.

---

## 2. The nilpotent-depth cliff

For the forced set,

```text
L_nil = 2^5 * 5 = 160.
```

Base 50 contributes only one factor of 2 per radix power:

```text
v_2(50) = 1.
```

Therefore five radix powers are needed before the base power is zero modulo
`2^5`:

```text
T = min {t : 50^t == 0 (mod 160)} = 5.
```

For comparison:

| base | `L_nil` | suffix depth `T` |
|---:|---:|---:|
| 48 | 216 | 3 |
| 49 | 7 | 1 |
| 50 | 160 | 5 |

This is not an intrinsic divergence-band cliff. Using the repository's full
hardness metric, bases 49 and 50 both have

```text
m* = 21
P  = 13
W  = 21!/13! = 8,204,716,800.
```

After peeling, base 50's core band is actually somewhat smaller:

```text
m*_core = 19
P_c     = 11
W_core  = 19!/11! = 3,047,466,240.
```

The slowdown comes from how the program represents the five-digit
nilpotent condition, not from a sudden jump in the underlying `W` score.

---

## 3. How the two current paths fail

### 3.1 Engine C loses peeling

The v13/v15 peeled solver has these caps:

```c
#define MAXWITT 8192
#define MAXWITER 30000000LL
```

Its lightweight witness-liveness module supports only suffix depths 2 and 3.
At base 50 the program reports:

```text
NOTE: witness-tuple enumeration overflow base 50 k=47 s=5 — falling back to engineC
NOTE: witness module skipped base 50 — s=5 exceeds supported tuple size (3)
```

Increasing `MAXWITT` is not a sound architectural fix. Even the small
21-digit critical-window pool already contains 14,451 admissible ordered
five-digit suffixes. Over the full 47-digit set the witness family is vastly
larger. A flat list is the wrong representation.

### 3.2 The generalized certifier multiplies by every suffix

The generalized `carrytrie cert` path does support arbitrary `T`, but it
does this:

```text
for each candidate:
    enumerate every admissible ordered suffix
    for each suffix:
        build a fresh y bucket
        enumerate every x
        search the bucket
```

For base 50,

```text
T  = 5
Pc = 11
NX = 2
NY = 2
K  = 3
```

The naive critical pool is `{1,...,21}`. After candidate 21 is removed, the
remaining `{1,...,20}` has exactly 11,552 admissible ordered suffix
five-tuples modulo 160.

Each suffix leaves 15 free digits:

```text
y records per suffix       = 15P2 = 210
x values per suffix        = 15P2 = 210
lifts                      = 2
low-leaf choices per root  = 13P3 = 1716
```

Thus candidate 21 alone performs

```text
11552 * 210 * 2 * 1716
    = 8,325,757,440 bucket lookups.
```

Every individual bucket contains only 210 records but uses `50^3 = 125,000`
buckets. Reinitializing the bucket counts for all 11,552 suffixes also writes
about 5.8 GB of zero-filled counter memory cumulatively. Peak RAM can remain
small while wall-clock explodes.

---

## 4. Full-modulus shallow bucket

The bucket join does not fundamentally require peeling. It can solve the
whole critical tail directly modulo `L`.

Assume a fixed maximal descending prefix, followed by a candidate digit at
position 20. The remaining lower positions are `0..19`. Choose

```text
P  = 13    leaf positions 0..12
NY = 5     y positions 13..17
NX = 2     x positions 18..19
```

Let `C` be the contribution modulo `L` of the fixed prefix and the candidate.
Write `leaf`, `y`, and `x` as ordinary little-endian base-50 blocks. Then

```text
N == C + leaf + 50^P y + 50^(P+NY) x   (mod L).
```

For every ordered `y`, build one record

```text
u_y = 50^P y mod L
```

and store:

```text
u_y
y_mask
packed y digits
```

For every ordered `x`, compute

```text
c_x = -C - 50^(P+NY) x   (mod L).
```

A compatible leaf must satisfy

```text
leaf == c_x - u_y   (mod L)
0 <= leaf < 50^P.
```

The existing shallow-radix mechanism can find these pairs:

1. enumerate the few integer lifts `W_j = c_x + jL`;
2. enumerate the first `K` candidate leaf digits;
3. subtraction with borrow determines the required low `K` digits of `u_y`;
4. visit that radix bucket;
5. reject records whose `y_mask` intersects `x_mask` or the tentative leaf
   mask;
6. form `leaf = W_j - u_y`;
7. decode all 13 base-50 digits;
8. require the decoded leaf mask to be exactly the complement of
   `x_mask | y_mask`;
9. directly verify the reconstructed number modulo the original `L`.

No inverse of 50 modulo `L` is needed. The current implementation uses an
inverse only to recover the `y` digits from `u_y`; retaining packed `y`
digits in each record removes that requirement.

### Lift enumeration

Do not copy a fixed `0..lifts-1` loop blindly. For soundness, either derive
the exact integer `j` bounds or conservatively run

```text
j = 0 .. ceil(50^P / L)
```

inclusive and retain a result only when

```text
W_j >= u_y
W_j - u_y < 50^P.
```

This covers the wrap case where `u_y > c_x` without relying on an informal
lift-count convention.

---

## 5. Base-50 cost

For one candidate there are 20 available lower digits.

```text
y records = 20P5 = 1,860,480
x values  = 20P2 = 380
```

With `K=3`:

```text
bucket count             = 50^3 = 125,000
low-leaf choices per x   = 18P3 = 4,896
safe maximum lift roots  = 21
bucket lookups           = 380 * 21 * 4,896
                         = 39,070,080
mean records per bucket  = 1,860,480 / 125,000
                         = 14.88
naive record checks      ~= 581 million
```

Most record checks are a single mask intersection and will reject quickly.
The index is built once, not 11,552 times.

The planner may also build one `21P5 = 2,441,880`-record index over the pool
including every candidate and reject records containing the currently tested
candidate. That allows all candidate digits at position 20 to reuse one
index. The simpler per-candidate build is sufficient for the first
implementation.

The full-modulus route therefore trades:

```text
8.33 billion sparse lookups + 11,552 index builds
```

for approximately:

```text
39 million lookups + 0.6 billion compact mask checks + one index build.
```

This is a realistic computation.

---

## 6. Automatic strategy selection

The solver should evaluate search plans before allocating the index or
starting the expensive walk.

At minimum, compare:

1. **Peeled suffix-family bucket**
   - modulus `L_c`;
   - fixed suffix depth `T`;
   - one bucket search per admissible ordered suffix.
2. **Full-modulus bucket**
   - modulus `L`;
   - no fixed suffix family;
   - a larger leaf and more lifts, but one shared bucket.
3. **Low-memory scan fallback**
   - selected when both bucket plans exceed configured time or memory limits.

For each bucket plan, enumerate reasonable values of `NX`, `NY`, and `K`.
Estimate:

```text
records       = size of the stored y side
buckets       = B^K
roots         = x_count * lift_count
lookups       = roots * falling(leaf_available, K)
mean_bucket   = records / buckets
record_checks ~= lookups * mean_bucket
memory        = records * bytes_per_record
              + buckets * bytes_per_offset
              + construction scratch
```

For the peeled plan, multiply build and search work by the number of suffix
families unless the implementation shares work across them.

A practical calibrated score is

```text
score =
    c_build  * records
  + c_clear  * buckets * index_build_count
  + c_lookup * lookups
  + c_scan   * record_checks
```

where the four coefficients come from short same-machine microbenchmarks.
Memory remains a separate hard constraint.

The suffix-family count can be obtained without materializing every tuple.
An exact digit DP over

```text
(processed digit, occupied suffix-position mask, residue mod L_nil)
```

counts ordered distinct-digit suffixes in roughly

```text
O(number_of_digits * 2^T * T * L_nil)
```

time. At base 50, `2^5 * 5 * 160` is tiny. Materialize tuples only if the
selected execution plan actually needs them.

Expected choices:

| base | selected plan |
|---:|---|
| 48 | peeled bucket; suffix family is small |
| 49 | peeled bucket; suffix depth is one |
| 50 | full-modulus bucket; peeled family explodes |

---

## 7. Optional second-stage improvement: fuse suffix and y

If the full-modulus bucket is still slower than desired, retain peeling but
remove the suffix family from the outer loop.

For each admissible suffix `S` and compatible ordered `y`, build one combined
record

```text
u = B^(-T) S + B^Pc y   (mod L_c)
mask = suffix_mask | y_mask
```

and retain packed suffix and `y` digits. Then one `x` query searches every
suffix family simultaneously.

For base-50 candidate 21 this produces

```text
11552 * 15P2 = 2,425,920 combined records.
```

That is a manageable index and should be faster than the full-modulus route
because it keeps `Pc=11` and only two core lifts. It is, however, a larger
code change:

- the record carries both suffix and `y` identity;
- combined-mask compatibility must be exact;
- duplicate residue/mask records cannot be discarded blindly because their
  digit orders can produce different full values;
- reconstruction and maximum selection must retain the lexicographically
  best genuine completion.

Therefore the recommended order is:

1. implement the simpler full-`L` fallback;
2. benchmark and certify it;
3. add fused peeled records only if the remaining constant factor matters.

---

## 8. Correctness argument

For a fixed subset, prefix, and candidate:

1. Every arrangement of the remaining digits has a unique decomposition into
   `x`, `y`, and `leaf` blocks.
2. Every ordered `y` block is present in the bucket.
3. Every ordered `x` block is explicitly enumerated.
4. The congruence determines all possible ordinary leaf integers as
   `r + qL` in the interval `[0,B^P)`.
5. The radix-bucket walk enumerates every such compatible pair; bucket
   filtering only rejects impossible low-digit or mask combinations.
6. Exact base-`B` decoding verifies that the leaf consists of distinct,
   nonzero digits and uses precisely the remaining digit set.
7. Direct reconstruction and reduction modulo the original `L` independently
   verify every reported survivor.

Thus the join neither misses a valid completion nor accepts an invalid one.

Candidates at the window top are tried largest-first. The first candidate
with a verified completion is maximal for the fixed prefix. Because the fixed
prefix itself is the descending maximum and now has a completion, any number
with a different earlier digit is smaller. If no candidate completes, widen
the window by releasing one more prefix digit and re-plan.

Subset enumeration remains descending by size and lexicographic upper bound,
so the first fully certified subset completion is globally maximal.

---

## 9. Implementation checklist

### Phase A: full-modulus bucket engine

- Parameterize the generalized bucket code by:
  - modulus `M`;
  - leaf width `P`;
  - total free-window mask;
  - `NX`, `NY`, and `K`.
- Add packed ordered `y` digits to each record.
- Remove the modular-inverse reconstruction requirement.
- Implement exact or conservative-inclusive lift bounds.
- Search directly modulo `L`.
- Directly verify every survivor modulo `L`.

### Phase B: planner

- Derive both full and peeled parameters.
- Count admissible suffix families with the exact DP.
- Enumerate candidate `(NX,NY,K)` configurations.
- Estimate build time, search time, and peak memory.
- Select the cheapest plan under the memory budget.
- Print the selected plan and its estimates before execution.
- Refuse configurations whose predicted work exceeds the configured cap.

### Phase C: certification integration

- Try window-top candidates in descending order.
- Reuse the full bucket across candidates when practical.
- If no candidate succeeds, widen the free window and re-plan.
- Preserve subset-discovery ordering.
- Label candidate-only runs separately from exhaustive certificates.

### Phase D: optional fused peeling

- Build combined `(suffix,y)` records modulo `L_c`.
- Store combined masks and packed orders.
- Validate against the full-modulus result before considering it production.

---

## 10. Validation gates

Before trusting base 50:

1. Run the full-modulus bucket on bases with known answers and compare
   character-for-character.
2. For small bases, compare its complete survivor set against brute force.
3. Compare new and existing engines on bases 30–47.
4. Run full-modulus A/B checks on bases 48 and 49, even if peeling remains
   faster there.
5. Exercise wrap-sensitive cases where `u_y > c_x` to verify lift coverage.
6. For every survivor, check:
   - exact digit set;
   - nonzero distinct digits;
   - reconstructed value modulo `L` equals zero.
7. For base 50, independently reproduce:
   - dropped digits `{24,25}`;
   - `L`, `L_nil`, and `L_c`;
   - `T=5`, `P_full=13`, and `P_c=11`;
   - the planner's rejection of the 11,552-family peeled plan.

---

## Recommendation

Implement the full-modulus shallow-bucket path and the preflight planner
together. The planner prevents future bases from silently entering a
catastrophic decomposition, while the full-modulus path gives it a genuinely
different exact strategy to choose—not merely an abort-and-fallback rule.

Do not solve base 50 by raising tuple caps. Base 50 is the evidence that
nilpotent peeling must be optional.
