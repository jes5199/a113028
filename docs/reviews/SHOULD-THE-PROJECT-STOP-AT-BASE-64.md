> **Provenance:** external review by **Sol Max**, received 2026-07-26 via jes.
> Reproduced verbatim below this note. Its central argument — every valid
> full-alphabet base-64 answer must end in digit 32 — was verified
> independently before this was committed, and it constitutes the independent
> re-derivation that [../results/A64-MAXIMALITY.md](../results/A64-MAXIMALITY.md)
> was waiting on. Its "proof-producing SAT" proposal is a v2 project, not part
> of the solved-through-64 release.

A113028: Should the Project Stop at Base 64?
================================================

Repository:
https://github.com/jes5199/a113028

Assessment date: 2026-07-26


Executive recommendation
------------------------

Yes: call "A113028 solved through base 64" done, tag that result, and turn
toward publication.

This does not mean that the infinite sequence is finished. It means that the
finite computational chapter from base 2 through base 64 is genuinely closed:
there are no remaining WEAK lower bounds or known maximality gaps in that
range. Continuing above 64 is no longer routine frontier extension with the
same machinery. It is a new research project requiring either a different
search geometry or a different solver family.

The remaining distinction between the project's STRONG and CERTIFIED labels
is important but should not be mistaken for an unsolved mathematical branch:

* CERTIFIED means that independent engine families or methods concur.
* STRONG means that one engine family supplied an exhaustive maximality
  argument, but no independent engine family has repeated it.

Thus a STRONG row still has a complete first-engine proof. Its remaining risk
is implementation error, not an explicitly unsearched region. Independent
confirmation would be valuable defense in depth, but it need not hold the
2--64 release hostage.


Why the range through 64 is complete
------------------------------------

The earlier standard for calling a provisional value confirmed was stricter
than "the solver found a valid number." Every region capable of producing a
larger number had to be:

1. exactly refuted;
2. exhaustively searched with its branch maximum at or below the incumbent;
   or
3. pruned by a mathematically valid lexicographic upper bound.

Window-bounded failure from buildFeasiblePrefix() was never sufficient.
Aggregate terminal counts were never sufficient. Coverage had to be keyed by
the exact prefixes that could still beat the incumbent.

The current results meet that standard through base 64:

* Base 61 now has a full maximum-cardinality completion. Reframed at W=22,
  exactly 16 terminal prefixes are lexicographically greater than the
  incumbent's prefix, and all 16 already carry definitive exhaustive
  REFUTED records at identical parameters. The result is no longer merely
  "maximal within a W=23 discovery window."

* Base 63 has 82 lex-relevant terminal prefixes. Their exact-prefix coverage
  is 81 REFUTED plus one FOUND, with the FOUND branch's maximum equal to the
  incumbent. There are no missing prefixes, conflicting records, or resource
  declines in the relevant frontier. The engine subsequently traversed the
  same cutoff frontier from the completed manifest without executing any new
  terminals.

* Base 64 uses all 63 nonzero digits. Its lex-greater region is eliminated
  almost entirely by arithmetic, with only the equal-prefix terminal left to
  exhaustive computation.

The current status table is:
https://github.com/jes5199/a113028/blob/main/FRONTIER-STATUS.md


Why base 64 is an unusually natural capstone
--------------------------------------------

The forced digit set for base 64 is the entire alphabet {1,...,63}. This is
possible because 64 = 2^6 while the largest power of two among the available
digits is only 32 = 2^5, so 64 does not divide lcm(1,...,63). The digit-sum
condition also holds:

    1 + ... + 63 = 2016 = 32 * 63.

Let L = lcm(1,...,63). The 2-primary part of L is 32. Since every positive
power of 64 is zero modulo 32, a base-64 number using the full alphabet is
divisible by 32 if and only if its units digit is divisible by 32. Among the
digits 1,...,63, the only such digit is 32. Therefore:

    Every valid full-alphabet completion in base 64 must end in digit 32.

The incumbent agrees with descending order through digits 63,...,33 and then
uses 31,30,... rather than consuming 32. Any lexicographically greater prefix
in the seven relevant deviation regions must place 32 inside the prefix,
leaving no admissible units digit. Those regions are impossible immediately.
The equal-prefix branch was searched exhaustively and contains exactly the
reported maximum.

This replaces roughly 3.29 * 10^11 potential W=22 terminal prefixes with a
one-line divisibility argument. It is both a correct stopping point and a
beautiful narrative ending for the first phase of the project.

Full argument:
https://github.com/jes5199/a113028/blob/main/A64-MAXIMALITY.md

The reported decimal a(64) has also been checked independently at the witness
level: it decodes to 63 distinct base-64 digits, its digit set is exactly
{1,...,63}, and it is divisible by lcm(1,...,63).


What the 65--89 campaign says
-----------------------------

The work above 64 is valuable precisely because it shows that 64 is not an
arbitrary stopping point chosen just after a lucky hit.

The completed 65--89 first campaign found:

* 22 bases whose descending terminal prefix was exhaustively REFUTED at the
  base's correct minimum width;
* zero completions;
* two INCONCLUSIVE-resource cases, bases 74 and 82;
* one deliberately unattempted irreducible case, base 86.

These are not proofs that no values exist above 64. Each refutation covers
one prefix region at one width. They do show that the near-descending regime
which supplied the values through 64 stops paying immediately.

Base 82 also exposed a structural limit of the current engine. It must run at
its minimum width W=25. At minimum width it cannot be decomposed into smaller
legal child terminals, so checkpointing, sharding, early stopping, and
resumption cannot rescue it. Two runs, including a ten-hour uncontended run,
only increased the resource lower bound. Base 86 is wider and larger still.

Therefore the present engine has not merely "failed to get lucky" above 64.
It has identified a new irreducible wide-window regime for which spending
more of the same compute has poor expected value.

Campaign record:
https://github.com/jes5199/a113028/blob/main/HIGH-BASE-RANGE-65-89.md


Work to do before declaring the release finished
-------------------------------------------------

The remaining work is editorial and archival rather than mathematical.

1. Reconcile stale prose.

   The README's "tractability frontier" still describes bases 54, 59, 61, 62,
   and 64 as weak lower bounds and base 63 as mid-proof. The "Open, not
   concluded" subsection of HIGH-BASE-RANGE-65-89.md still lists completed
   jobs as running. A64-MAXIMALITY.md still says that its argument is awaiting
   independent re-derivation. These passages should either be updated or
   explicitly marked as historical snapshots.

2. Create one canonical release table for bases 2--64.

   It should give, for every base:

   * the decimal value;
   * the base-B digit representation;
   * the forced digit set or forced drops;
   * the evidence class;
   * the exact proof artefact or log location;
   * a witness-verification result.

   This should be the one source from which the README table and OEIS b-file
   are generated, preventing another round of prose/table drift.

3. Freeze the proof-bearing evidence.

   Record hashes for manifests, coverage ledgers, independently written
   verifiers, and the exact production binary or source commit used by each
   STRONG result. A release tag should make "the computation being claimed"
   a fixed object rather than whatever main happens to contain later.

4. Generate and validate the OEIS b-file.

   The public OEIS entry still links to the table through base 48. The new
   submission should include the values through 64, the corrected a(46), and
   a concise explanation of the computational proof standard.

   OEIS entry:
   https://oeis.org/A113028

5. Tag the result.

   A name such as "solved-through-64" or a v1.0 release would state the scope
   exactly. Future experimental work above 64 can then proceed without
   continually destabilizing the completed result.


One genuinely different next algorithm: proof-producing SAT
-------------------------------------------------------------

There is one substantial avenue not apparent in the repository that is
orthogonal to widening the terminal window, release-layer enumeration, or
another position-split meet-in-the-middle search: encode the fixed-digit-set
problem as SAT, with proof logging.

For a fixed digit set D of size m, introduce a Boolean variable

    x[i,d] = "digit d occupies position i".

Impose:

* exactly one digit at every position;
* every digit used exactly once.

These are the row and column constraints of a permutation matrix.

Next factor L = lcm(D) into its pairwise-coprime maximal prime powers q.
Divisibility by L is equivalent to divisibility by every q. For each q,
introduce a small one-hot residue state r[i,q,s] and encode the deterministic
transition

    r[i+1] = r[i] + d * B^i  (mod q)

whenever x[i,d] is true. Start the automaton at residue zero and require its
final residue to be zero, after incorporating any fixed prefix contribution.

Every prime-power modulus q is at most B-1, so these automata are small. The
encoding does not need an enormous state space modulo the full L.

For maximality, add a compact lexicographic comparator asserting that the
digit sequence is greater than the reported incumbent. The solver then has
only two possible useful outcomes:

* SAT: it returns a directly verifiable counterexample;
* UNSAT: a solver such as CaDiCaL or Kissat can emit a DRAT/LRAT proof that a
  separate checker can validate.

This would be a genuinely independent engine family and, unlike a second
implementation of carrytrie's search, it could produce portable UNSAT
certificates.

The instance sizes are credible:

* Base 64's equal-prefix branch has 25 remaining digits and approximately
  386,000 residue-transition implications in the straightforward encoding.
* A base-61 W=22 terminal has 23 remaining digits and approximately 294,000
  transitions. Its 16 lex-greater branches could be checked separately.
* Even a global instance near base 89 is on the order of eight million
  straightforward transitions: large, but ordinary modern-SAT scale rather
  than an absurd encoding size.

Solving time is not guaranteed. The underlying general problem is hard, and
the residue automata may or may not give CDCL enough propagation. But this is
cheap enough to prototype, and it attacks exactly the feature current
relaxations lose: coupling among all prime-power congruences.

Recommended SAT experiment sequence:

1. Gate the encoder on several small known bases.
2. Reproduce an independently certified base such as 56, 58, or 60.
3. Attack the single equal-prefix branch for base 64.
4. Attack base 61's 16 exact W=22 lex-greater branches.
5. If those work, use the solver as a satisfiability instrument on base 65
   or on selected release prefixes above 64.

Success at steps 3--4 would promote important STRONG rows using an actually
independent method. Failure would still be informative: it would establish
that generic clause learning does not cheaply solve the joint-congruence
coupling at the observed threshold.

This is best regarded as a version-2 research project, not unfinished work
required for the solved-through-64 release.


One optional final run with the existing engine
-----------------------------------------------

If there is appetite for exactly one more bounded attempt before stopping,
the most defensible consecutive-frontier experiment is base 65's release-one
layer.

At W=22, the descending prefix is already definitively refuted. The r=1 layer
contains at most 36 additional candidate prefixes before feasibility
filtering. Do not project their cost from another base or from the descending
terminal. Instead:

1. run a sample of three r=1 terminals under the same execution conditions;
2. use that within-configuration sample to estimate the remaining layer;
3. finish the layer only if the measured budget is attractive;
4. stop at a completion and seed the exact outer maximality prover.

If the layer returns no hit, record precisely "no completion within r<=1 at
W=22." It is not a refutation of the forced digit set and should not trigger
an automatic escalation to r=2, whose candidate count is already roughly
quadratic.

This run is optional and should not delay publication. A hit would extend the
consecutive frontier; a miss would add only a bounded negative.


Final judgment
--------------

Publish the castle.

The project has:

* extended the exact sequence from the old frontier through base 64;
* corrected a published value;
* developed an honest hierarchy of computational evidence;
* converted all provisional 50--64 values into exhaustive maximality results;
* found a striking paper-level obstruction at the capstone base;
* and experimentally located the point where the current engine ceases to be
  the right instrument.

That is a completed result. Work above 64 should begin from a fresh question:
"Can a different solver family cross the new regime?" It should not be framed
as though the existing 2--64 computation remains unfinished.
