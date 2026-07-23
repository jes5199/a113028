# Path-compressed carry trie: reopening the 1+6 split

**Status:** proposed implementation, with two supporting measurements complete  
**Scope:** practical exact acceleration of the peeled base-49 window branch; not a polynomial-time result  
**Baseline commit:** `f8ffae3`  
**Date:** 2026-07-22

## Executive summary

The carry-trie campaign rejected the `1+6` split because the existing trie
representation projected to roughly 199 million nodes, or about 16 GB at the
measured marginal cost of 80.7 bytes per node. The search projection itself
was attractive: the fitted exponent predicts about 10 million trie visits,
versus roughly 24–29 million for `2+5` and 60–70 million for `3+4`.

The memory rejection is an artifact of representing every radix digit as a
heap-allocated trie node. Direct measurements show that about 89% of the nodes
in both existing tries are nonterminal unary nodes:

| split | ordinary nodes | unary nodes | branching nodes | terminals | Patricia structural-node upper count |
|---|---:|---:|---:|---:|---:|
| `3+4` | 1,093,819 | 976,363 | 24,432 | 93,024 | 117,457 |
| `2+5` | 15,300,916 | 13,628,413 | 277,143 | 1,395,360 | 1,672,504 |

A Patricia/radix trie collapses every maximal unary path into one edge while
retaining all of its base-49 digits. The measured reductions are 9.3x and
9.1x. More generally, a compact trie over `Y` distinct fixed-length keys has
at most `2Y - 1` structural nodes. For `1+6`,

```text
Y = 19P6 = 19,535,040,
```

so the rigorous structural-node bound is below 39.1 million, not 199
million. Occupancy estimates suggest roughly 20–30 million in this instance.
A flat packed implementation should therefore fit within the former 8 GB
ceiling, plausibly in 1–3 GB including records and sorting scratch space.

This reopens `1+6`. It does not change the carry-trie's asymptotic verdict,
but it may provide a useful additional constant-factor win at the practical
frontier.

There is also a smaller compatible improvement: at trie depth 3, retain the
exact set of descendant `y` digit masks rather than only their union and
intersection. This exact disjointness query removed another 6–7% of visits in
the existing splits and improved wall time by 4–6% in a diagnostic
implementation.

## 1. Problem decomposition

After nilpotent peeling, the base-49 candidate branch has:

- 19 free digits;
- an exact leaf of `P = 12` low positions;
- seven positions outside the leaf;
- a coprime modulus

```text
L_c = 63245806209101973600;
```

- four leaf lifts, since `ceil(49^12 / L_c) = 4`.

The carry-trie experiment divides the seven nonleaf positions into:

- an explicitly enumerated top block `x` of size `NX`;
- a precomputed middle block `y` of size `NY`;
- the exact 12-digit decoded leaf;

with `NX + NY = 7`.

For every `x` and lift, the trie walks the radix digits of the stored
`u_y = 49^12 y (mod L_c)`. Subtracting those digits from the forced value
determines the leaf digits, including the borrow bit. A path survives only
when:

1. every decoded leaf digit belongs to the remaining pool;
2. decoded leaf digits are distinct;
3. the `x`, `y`, and leaf masks form the exact 19-digit partition;
4. the subtraction terminates with the required zero high digits and borrow;
5. terminal arithmetic verification succeeds.

The implementation is exact; the trie is an indexing device, not a
relaxation.

## 2. Why 1+6 is attractive

The list sizes at base 49 are:

| split | `|X|` | `|Y|` |
|---|---:|---:|
| `3+4` | 5,814 | 93,024 |
| `2+5` | 342 | 1,395,360 |
| `1+6` | 19 | 19,535,040 |

The measured carry-trie law is approximately

```text
visits per x = C * |Y|^alpha,    alpha ~= 0.7065.
```

Using the campaign fit gives:

| split | projected total visits |
|---|---:|
| `3+4` | about 60–70 million |
| `2+5` | about 24–29 million |
| `1+6` | about 10 million |

Moving from `2+5` to `1+6` increases `Y` by 14x but reduces `X` by 18x.
Because the observed trie cost is sublinear in `Y`, the projected search
work falls by roughly 3x.

The open question is end-to-end cost. `1+6` must generate and index 19.5
million records, so a slow builder could erase the search gain. The proposed
representation is designed to make both construction and traversal
sequential and cache-friendly.

## 3. Why the ordinary trie wastes memory

The present `TrieNode` owns a dynamic vector of child pairs plus mask and
moment summaries. Once the random-looking residue strings have separated,
most remaining prefixes lead to exactly one terminal. Nevertheless, the
ordinary trie allocates one full node for every remaining base-49 digit.

The measured shapes are:

```text
3+4:
    ordinary nodes  = 1,093,819
    unary nodes     =   976,363  (89.3%)
    branch nodes    =    24,432
    terminals       =    93,024

2+5:
    ordinary nodes  = 15,300,916
    unary nodes     = 13,628,413  (89.1%)
    branch nodes    =    277,143
    terminals       =  1,395,360
```

For fixed-length distinct keys, deleting every nonterminal unary node leaves
only:

```text
root + branching nodes + terminal nodes.
```

That produces the 9x reductions in the summary table. The phenomenon becomes
more valuable, not less, at `NY = 6`: after the first few radix digits,
almost every one of the 19.5 million keys follows a private unary tail.

## 4. Proposed packed Patricia representation

### 4.1 Flat records

Generate one record per ordered `y` arrangement:

```text
(radix_key, y_mask)
```

where:

- `radix_key` is the 14-digit, least-significant-digit-first base-49
  representation of `u_y`;
- `y_mask` is the 19-bit set of digits used by `y`.

The least-significant-digit-first order is essential because subtraction and
borrow propagation proceed from the low end.

The ordered `y` arrangement itself need not be stored. Since
`gcd(49, L_c) = 1`, multiplication by `49^12` is invertible modulo `L_c`.
Also `y < 49^6 < L_c`, so distinct ordered `y` values produce distinct
`u_y` values. The radix key therefore identifies the ordered arrangement,
and the terminal needs only its digit mask.

### 4.2 Sort instead of pointer insertion

Radix-sort the flat records by their 14 radix digits. This avoids constructing
the 199-million-node intermediate trie.

Suitable implementations include:

- fourteen stable counting-sort passes over digits `0..48`;
- fewer passes over packed groups of digits;
- an in-place MSD radix sort that emits compact trie nodes directly.

Comparison sorting is acceptable for a prototype, but a radix builder is the
intended production path.

### 4.3 Build from longest common prefixes

Scan the sorted keys and construct an implicit Patricia trie from adjacent
longest-common-prefix lengths. A node needs only compact fields such as:

```text
first_child
child_count
edge_digit_offset
edge_digit_length
terminal_y_mask
```

Child labels and compressed edge digits live in global packed arrays. No node
owns a dynamic allocation.

An alternative is to retain only the sorted record array plus an LCP index
and treat subranges as implicit trie nodes. The explicit packed form is likely
faster during repeated walks; the implicit form is useful as a minimal first
prototype.

### 4.4 Traverse compressed edges soundly

Path compression must not skip semantic work. For every digit stored on a
compressed edge, the walker still:

1. subtracts the corresponding `u_y` digit and borrow from the forced digit;
2. updates the borrow;
3. checks leaf membership and distinctness while depth is below 12;
4. requires zero output above the leaf;
5. updates the live leaf mask and any enabled summaries.

Compression removes node dispatch, vector chasing, and stack frames along a
unary chain. It does not replace several radix transitions with one
arithmetic approximation.

## 5. Exact descendant-mask pruning

### 5.1 Existing rung 1

The existing mask summary stores:

- `andMask`: digits present in every descendant `y` mask;
- `orMask`: digits present in at least one descendant `y` mask.

This catches mandatory-digit conflicts and, after the leaf mask is fixed,
some missing-digit conflicts. It does not answer the exact question:

```text
Does this node have any descendant y mask disjoint from
the digits already consumed by x and the partial leaf?
```

Different descendant masks may each hit a different forbidden digit. Their
union and intersection can both look harmless even though no compatible mask
exists.

### 5.2 Exact query

For a trie node `v`, let `D(v)` be its set of descendant `y` masks and let

```text
F = x_mask | partial_leaf_mask.
```

The exact necessary condition is:

```text
exists Y in D(v) such that (Y & F) == 0.
```

At completed leaf depth it is also the exact mask-partition condition, because
the fixed block sizes sum to all 19 free digits.

For `NY = 4` there are only

```text
C(19,4) = 3,876
```

possible masks; for `NY = 6` there are

```text
C(19,6) = 27,132.
```

A diagnostic implementation pooled sorted unique descendant masks. Querying
at every level was too expensive. Nearly all useful pruning occurred at depth
3, where descendant lists are already small enough to scan efficiently.

Production `1+6` should therefore store exact mask lists only for selected
cut layers, initially depth 3. Before deduplication, a single selected layer
stores at most one mask entry per `y` record—about 19.5 million entries, or
156 MB with 64-bit masks. Deduplication within each depth-3 prefix can reduce
that substantially.

### 5.3 Measured effect on existing splits

The following comparisons used the same temporary diagnostic build in each
pair. The exact-mask mode retained ordinary rung 1 and omitted the previously
measured inert moment checks.

| split | rung-1 visits | exact depth-3 visits | visit reduction | rung-1 wall | exact wall |
|---|---:|---:|---:|---:|---:|
| `3+4` | 60,100,471 | 56,306,188 | 6.3% | 4.447 s | 4.253 s |
| `2+5` | 23,950,821 | 22,182,110 | 7.4% | 12.385 s | 11.648 s |

The wall improvements were approximately 4–6%. These timings are paired
diagnostic measurements, not replacements for the campaign's published
baseline timings.

The candidate-20 soundness gate passed:

```text
known completion direct verification: PASS
survivors: 1
direct-arithmetic verification: 1 OK, 0 FAILED
known completion found: YES
```

Candidate 21 remained at zero survivors.

The exact-mask gain must be remeasured at `1+6`; the different `x` and `y`
mask sizes may change its selectivity.

## 6. Memory estimate for 1+6

The exact layout should be measured, but a reasonable budget is:

| component | rough scale |
|---|---:|
| 19.5M flat `(key,mask)` records at 16–24 bytes | 0.31–0.47 GB |
| 20–39M packed structural nodes at 12–20 bytes | 0.24–0.78 GB |
| packed edge digits and child labels | 0.1–0.4 GB |
| optional depth-3 exact-mask pool | at most ~0.16 GB before overhead |
| radix-sort scratch and construction workspace | implementation-dependent |

This supports a working expectation of 1–3 GB and a hard design goal below
4 GB. Even the rigorous 39.1-million-node structural bound is compatible with
the former 8 GB ceiling when nodes are flat and packed.

The design must not temporarily materialize the ordinary trie: doing so would
reintroduce the original 16 GB failure before compression has a chance to
help.

## 7. Construction and traversal optimizations

These are secondary to getting a sound compact prototype:

1. **Permutation generation by adjacent swaps.** A Gray-code permutation
   order allows `y_val` to be updated from the previous arrangement using one
   adjacent-swap delta instead of recomputing six weighted digits.
2. **Packed radix edges.** Six bits encode a base-49 digit. Fourteen digits fit
   in 84 bits and can be stored in two machine words.
3. **Small-stack traversal.** The compressed tree has bounded semantic depth
   14. Use a fixed local stack rather than heap allocation.
4. **Specialized unary-edge loop.** Most work occurs while checking compressed
   private tails. Keep `W` digits, borrow, and leaf mask in registers.
5. **Parallel `x` walks.** The 19 top-digit branches and four lifts are
   independent after trie construction.
6. **Parallel/radix construction.** Record generation and radix buckets can be
   partitioned without changing search semantics.

## 8. Negative control: full high-order per-mask tables

Another apparent opportunity was tested and should not distract this
experiment.

For each prime-power factor `q < 49`, one can precompute a 64-bit reachable
residue set for every one of the `2^19` remaining-digit masks:

```text
R_q[mask] =
    union over d in mask of
    rotate(R_q[mask \ {d}], d * 49^(|mask|-1) mod q).
```

All 14 tables would occupy only about 59 MB and take roughly
`14 * 19 * 2^19` word operations. This looks like an inexpensive way to
enable every high-order marginal at every DFS node.

It does not help above the exact leaf. On the candidate-21 digit pool, the
non-invariant factors reach every residue for every mask by these layer
sizes:

| factor | every mask saturated by |
|---:|---:|
| 11 | 6 digits |
| 13 | 7 |
| 17 | 6 |
| 19 | 7 |
| 23 | 6 |
| 25 | 10 |
| 29 | 6 |
| 31 | 6 |
| 37 | 6 |
| 41 | 6 |
| 43 | 9 |
| 47 | 6 |

The factors 32 and 27 retain invariant cosets, already represented by the
existing low-order conditions. Since the exact radix leaf begins at 12
digits, all new high-order marginal tables are saturated before they can
prune. This is a clean negative result and agrees with the irreducibility
law.

Path compression is different: it changes the cost and feasible split of the
exact join rather than adding another saturated marginal condition.

## 9. Implementation sequence

### Phase A: compact-builder validation on 3+4

1. Emit flat `(radix_key, y_mask)` records for `NY = 4`.
2. Sort and build the compact representation.
3. Reproduce exactly the existing 93,024 terminals.
4. Reproduce candidate 20's unique survivor and candidate 21's zero
   survivors.
5. Compare compact node counts against the measured 117,457 structural upper
   count.
6. Compare build time, walk time, and peak RSS with the ordinary trie.

This phase is small enough to debug with full cross-checks.

### Phase B: 2+5 confirmation

Repeat at `NY = 5`. The expected compact structural count is about 1.67
million. This validates that construction remains linear and memory does not
hide a per-terminal multiplier.

### Phase C: 1+6 measurement

1. Build all 19,535,040 records without constructing an ordinary trie.
2. Record generation time, sorting time, compact-build time, node counts, and
   peak RSS separately.
3. Run candidate 20 first and require the unique known completion.
4. Run candidate 21 and require zero survivors.
5. Add the selected-layer exact-mask pool only after the compact baseline is
   sound.
6. Attribute its effect separately.

### Phase D: frontier projection

Fit construction and traversal costs separately. The old alpha fit models
walk visits but not the enlarged `NY = 6` build. Project both components to
the prime-base frontier before adopting the design.

## 10. Go/no-go criteria

Suggested practical gate for base 49:

### Soundness gate

- candidate 20: exactly one survivor;
- known completion present;
- every survivor passes direct arithmetic verification;
- candidate 21: zero survivors.

Any failure is an immediate stop.

### Memory gate

- **GO:** peak RSS at most 4 GB;
- **HOLD:** 4–8 GB with a clear removable construction copy;
- **NO-GO:** above 8 GB or requires materializing the ordinary trie.

### End-to-end wall gate

Compare full build plus search against the best `2+5` implementation on the
same machine:

- **GO:** at most 0.6x the `2+5` wall time;
- **HOLD:** 0.6–0.9x, pending frontier projection or parallel build;
- **NO-GO:** at least 0.9x after basic packing and radix construction.

The search-only projection is not sufficient: a 3x walk reduction is not a
win if the 14x-larger `y` list makes construction dominant.

## 11. Expected result and limitations

The strongest defensible expectation is:

- the `1+6` representation fits;
- search visits fall to approximately 10 million;
- compact traversal improves locality and unary-chain overhead;
- construction becomes the main uncertainty;
- end-to-end improvement is likely a few-fold, not 30x.

This avenue does not produce a polynomial decision procedure and does not
reopen the bounded-order filtering program. It is a practical exact-search
improvement obtained by:

1. moving one more position into the empirically sublinear side of the join;
2. removing a representation-induced memory barrier;
3. adding a small exact mask-compatibility test at the one layer where it has
   measured leverage.

That is enough to justify a contained implementation experiment.
