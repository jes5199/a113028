A113028: Faster Validation of the Provisional Maximums
=======================================================

Repository:
https://github.com/jes5199/a113028

Executive summary
-----------------

The highest-value change is to make the proof cutoff-aware: once an incumbent
has been found, validate only prefixes that could still produce a larger
answer. Do not finish every originally scheduled shard merely because it was
scheduled.

This may already be enough to certify the provisional maximum for base 63
using work that has been completed. At terminal width W=22, only 82 terminal
prefixes can be lexicographically at least as large as the current incumbent.
The incumbent is terminal prefix number 81 in descending traversal order.
The README reports far more completed branches than that, so the existing
manifests may already contain every proof record needed. The manifest records
must be checked by exact prefix; the aggregate counts alone are not a
certificate.

The recommended order of work is:

1. Build a cutoff-aware manifest verifier and try it on base 63.
2. Turn the discovery seed into a reusable proof record, avoiding duplicated
   exact-terminal work.
3. Use the existing terminal planner inside the outer proof rather than
   hardcoding the peeled NX=2, K=3 method.
4. Replace static modulo sharding with a prefix work queue, or at least
   periodically restart shards using the best global incumbent.
5. Only then investigate a reusable terminal bucket index.


1. Why base 63 may already be certifiable
-----------------------------------------

For base 63, the relevant digit set has 55 digits. With terminal width W=22,
the terminal routine is entered after:

    55 - (22 + 1) = 32

prefix digits have been fixed.

The current incumbent begins with 30 digits in descending order. Its next two
digits are 24 and 16. The terminal prefixes lexicographically greater than or
equal to that incumbent are therefore:

    3 * 24 + 9 + 1 = 82

Explanation:

* At the first departure from descending order, the available digits larger
  than 24 are 29, 26, and 25. Each choice leaves 24 possibilities for the
  following prefix digit: 3 * 24 = 72.
* After choosing 24, nine available digits are larger than 16.
* One final prefix is the incumbent's own prefix.

Thus, in the deterministic descending traversal, the incumbent is terminal
counter 81, counting from zero.

The exact base-63 incumbent digits are:

    62, 61, 60, 59, 58, 57, 56, 55, 53, 52, 51, 50, 49, 48,
    47, 46, 44, 43, 42, 41, 40, 39, 38, 37, 35, 34, 33, 32,
    31, 30, 24, 16, 14, 17, 10, 8, 20, 5, 23, 12, 7, 4, 25,
    15, 13, 26, 19, 6, 29, 3, 1, 22, 11, 2, 21

The README currently reports 1,164 refuted branches and 31 survivor branches.
That is much more work than the 82 lex-relevant terminal prefixes. If the
union of the shard manifests contains definitive records for every exact
prefix from counter 0 through counter 81, the current incumbent is maximal
and the later work is unnecessary.

Important qualification: a count such as "1,164 refuted" does not prove
coverage. The verifier must union records by their exact terminal prefix and
confirm that every lex-relevant prefix is present, definitive, and compatible
with the same search parameters.


2. A cutoff-aware verifier
--------------------------

The verifier should traverse the outer search tree in descending order while
using the best known survivor as a cutoff. At every node, construct the
largest possible completion by appending the remaining digits in descending
order. If that upper bound is no greater than the incumbent, the entire
subtree is proved irrelevant without requiring a terminal manifest record.

Conceptual pseudocode:

    verify(prefix, remaining, incumbent):
        upper = prefix + descending(remaining)

        if upper <= incumbent:
            return PROVED_BY_BOUND

        if size(remaining) == W + 1:
            record = definitive_record_for_exact_prefix(prefix)
            require record exists
            require record parameters match this proof
            require record is FOUND or REFUTED
            require record.maximum_survivor <= incumbent
            return PROVED_BY_TERMINAL_RECORD

        for digit in remaining, in descending order:
            verify(prefix + digit, remaining - digit, incumbent)

        return PROVED

The result should be one of:

* CERTIFIED, with the incumbent and the manifest identities used; or
* INCOMPLETE, with the exact missing or incompatible prefixes.

This turns previously computed shard output into a durable certificate rather
than treating each run as an all-or-nothing job.


3. Immediate operational procedure for base 63
-----------------------------------------------

1. Make immutable checkpoints of all three base-63 shard manifests and record
   their command-line/search parameters.
2. Parse all definitive FOUND and REFUTED records.
3. Deduplicate records by exact prefix, rejecting conflicting records.
4. Select the lexicographically largest recorded survivor as the incumbent.
5. Traverse the outer tree with the cutoff-aware verifier.
6. Require terminal proof records only for subtrees whose descending upper
   bound exceeds the incumbent.
7. If the verifier reports missing prefixes, run precisely those prefixes,
   then repeat the verification.

The existing non-sharded "certbb ... resume" path appears close to this
behavior. One possible low-effort experiment is to concatenate compatible
shard proof records into a baseline manifest and resume from it. The incumbent
loaded from the manifest should prune everything below terminal counter 81.
The discovery-seed call should be skipped on such a resume, or at least must
not overwrite a larger resumed incumbent.


4. Promote the discovery seed to a proof record
-----------------------------------------------

The discovery seed is itself an exhaustive exact-terminal search, but the
outer DFS later reaches the same prefix and performs the exact-terminal
search again. The seed is currently useful for finding an incumbent but not
for proving that terminal branch.

When the discovery seed finishes, record it exactly as a normal terminal
result:

    EXACT_TERMINAL_FOUND

or:

    EXACT_TERMINAL_REFUTED

Install the record in the same proven-prefix map consulted by the DFS. Then
the later visit to that prefix can reuse it.

Also compare a discovered seed against the incumbent already loaded by
"resume". Never unconditionally replace a resumed incumbent with the seed;
retain the larger of the two.

The base-58 log strongly suggests that the terminal work is currently being
done twice: roughly the first half of the log occurs before the seed, and a
similar amount occurs afterward. Eliminating that duplication could cut a
large fraction of some outer-proof runs.


5. Use the terminal planner inside the outer proof
--------------------------------------------------

The outer exact-terminal path currently hardcodes the peeled method with
NX=2 and K=3. Elsewhere, the code already has planning logic that chooses
between peeled and full-modulus formulations and selects legal split
parameters. The outer proof should call that planner for each terminal
instance instead of fixing one strategy.

This is especially important for bases 54 and 62, where the target multiplier
is 5. Representative constants at W=22 are:

    Base   T   Pc   Pfull
    ----  --  ---  -----
      54   5   12     14
      59   1   14     14
      61   1   14     14
      62   5   14     15
      63   2   14     15
      64   1   14     15

The fixed peeled configuration can be badly mismatched to a terminal. Base 58
is a warning sign: its recorded outer proof took about 9,559 seconds, while a
planner-selected W=21 full-modulus run was around 300 seconds. Those timings
are not an exact apples-to-apples benchmark, but the difference is large
enough to justify integrating the planner before doing more long proofs.

Relevant source:
https://github.com/jes5199/a113028/blob/main/carrytrie.cpp#L5517-L5554


6. Replace frozen modulo sharding
---------------------------------

The current modulo-shard scheme freezes the incumbent so that all workers
maintain synchronized terminal counters. That makes assignment deterministic,
but it also means that discovering an excellent incumbent does not stop other
workers from processing lower prefixes. In base 63, workers continued far
beyond counter 81 even though those branches cannot beat the incumbent.

The better design is a coordinator-managed queue of explicit prefix jobs:

* Generate terminal prefixes as stable, self-describing work items.
* Workers claim prefixes rather than counter congruence classes.
* Publish every better survivor to the coordinator immediately.
* Prune queued jobs whose descending upper bound is no greater than the new
  incumbent.
* Persist FOUND or REFUTED records keyed by exact prefix.
* Allow retries and deduplication without depending on traversal counters.

A smaller transitional change is epochal sharding:

1. Run the current shards for a bounded period.
2. Stop at checkpoints.
3. Union the manifests and select the global incumbent.
4. Regenerate only the unresolved, lex-relevant frontier.
5. Start a new shard epoch using that frontier.

Epochal sharding preserves most of the existing implementation while
capturing much of the benefit of live incumbent propagation.


7. Consider a reusable terminal bucket index later
--------------------------------------------------

For a fixed base, modulus, and split, the bucket contribution

    u_y = B^P * y mod M

depends on the y-digits, not on the candidate prefix, suffix, or target.
A reusable superset index over y-combinations could therefore amortize index
construction across many terminal calls. Each query would still need to
reject entries whose digits conflict with the current forbidden mask.

The base-63 log reported about 2,369 index builds for 41 terminal calls,
roughly 58 builds per terminal call. That makes reuse plausible, but it should
come after the cutoff, seed-reuse, and planner changes. A superset index may
save construction cost while leaving lookup and mask-filtering cost intact,
so profile the revised program before committing to the added complexity.


8. Treat the other unresolved bases as completion searches first
----------------------------------------------------------------

For bases 54, 59, 61, 62, and 64, the maximal-cardinality digit set is unique
and larger than the digit set of the current provisional answer. Therefore
any completion using the forced top set immediately supersedes the current
provisional value.

    Base   Forced omissions              Forced size   Current size
    ----   ----------------------------  -----------   ------------
      54   26, 27                              51             50
      59   29                                  57             56
      61   30                                  59             58
      62   30, 31                              59             58
      64   none                                63             60

Base 63 is different: its current incumbent already uses the forced
maximal-cardinality set, omitting:

    9, 18, 27, 28, 36, 45, 54

For the other five bases, the efficient sequence is:

1. Search for any valid completion of the unique forced top set.
2. If one is found, install it as the incumbent; it beats the provisional
   value by cardinality, regardless of its within-set lexicographic order.
3. Run a cutoff-aware maximization proof within that forced set.

This avoids spending time proving maximality of a provisional value that is
known to use too few digits if the forced top set is feasible.

Strategy document:
https://github.com/jes5199/a113028/blob/main/HIGHER-BASE-CERTIFICATION-STRATEGY.md


9. Certificate integrity checks
-------------------------------

Before accepting records from multiple runs, verify:

* base, multiplier/target, digit set, terminal width, modulus, and split
  parameters match;
* each record identifies an exact prefix rather than only a traversal counter;
* FOUND records contain the maximum survivor for that exact terminal branch,
  not merely the first survivor;
* REFUTED records represent exhaustive completion of the exact branch;
* interrupted or partial records are never treated as definitive;
* duplicate records agree, or are resolved by rerunning that prefix;
* the final verifier records the incumbent, manifest hashes, and all
  bound-pruned and terminal-proved frontier nodes.

A useful independent check is to replay the final certificate with a small
verifier that contains no search heuristics. It only needs lexicographic
upper bounds, prefix-key validation, and verification that each terminal
record's maximum does not exceed the incumbent.


Recommended implementation sequence
-----------------------------------

Phase 1: recover already completed value

* Write the cutoff-aware manifest verifier.
* Run it against the base-63 shard checkpoints.
* Compute and execute only the exact missing prefixes, if any.

Phase 2: remove obvious repeated work

* Save discovery-seed results as normal terminal proof records.
* Preserve the maximum of the resumed and discovered incumbents.
* Skip a seed whose exact prefix is already definitively recorded.

Phase 3: make each remaining terminal cheaper

* Route outer-proof terminal calls through the existing planner.
* Benchmark peeled versus full-modulus choices on recorded terminal prefixes.

Phase 4: make parallel work incumbent-aware

* Implement epochal resharing as a low-risk first step.
* Move to an explicit prefix queue if long-running multiworker proofs remain
  important.

Phase 5: optimize index reuse only if profiling still points there

* Prototype a reusable superset bucket index.
* Measure construction savings against forbidden-mask filtering overhead.


Bottom line
-----------

The immediate opportunity is not a faster exhaustive traversal; it is to stop
requiring exhaustive traversal below an already excellent incumbent. For base
63, only 82 terminal prefixes matter at W=22, and the existing shard work may
already cover them. A cutoff-aware verifier can determine that cheaply and,
if coverage is incomplete, identify exactly what remains.
