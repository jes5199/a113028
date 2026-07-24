# Confirming the Higher A113028 Values

## Executive summary

The principal problem above base 52 is no longer finding arithmetically valid
numbers. It is proving maximality.

The current weak values at bases 54, 59, 61, 62, and 64 are valid lower
bounds, but the driver reached them only after treating one or more
window-bounded failures on lexicographically earlier digit sets as if those
sets were globally infeasible. Base 63 has no lower bound because the same
refute-and-descend process exhausted its time budget before reaching a
completion.

The evidence from bases 50, 56, 58, and 59 says to expect further
supersedes. The right immediate goal is therefore not to independently
reconfirm the existing weak strings. It is:

1. identify the unique largest-cardinality digit set that can possibly work
   at each base;
2. search that set at its predicted true divergence depth;
3. replace the fixed-window proof boundary with an exact outer
   lexicographic search whose terminal oracle is the existing bucket engine;
4. produce a branch-by-branch proof manifest for maximality.

The most attractive immediate target is base 63. Its unique maximal
candidate set has 55 digits and predicted divergence depth 23, only one
position beyond the current 22-digit `CERTPOS=21` window. The existing b63
scan spent about 40 seconds per attempted subset. A targeted
`CERTPOS=22`-shaped search of the correct top set may therefore find the
first b63 value without a large new algorithm.

For a longer-term meet-in-the-middle path, an exact additive ternary
digit-set encoding removes the mask-disjointness/superset-query obstruction:

\[
\tau(S)=\sum_{d\in S}3^d.
\]

For internally distinct blocks, equality of their summed ternary encodings
to the universe encoding proves both disjointness and exact union. Through
base 64 the encoding fits comfortably in 128 bits.

---

## 1. Two different kinds of unfinished result

The higher-base table currently contains two conceptually different evidence
classes.

### 1.1 Strong, single-engine results

Bases 53, 55, and 57 were exhaustively completed by one engine family.
Their issue is independent confirmation, not a known gap in the exhaustive
search.

The cleanest second-method question is not “find the value again.” It is the
decision problem:

> Does any arrangement exist that is lexicographically larger than the
> reported value?

That formulation permits an independent exact solver to use the reported
value as a cutoff and return either a counterexample or an infeasibility
certificate.

### 1.2 Weak lower bounds and no-value

Bases 54, 59, 61, 62, and 64 have directly verified completions but no proof
that a larger completion does not exist. Base 63 has no completion yet.

These are not primarily “second engine” problems. They are incomplete
first-engine proofs. The fixed window searched one selected high prefix of a
digit set; failure inside that window does not prove that another high prefix,
or a deeper divergence, has no completion.

This is the precise failure mode already exposed by the b50, b56, and b58
supersedes.

Canonical status:

- [FRONTIER-STATUS.md](https://github.com/jes5199/a113028/blob/884eb1c1c6382f7fd0fc2b37a8f86ea1f007b58a/FRONTIER-STATUS.md)
- [README.md](https://github.com/jes5199/a113028/blob/884eb1c1c6382f7fd0fc2b37a8f86ea1f007b58a/README.md)

---

## 2. The digit sets that should be searched first

For each open base, elementary arrangement-independent conditions identify a
unique maximum-cardinality digit set. If that set has any valid arrangement,
every currently reported shorter value is automatically submaximal.

| Base | forced drops from the first set that matters | set size | current lower-bound size | predicted \(m^*\) |
|---:|---|---:|---:|---:|
| 54 | `{26, 27}` | 51 | 50 | 21 |
| 59 | `{29}` | 57 | 56 | 23 |
| 61 | `{30}` | 59 | 58 | 25 |
| 62 | `{30, 31}` | 59 | 58 | 23 |
| 63 | `{9, 18, 27, 28, 36, 45, 54}` | 55 | none | 23 |
| 64 | none | 63 | 60 | 26 |

Here \(m^*\) is the usual planning statistic

\[
m^*=\min\{m:m!\ge L_{\mathrm{eff}}\}.
\]

It is a search-depth forecast, not a theorem or a certification boundary.

### 2.1 Base 54

\(54=2\cdot3^3\). To avoid \(54\mid\operatorname{lcm}(D)\) without deleting
all even digits, digit 27 must be removed. Since

\[
1+\cdots+53=27\cdot53,
\]

the order-one condition modulo 53 then forces one additional removed digit
congruent to \(26\), namely digit 26.

Thus the first set is

\[
\{1,\ldots,53\}\setminus\{26,27\}.
\]

Its predicted depth is already within the present effective window. That
makes b54 especially suggestive: the failure is likely prefix selection and
suffix-family handling rather than a genuinely much deeper transition.

### 2.2 Base 59

Because 59 is prime and does not occur as a digit, the ten-rule is automatic.
The order-one modulus is \(58\), and

\[
1+\cdots+58\equiv29\pmod{58}.
\]

Deleting digit 29 gives the unique 57-digit set satisfying the invariant.
The existing 56-digit value cannot be maximal until this one set is resolved.

### 2.3 Base 61

The full digit sum satisfies

\[
1+\cdots+60\equiv30\pmod{60}.
\]

Deleting digit 30 gives the unique 59-digit candidate set.

Its predicted depth 25 is materially beyond the current 22-digit window, so
b61 is a later deep-ladder target rather than the first experiment.

### 2.4 Base 62

\(62=2\cdot31\). Keeping digit 31 makes the full digit lcm divisible by 62,
so digit 31 must be removed unless every even digit is removed. The latter is
far more expensive.

The full digit sum is divisible by 61. Once 31 is removed, the order-one
condition modulo 61 forces removal of digit 30 as well. Hence the unique
59-digit first set drops `{30,31}`.

### 2.5 Base 63

\(63=3^2\cdot7\). The cheapest way to break \(63\mid L\) is to remove every
multiple of 9:

```text
9, 18, 27, 36, 45, 54
```

Their sum is

\[
189\equiv3\pmod{62}.
\]

Meanwhile,

\[
1+\cdots+62=1953\equiv31\pmod{62}.
\]

The remaining digit sum is therefore 28 modulo 62, forcing the additional
removal of digit 28. This gives the unique 55-digit set in the table.

The competing ten-rule route—removing all multiples of 7—requires more
drops and therefore cannot win if the 55-digit set has a completion.

### 2.6 Base 64

No digit below 64 contains \(2^6\), so the full digit lcm is automatically
not divisible by 64. Also,

\[
1+\cdots+63=32\cdot63.
\]

Thus the full set `{1,...,63}` passes the elementary invariants. Any valid
63-digit arrangement would dominate the current 60-digit lower bound.

### 2.7 Why existence is the natural expectation

The rough equidistribution count \(k!/L\) for these top sets ranges from
approximately \(10^{43}\) to \(10^{61}\). This is not a proof—cyclotomic
structure can invalidate naive independence—but it makes global
infeasibility of every top set an extraordinary hypothesis.

The much more likely explanation is the one already observed repeatedly:
valid arrangements exist, but the lexicographically largest one diverges
below the fixed search window.

---

## 3. Replace the fixed proof window with an exact outer search

The current bucket machinery is exact once the entire unresolved remainder
fits inside its window. The incompleteness occurs above that window:
`buildFeasiblePrefix()` chooses one feasibility-adjusted prefix, and failure
below that prefix is not a refutation of the whole digit set.

The proposed change is to retain the bucket engine as a terminal oracle and
wrap it in a complete lexicographic branch-and-bound.

### 3.1 State

Each outer-search node contains:

- the fixed digit set \(D\);
- an explicitly fixed high prefix;
- the remaining digit mask;
- the prefix residue/contribution;
- the incumbent full value;
- a comparison state: prefix already greater than, equal to, or less than
  the incumbent prefix.

### 3.2 Recurrence

```text
prove(prefix, remaining, incumbent):
    upper = prefix + sort_descending(remaining)

    if upper <= incumbent:
        return PROVED_BELOW_INCUMBENT

    if remaining fits the exact bucket window:
        maximum = exact_bucket_max(prefix, remaining)
        if maximum exists and maximum > incumbent:
            update incumbent
        return EXACT_TERMINAL_RESULT

    for digit in remaining, descending:
        if the child's descending upper bound can beat incumbent:
            prove(prefix + digit, remaining - digit, incumbent)

    return aggregate proof over every eligible child
```

For a fixed terminal prefix, the existing wrong-turn engine already tries
the candidate digit in descending order and computes the maximum survivor.
The required architectural change is to call it with the prefix fixed by the
outer recursion, rather than asking `buildFeasiblePrefix()` to invent one.

### 3.3 Why this is a certification

Every number lexicographically above the incumbent belongs to exactly one
outer child:

1. branches whose descending upper bound is at most the incumbent are
   arithmetically irrelevant;
2. every other branch is either recursively partitioned or passed whole to
   an exact terminal search;
3. terminal zero-survivor results are exhaustive for their entire remaining
   digit pool;
4. the union of the terminal branches is the complete interval above the
   final incumbent.

No heuristic assertion about the true divergence depth occurs in the proof.
Window width affects performance only.

### 3.4 What `buildFeasiblePrefix()` may still do

It remains useful as a discovery heuristic:

- generate an initial completion quickly;
- improve the incumbent;
- choose promising branches first.

It must not justify any `NO` conclusion. In proof mode, a prefix that it
declines is an unsearched branch, not an infeasible branch.

### 3.5 Why this can beat monolithic widening

The worst-case work is still exponential. The practical differences are
important:

- already-proved 22-position terminals are reusable;
- widening does not redo the common descending path;
- every outer child is independently resumable;
- branches can use different planner configurations;
- work can be distributed across cores or machines;
- a newly found larger incumbent immediately prunes other branches;
- failures and resource declines remain attached to explicit branch IDs
  instead of being conflated with refutations.

For the current bases, the predicted additional outer depth is only about:

- b54: zero or one level;
- b59, b62, b63: about one level beyond the current 22-digit window;
- b61: about three;
- b64: about four.

These are heuristic expectations, but they size the first experiment.

---

## 4. A branch-proof queue

The implementation should materialize the outer frontier as explicit jobs.

Suggested record:

```json
{
  "base": 63,
  "digit_set_mask": "...",
  "prefix_digits": ["..."],
  "remaining_mask": "...",
  "modulus": "...",
  "incumbent": "...",
  "terminal_width": 22,
  "status": "pending | running | refuted | survivor | declined",
  "engine_plan": {
    "family": "peeled",
    "NX": 0,
    "NY": 5,
    "K": 3
  }
}
```

Each completed branch should record:

- exact digit set and prefix;
- executable/commit identity;
- modulus split and planner choice;
- roots, lookups, scans, and survivor counts;
- directly verified survivor values, if any;
- whether termination was `REFUTED`, `FOUND`, or `RESOURCE_DECLINED`.

`RESOURCE_DECLINED` must remain an unfinished branch. It can never be folded
into a parent refutation.

An aggregate checker then verifies:

1. child prefixes partition every lex-relevant next-digit choice;
2. bound-pruned children truly cannot exceed the incumbent;
3. every remaining child has an exact terminal result;
4. every reported survivor passes an independent digit/divisibility check.

The manifest is also a convenient work-stealing queue. In b61 and b62,
existing per-subset terminal scans had highly repetitive 60–70 second shapes,
so branch-level parallelism should be effective.

---

## 5. Immediate experimental order

### 5.1 Base 63 first

Run only the forced 55-digit set:

```text
{1,...,62} \ {9,18,27,28,36,45,54}
```

Do not descend to smaller sets merely because a selected 22-position prefix
has no completion.

First try a 23-position terminal search—approximately the geometry of
`CERTPOS=22`. If it finds a survivor, b63 immediately gains a 55-digit lower
bound. If it refutes one terminal prefix, place the other outer prefixes into
the proof queue rather than descending in subset size.

### 5.2 Base 54 next

Target only:

```text
{1,...,53} \ {26,27}
```

Because the predicted depth is already about 21, first test whether exhaustive
outer-prefix handling at the existing width finds a completion.

Base 54 is also the best case for implementing fused suffix records. Its
five-digit nilpotent suffix produces thousands of admissible suffix tuples,
so rebuilding a separate index per tuple is plausibly a major avoidable
constant.

### 5.3 Bases 59 and 62

Target respectively:

```text
B59: {1,...,58} \ {29}
B62: {1,...,61} \ {30,31}
```

Both have predicted depth 23. They are natural second tests of the same
23-position terminal configuration used for b63.

### 5.4 Bases 61 and 64

These are the deeper cases:

```text
B61: {1,...,60} \ {30}, predicted m* = 25
B64: {1,...,63},        predicted m* = 26
```

They should follow implementation of the outer proof queue and practical
parallel execution. Base 64 also requires allowing a terminal position beyond
the current `CERTPOS <= 24` guard.

---

## 6. Exact additive masks for meet-in-the-middle

The previous MITM obstruction had two pieces:

1. each half is an injection, so list sizes are larger than subset-sum list
   sizes;
2. compatibility is mask disjointness with a non-full union, apparently
   requiring a subset/superset query rather than a hash equality.

The second obstruction has an exact algebraic solution.

### 6.1 Ternary encoding

Define:

\[
\tau(S)=\sum_{d\in S}3^d.
\]

Suppose \(X,Y,Z\) each contain no repeated digit. Then:

\[
\tau(X)+\tau(Y)+\tau(Z)=\tau(A)
\]

if and only if \(X,Y,Z\) are pairwise disjoint and their union is \(A\).

Proof: in base 3, the left side has a coefficient in `{0,1,2}` at each digit
position. The target has coefficient 1 for digits in \(A\) and 0 elsewhere.
A duplicated digit produces coefficient 2 and cannot carry into the next
position because the coefficient never reaches 3. Equality therefore forces
each required digit to occur exactly once and every other digit zero times.

### 6.2 Size

At base 64 the largest exponent is 63:

\[
\sum_{d=1}^{63}3^d < 3^{64}<2^{102}.
\]

Thus one `u128` stores the exact additive mask through the current frontier.
Beyond approximately base 80, use a two-limb integer or a pair of
independently checked representations.

### 6.3 How to use it

A partial record can carry:

```text
(
    modular residue or carry-boundary state,
    ternary digit encoding
)
```

The matching half asks for the exact complementary pair. No subset
convolution and no probabilistic mask fingerprint is required.

This is particularly natural for:

- a bidirectional carry-state join;
- a quotient-multiplication split;
- a three-way `top + bottom + decoded leaf` join;
- measuring the true collision ceiling of moment-based signatures.

### 6.4 Limitation

The ternary encoding removes the mask-query problem, not the injection-count
problem. A proposed MITM still needs a half-local treatment of the decoded
leaf or an explicit carry-boundary state.

In particular, measuring how often moments reject a fully formed
`(leaf,candidate)` pair is not by itself a proof that halves can be joined
cheaply. The leaf digits depend nonlinearly on the combined residue through
subtraction and carries. Before accepting a moment-based GO result, the
prototype should exhibit the actual half-local key and compute its real
collision sum.

The ternary encoding should be added to the MITM experiment as an exact
control:

- moments measure increasingly strong lossy approximations;
- \(\tau\) measures the exact mask condition;
- the gap between their collision counts quantifies how much remains to be
  gained from a better additive signature.

Related task:

- [TASK-MITM-FP.md](https://github.com/jes5199/a113028/blob/884eb1c1c6382f7fd0fc2b37a8f86ea1f007b58a/TASK-MITM-FP.md)

---

## 7. What not to prioritize

### 7.1 More bounded-order marginal filters

The grouped cyclotomic DPs were sound but produced zero useful rejections on
the real frontier probes. The Irreducibility Law explains why this is not
mere bad luck: bounded-order congruence layers account for too little of
\(\log L_{\mathrm{eff}}\) to force survivor-density decay near the critical
band.

Further work should use those layers only if it transforms the problem or
participates non-marginally in an exact join.

- [IRREDUCIBILITY-LAW.md](https://github.com/jes5199/a113028/blob/884eb1c1c6382f7fd0fc2b37a8f86ea1f007b58a/IRREDUCIBILITY-LAW.md)

### 7.2 Blind subset descent

Once a window-bounded search fails on a larger digit set, moving to a smaller
set may find a valid number but cannot support a maximality claim. It also
wastes the most informative next computation: widening or changing the
prefix on the larger set.

Discovery and certification should be separate modes:

- discovery may jump around to improve an incumbent;
- certification must cover every lexicographically relevant branch of every
  larger digit set.

### 7.3 Treating heuristic \(m^*\) as a proof

\(m^*\) is valuable for scheduling. It is not a sound upper bound on actual
divergence and must not suppress branches. The exact outer search removes the
need to pretend otherwise.

---

## 8. Recommended implementation sequence

1. Add a CLI mode that accepts an explicit digit set and never descends to a
   smaller set after a window failure.
2. Add an exact terminal call that accepts an explicit prefix and remaining
   pool, bypassing `buildFeasiblePrefix()`.
3. Implement the incumbent-guided outer DFS with a configurable terminal
   width.
4. Persist terminal jobs and results as a resumable proof queue.
5. Run the forced b63 set at a 23-position terminal width.
6. Run the forced b54 set with exact outer-prefix handling.
7. Add branch-level parallel execution.
8. Run the forced b59 and b62 sets at approximately 23 positions.
9. Implement fused suffix records if b54 profiles confirm that suffix-index
   rebuilding dominates.
10. Add the ternary encoding to the MITM measurement as the exact mask
    baseline.
11. Tackle b61 and b64 after the proof queue and practical parallelism are
    stable.

---

## Bottom line

The weak higher values are probably not waiting for a second verifier. They
are waiting for deeper completions on larger, already identifiable digit
sets.

The most practical route is to stop allowing a fixed-window miss to trigger
subset descent. Use the current exact bucket join as a terminal oracle inside
a complete, incumbent-guided outer search. That produces a genuine maximality
proof, makes widening incremental and parallel, and focuses every expensive
run on an explicit unresolved branch.

Base 63 is the best immediate experiment: its top digit set is uniquely
forced, its predicted depth is only one position beyond the current window,
and its existing per-subset scans are relatively cheap. It may be possible to
turn “no value known” into a 55-digit lower bound before undertaking any major
new engine.

---

## Review note (Fable, 2026-07-24, pre-implementation vet)

§3.3 verified sound: every D-arrangement above the incumbent lies in exactly
one outer child (unique next-digit), bound-pruning is arithmetically safe
(descending suffix maximizes), terminals are exhaustive for their pool. All
§2 forced-set derivations re-derived by hand and confirmed (b54 {26,27},
b59 {29}, b61 {30}, b62 {30,31}, b63 {9,18,27,28,36,45,54} — the 9-multiples
route at 7 drops beats the 7-multiples route at ≥8, set unique; b64 full).
§6's ternary proof and u128 bound check out. Implementation obligations:

**I1 (comparisons are NOT integers):** full values run to ~64^63 ≈ 2^378 —
far beyond u128. ALL outer-search comparisons (incumbent, upper bounds)
must be fixed-length digit-sequence lex comparisons, never integer
arithmetic. Only per-branch RESIDUES (mod L < 2^128) are u128.

**I2 (one terminal geometry):** fire the terminal exactly at
|remaining| == W_terminal+1 (candidate slot + W_terminal window), a single
validated configuration — not "fits ≤", which would need per-size position
arithmetic re-validation.

**I3 (the historic bug class, highest risk):** the terminal's
prefix-contribution C must be computed from the EXPLICIT (possibly
non-descending) outer prefix. Every prior false-refutation incident in this
engine was position/contribution arithmetic. Hard gate before b63: the
outer DFS in certification mode must reproduce AND prove the known answers
on b56/b58/b60 end-to-end (known-value oracle for the whole pipeline,
including non-descending prefix branches explored en route).

**I4 (status discipline):** RESOURCE_DECLINED / timeout terminals stay
unfinished in the manifest and block the certification claim; only
REFUTED/FOUND/PRUNED branches may aggregate. Incumbent updates take the
terminal's max survivor.

Minor: b64's terminal will eventually need the CERTPOS>24 validation range
opened (doc notes this); the queue/manifest (§4) can start as a simple
append-only JSONL. Build order §8.1→8.3 then §5.1 b63 endorsed.
