# Admission control for the shallow radix-bucket join

**Status:** design recommendation  
**Scope:** autonomous `carrytrie cert` driver, bases 41–49 and future bases  
**Code baseline:** `6832916`  
**Date:** 2026-07-23

## Executive summary

The shallow radix-bucket join should not be selected by base number, and it
should never discover that it is too large by constructing the index.

Its memory requirement is almost completely predictable before allocation:

```text
Y = a P NY
Q = B^K
```

where:

- `a` is the number of free digits after fixing the candidate and peeled
  suffix;
- `NY` is the number of high-window positions indexed on the `y` side;
- `K` is the radix-directory depth;
- `Y` is the number of ordered records;
- `Q` is the number of buckets.

For the current autonomous driver,

```text
a = Pc + NX + NY = 20 - T
NX + NY = 20 - T - Pc.
```

These quantities are known immediately after deriving `(T, Pc)` for a
candidate digit set.

The right policy has three gates:

1. **Memory gate:** reject every `(NX, NY, K)` whose projected peak memory,
   with headroom, exceeds the smaller of the configured RSS budget and
   available system memory.
2. **Work gate:** among the configurations that fit, reject those whose
   projected construction, lookup, and record-scan work is not competitive
   with the scan engine.
3. **Bounded pilot:** for borderline configurations, sample residue-bucket
   and query-key distributions for at most a small fixed time, then either
   admit the bucket join or decline it.

“Declined” must be a separate result from “refuted.” Declining to run an
algorithm proves nothing about the mathematical branch. It must cause an
exact fallback engine to run.

This would have rejected base 41 before allocating a byte of bucket storage.
Its default split creates

```text
19P7 = 253,955,520 records.
```

The current builder reached 8,932,744 KB peak RSS before its first budget
check and aborted after about 198 seconds. Had it continued through the
scatter phase, its temporary representation would have required roughly
16.6 GiB. Even a much better two-pass packed builder would remain too large
for the 3 GB budget.

It also would not blindly endorse base 42 merely because it fits. Base 42
uses only

```text
15P4 = 32,760 records,
```

but the full bucket certificate took 358.3 seconds, versus 72.6 seconds for
the existing scan engine. Memory feasibility and algorithmic advantage are
different decisions.

## 1. What controls bucket size

The generalized certificate driver fixes the candidate at absolute position
20. After peeling a suffix of length `T`, the coprime leaf has length `Pc`.
The remaining high-window width is

```text
W = NX + NY = 20 - T - Pc.
```

After selecting one suffix tuple, the free pool used by the join has size

```text
a = Pc + W = 20 - T.
```

The bucket builder materializes every ordered `NY`-permutation from this
pool:

```text
Y(a, NY) = a! / (a - NY)!.
```

That falling factorial, not `B` by itself, is the dangerous quantity.
Changing `T` or `Pc` by one can change `NY`, and therefore `Y`, by a factor
near `a`.

The current default is:

```text
NX = 2
NY = W - 2
K  = 3.
```

That default is excellent for bases 48 and 49, but catastrophic for base 41:

| base | `T` | `Pc` | `a` | default split | records |
|---:|---:|---:|---:|---:|---:|
| 41 | 1 | 10 | 19 | `2+7` | 253,955,520 |
| 42 | 5 | 9 | 15 | `2+4` | 32,760 |
| 48 | 3 | 11 | 17 | `2+4` | 57,120 |
| 49 | 1 | 12 | 19 | `2+5` | 1,395,360 |

The non-monotonicity is real. Base 42 is structurally tiny while the smaller
base 41 is enormous.

## 2. Why the present RSS guard is too late

`buildBucketIndexGen` currently constructs:

```text
raw       : vector<BucketRecordGen>
key       : vector<uint32_t>
counts    : vector<uint32_t>
records   : final vector<BucketRecordGen>
offsets   : final vector<uint32_t>
cursor    : temporary copy of offsets
```

On this ABI:

```text
sizeof(BucketRecordGen) = 32 bytes
```

because the 128-bit residue gives the structure 16-byte alignment.

Let:

```text
R = sizeof(BucketRecordGen)
C = capacity reached by the growing raw/key vectors
Q = B^K.
```

Near the scatter phase, the current additional storage is approximately:

```text
C(R + 4) + YR + 12(Q + 1) bytes.
```

`C` may be substantially larger than `Y` because neither growing vector is
reserved. With the present libstdc++ growth pattern, base 41 reaches a
capacity near `2^28` records.

The existing call:

```cpp
checkRssBudget(rssBudgetKB, "certdrv-bucket-post-generation");
```

runs only after all `Y` records have been generated and their pages touched.
It can report an overrun cleanly, but it cannot prevent one. Linux memory
overcommit also means successful `resize` or `reserve` calls are not a
reliable safety signal; the process can be killed later while touching the
allocation.

The RSS guard should remain as a backstop. It should not be the planner.

## 3. An exact preflight memory gate

Use checked integer arithmetic to calculate:

```text
Y = falling_factorial(a, NY)
Q = checked_power(B, K).
```

Reject a configuration immediately if:

- either calculation overflows;
- `Y > UINT32_MAX` while the index uses 32-bit offsets;
- any single bucket count could overflow its counter type;
- the projected peak plus headroom exceeds the memory budget.

The estimate should be based on the actual builder representation, using
`sizeof` rather than copied constants.

For the current builder, a conservative estimate is:

```text
current_peak =
    vector_capacity_bound(Y) * (sizeof(BucketRecordGen) + sizeof(uint32_t))
  + Y * sizeof(BucketRecordGen)
  + 3 * (Q + 1) * sizeof(uint32_t)
  + allocator_headroom.
```

Because portable C++ does not specify vector growth, this is awkward to make
tight. That is itself a reason to change the builder.

### Recommended two-pass builder

Make construction predictable:

1. enumerate `y` once and tally bucket counts;
2. prefix-sum the counts;
3. allocate the final record arrays exactly once;
4. enumerate `y` again and scatter directly into their final positions.

This removes `raw` and `key`. If the prefix-summed `counts` vector is moved
into `offsets`, only one temporary cursor directory is needed.

The peak becomes approximately:

```text
two_pass_AoS_peak =
    Y * sizeof(BucketRecordGen)
  + 2 * (Q + 1) * sizeof(uint32_t)
  + allocator_headroom.
```

A structure-of-arrays layout can reduce it further:

```text
u[Y]             : u128
y_relative_mask  : uint32_t
offsets[Q+1]     : uint32_t
cursor[Q+1]      : uint32_t
```

Since the free pool has at most twenty digits in this driver, the `y` mask
can be relative to the pool rather than an absolute 64-bit digit mask. The
packed estimate is then:

```text
packed_peak ≈ 20Y + 8(Q + 1) bytes
```

before allocator and process headroom.

This optimization helps many intermediate cases, but it does not rescue
base 41:

| base-41 `2+7`, `K=3` representation | projected peak |
|---|---:|
| current grow-and-scatter builder | about 16.6 GiB |
| two-pass, 32-byte records | about 7.6 GiB |
| two-pass, `u128` + relative 32-bit mask | about 4.7 GiB |
| configured RSS budget | about 2.8 GiB |

The correct decision remains “do not build.”

### Budget comparison

Treat the CLI RSS budget as a total-process ceiling, not as space available
to the index:

```text
available_for_index =
    min(configured_peak_budget, system_memory_limit)
  - current_RSS
  - fixed_safety_reserve.
```

Then require, for example:

```text
1.15 * projected_index_peak <= available_for_index.
```

The 15% factor covers allocator metadata, vector objects, stacks, temporary
suffix data, and estimation drift. A fixed reserve such as 128–256 MiB is
also reasonable. When running beside a memory-sensitive service, use its
explicit operational headroom rather than `/proc/meminfo` alone.

## 4. Search every legal split instead of hard-coding `2+NY`

For a fixed window width `W`, enumerate:

```text
NX = 0 .. W - 1
NY = W - NX
```

and several directory depths:

```text
K = 1 .. min(Pc, a - NX),
```

stopping when `B^K` alone violates the memory budget.

For each `(NX, NY, K)`, calculate memory first. Discard infeasible plans
without a pilot.

Moving one position from `y` to `x` divides the record count by roughly
`a - NY + 1`, so it can rescue memory dramatically. It also multiplies the
number of `x` roots and leaf-prefix queries. A split that fits can therefore
be much slower.

Base 41 illustrates the trap:

```text
2+7: 253,955,520 y records
3+6:  19,535,040 y records
4+5:   1,395,360 y records
5+4:      93,024 y records
```

The `3+6` split could fit after the two-pass rewrite, but at `K=3` its
conservative per-suffix estimate is roughly:

```text
60 million bucket lookups
16.6 billion record scans.
```

Base 41 has `T=1` and `L_nil=1`, so the first candidate has about twenty
admissible one-digit suffix choices. Multiplying by those suffix branches
makes the plan plainly uncompetitive. Changing the split solves the
allocation problem, not the algorithm-selection problem.

## 5. A work model for the second gate

For one suffix branch, define:

```text
X       = a P NX
Y       = a P NY
prefix  = (a - NX) P K
Q       = B^K
```

Ignoring the root sieve, the number of bucket queries is:

```text
lookups ≈ X * lifts * prefix.
```

Under a uniform-residue approximation:

```text
record_scans ≈ lookups * Y / Q.
```

Construction work is:

```text
build_records =
    Y       for the present one-pass temporary builder,
    2Y      for the recommended two-pass builder.
```

The two-pass method deliberately trades extra arithmetic for a much smaller
and predictable memory footprint.

For a whole candidate, multiply these quantities by the number of admissible
suffix tuples. For the autonomous subset search, also account for the
number of wrong-turn candidates and subset hypotheses likely to be tried.
The suffix-tuple count is exact and already computed by the driver; it
should never be omitted from the estimate.

The uniform model is sufficient to reject wildly bad plans. It is not
accurate enough to choose between close plans because bucket occupancy and
query keys can be correlated.

## 6. A better bounded pilot: estimate the actual join dot product

The exact number of scanned records is:

```text
scans = sum over keys q of
            query_count[q] * record_count[q].
```

This suggests a cheap, target-specific pilot.

### Sample the record distribution

Draw a deterministic sample of ordered `y` permutations and count their
radix keys. Do not recurse through the first `S` lexicographic permutations,
which can be biased. Rank/unrank evenly spaced permutation indices, or use a
fixed-seed pseudorandom sampler without replacement.

### Sample the query distribution

Draw a deterministic sample of `x` permutations and lifts:

1. apply the real root-envelope sieve;
2. enumerate the actual `K` leaf prefixes;
3. count the resulting query keys.

This directly measures the surviving-lift fraction and the nonuniform query
distribution.

### Estimate the dot product

Scale both sampled histograms to their full population sizes and compute
their dot product. Bootstrap or repeat with several fixed seeds to obtain an
uncertainty band.

This is stronger than multiplying two global averages. It detects exactly
the correlations that can make a nominally uniform bucket estimate too high
or too low.

The pilot should be bounded by both operations and time, for example:

```text
at most 100,000 sampled y records
at most 256 sampled x roots
at most 1 second total
```

Those are starting values to calibrate, not mathematical constants. If the
confidence interval straddles the admission threshold, decline the bucket
join. The scan fallback is already available; uncertainty does not need to
be resolved by risking a full build.

## 7. Convert work into predicted time

Record three machine-local throughput estimates:

```text
y_records_generated_per_second
bucket_lookups_per_second
record_scans_per_second.
```

Refresh them from completed runs using an exponentially weighted average,
or ship conservative values derived from the scorecard machine and update
them online.

Then estimate:

```text
bucket_seconds =
    build_records / generation_rate
  + lookups       / lookup_rate
  + scans         / scan_rate
  + fixed_overhead.
```

Compare this with a scan-engine estimate. Possible scan estimates, in order
of preference, are:

1. a calibrated model already available from the DFS/scan engine;
2. a short bounded scan pilot;
3. a historical per-base/per-structure estimate;
4. a conservative absolute bucket time cap when no comparison exists.

Use a margin. A reasonable initial rule is:

```text
admit only if upper_confidence_bound(bucket_seconds)
              < 0.6 * predicted_scan_seconds.
```

The exact ratio should be calibrated from the sweep. The point is to require
a clear expected win, because a false negative merely chooses the known
exact fallback while a false positive can consume minutes or gigabytes.

Base 42 is the motivating example:

```text
bucket certificate: 358.3 s
scan engine:          72.6 s
```

It passes the memory gate and should fail the advantage gate.

## 8. Two-stage planning

Run admission control twice.

### Structural preflight

Immediately after `deriveConstantsGen`, before any bucket allocation:

1. derive `a` and `W`;
2. enumerate `(NX, NY, K)`;
3. compute exact record and directory sizes;
4. compute conservative lower bounds on build and query work;
5. decline the bucket family if every configuration is clearly bad.

This stage is independent of the particular suffix tuple.

### Branch preflight

After the prefix, candidate, and admissible suffix tuples are known:

1. multiply by the exact tuple count;
2. sample root-sieve survival and key correlations;
3. predict full wall time;
4. select the best admitted configuration or decline.

Cache plans by structural signature:

```text
(B, T, Pc, a, W, lifts, memory_budget)
```

and cache pilot results when the candidate/target dependence permits it.
Do not repeat the same planning work for every suffix tuple.

## 9. Sound fallback semantics

The planner needs a result type that cannot confuse resource policy with
mathematics:

```cpp
enum class AttemptStatus {
    Certified,  // at least one directly verified survivor
    Refuted,    // exhaustive exact search found none
    Declined    // this engine was not run
};
```

If all bucket configurations are rejected:

```text
AttemptStatus::Declined
```

must propagate out of `runWrongTurnSearch` and `runCert`. It must not:

- increment `wrongTurnsRefuted`;
- advance to a smaller candidate as though the current one were impossible;
- advance to another digit subset as though the current subset were
  impossible;
- produce a certificate.

The top-level sweep should recognize a distinct exit status such as:

```text
3 = BUCKET_DECLINED
```

and invoke the exact scan fallback. The existing RSS abort can remain exit
status 2 and should also fall back.

Algorithm selection is then outside the proof:

```text
planner chooses an exact engine
        |
        +-- bucket admitted --> exact bucket certificate/refutation
        |
        +-- bucket declined --> exact scan certificate/refutation
```

A mistaken performance prediction can waste time or miss a speedup, but it
cannot change the answer.

## 10. Planner sketch

```cpp
struct BucketPlan {
    int NX;
    int NY;
    int K;

    u128 yRecords;
    u128 bucketCount;
    u128 projectedPeakBytes;
    long double projectedLookups;
    long double projectedScans;
    double projectedSeconds;
};

enum class BucketDecisionKind {
    Admit,
    DeclineMemory,
    DeclineWork,
    DeclineUncertain
};

struct BucketDecision {
    BucketDecisionKind kind;
    std::optional<BucketPlan> plan;
    std::string reason;
};
```

The structural planner:

```cpp
BucketDecision planBucket(const ConstantsGen &c,
                          uint64_t totalPeakBudgetBytes,
                          uint64_t currentRssBytes) {
    std::vector<BucketPlan> feasible;

    int a = 20 - c.T;
    int W = 20 - c.T - c.Pc;

    for (int NX = 0; NX < W; ++NX) {
        int NY = W - NX;

        auto y = checkedFallingFactorial(a, NY);
        if (!y || *y > UINT32_MAX)
            continue;

        for (int K = 1; K <= c.Pc && K <= a - NX; ++K) {
            auto q = checkedPower(c.B, K);
            if (!q)
                break;

            auto bytes = checkedPackedPeak(*y, *q);
            if (!bytes || !fitsWithHeadroom(*bytes,
                                            totalPeakBudgetBytes,
                                            currentRssBytes))
                continue;

            auto work = conservativeWork(c, a, NX, NY, K, *y, *q);
            feasible.push_back(makePlan(...));

            if (directoryAloneExceedsBudgetAtNextK(...))
                break;
        }
    }

    if (feasible.empty())
        return declineMemory(...);

    erasePlansClearlyWorseThanScan(feasible);
    if (feasible.empty())
        return declineWork(...);

    return pilotAndChoose(feasible);
}
```

All byte and count expressions should use checked `u128` arithmetic and
saturate to “infeasible” on overflow.

## 11. Required diagnostics

Every decision should leave a compact, reproducible trace:

```text
[bucket-plan] B=41 T=1 Pc=10 lifts=3 a=19 W=9 budget=3000000KB
[bucket-plan] reject NX=2 NY=7 K=3:
              Y=253955520 Q=68921 packedPeak=4.7GiB > safeBudget=2.5GiB
[bucket-plan] reject NX=3 NY=6 K=3:
              projected candidate work=1.2B lookups + 332B scans
[bucket-plan] decision=DECLINE_WORK fallback=scan
```

For an admitted plan:

```text
[bucket-plan] decision=ADMIT NX=2 NY=5 K=3
              Y=1395360 peak=... pilotScans=... predicted=...
```

At completion, log predicted versus actual counts and seconds. Those
residuals are the data needed to improve the next decision.

## 12. Recommended implementation order

### Phase A: stop crashes immediately

1. Add checked `a P NY` and `B^K` helpers.
2. Compute the present builder's conservative peak before
   `buildBucketIndexGen`.
3. Add explicit `Declined` propagation and a distinct process exit status.
4. Teach `bucket_sweep2.sh` to interpret that status as an intentional scan
   fallback.
5. Keep the current runtime RSS and `ulimit` guards as backstops.

This phase alone converts base 41 from a 198-second, 8.9-GB failed attempt
into a millisecond-scale policy decision.

### Phase B: make feasible builds predictable

1. Replace `raw + key + scatter` with the two-pass builder.
2. Move the prefix-summed counts vector into `offsets`.
3. Use a relative 32-bit `y` mask in a structure-of-arrays layout.
4. Validate candidate 20/21 gates and direct verification unchanged.

### Phase C: select for speed

1. Enumerate all memory-feasible `(NX, NY, K)` configurations.
2. Add the conservative analytic work model.
3. Add the bounded key-distribution pilot.
4. Calibrate build/lookup/scan throughput from completed runs.
5. Compare against a bounded scan estimate and require a clear expected
   advantage.

## 13. Bottom line

The bucket join is not a base-wide strategy. It is a branch-local strategy
whose suitability is determined by a small vector of exact structural
parameters:

```text
(a, NX, NY, K, T, Pc, lifts, suffix_tuple_count).
```

The decisive first number is:

```text
a P NY.
```

Calculate it before allocation. Then calculate the whole-candidate work,
including suffix multiplicity. Only then run a short correlation-aware
pilot.

The recommended selector is:

```text
exact size gate
    -> exact representation-aware memory gate
    -> analytic whole-candidate work gate
    -> bounded histogram pilot
    -> bucket or scan
```

This preserves the spectacular wins at bases 48 and 49, avoids the base-41
memory disaster entirely, and stops treating “the index fits” as evidence
that the index is worth building.
