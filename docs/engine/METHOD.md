# Method overview: Engine C and the tractability frontier

Past any practical numeral system, "base B" is scaffolding: with n = B−1 the
problem is purely combinatorial — *choose a subset D ⊆ {1..n} and arrange it
as weights B⁰..B^{|D|−1} so that lcm(D) divides the total, maximizing the
value.* The subset turns out to be forced by elementary number theory; all
the difficulty is in the arrangement.

## Method (Engine C)

Three observations reduce the problem to a narrow core:

1. **The subset is forced.** If B | lcm(D) no arrangement works (the last
   digit would have to be ≡ 0 mod B), and for every prime power q | B−1 the
   value ≡ digit-sum (mod q) regardless of arrangement — so the digit *set*
   is pinned by cheap arithmetic before any search. (Example: for B=49,
   49 ≡ 1 mod 16 and mod 3 force the removal of exactly digit 24 from
   {1..48}.)
2. **The divergence-depth law.** Only L_eff = lcm(D)/(order-1 part)
   constrains the arrangement. Permuting the last m positions reaches a
   target residue only when m! ≳ L_eff, so the answer equals the plain
   descending arrangement except in its last **m\* = min{m : m! ≥ L_eff}**
   positions. This held within ±2 for every known value — and its violation
   is what exposed the a(46) error.
3. **The leaf horizon.** Below P = ⌈log_B lcm(D)⌉ positions, the suffix
   *value* is fully determined mod lcm(D): at most a handful of candidates
   exist, each checkable in O(P) by decomposing and comparing digit sets.
   No search is needed below P.

**Engine C** is then: walk the descending arrangement from the top (free,
by law #2); inside the critical band around m\*, run a largest-digit-first
DFS where each node is pruned by sound per-prime-power feasibility DPs
(exact partition dynamic programs for multiplicative orders 1–3, plus
order-2 split checks); at depth P, decode the forced suffix directly.
The first hit in descending order is provably the maximum — the algorithm
is deterministic, with no budgets or restarts.

The irreducible cost is refuting "wrong turns" in the band — prefixes
lex-above the answer with no completion, where every cheap test necessarily
passes. That sweep has size **W ≈ m\*!/P!**, which is the hardness score in
[../results/HARDNESS-AND-TIMINGS.md](../results/HARDNESS-AND-TIMINGS.md).

Above the mask range this core acquired the production superstructure:
subset discovery by feasible-subset enumeration, shallow radix-bucket
terminal searches, and the exact outer lexicographic branch-and-bound that
turns window width into a performance knob rather than a soundness boundary
(design docs in this directory; per-base proof summaries in
[../results/MAXIMALITY-ARGUMENTS.md](../results/MAXIMALITY-ARGUMENTS.md)).

## The tractability frontier

W grows like exp(Θ(n·ln ln n / ln n)) — subexponential but far
superpolynomial. ([../theory/FRONTIER.md](../theory/FRONTIER.md) has the
original analysis.) The 2026-07-23/26 campaigns rewrote the practical
picture, and this section states it as it now stands — an earlier revision
of this text (in the README) still described bases 54–64 at their interim
statuses; all of that is resolved:

- **bases ≤ 52: certified** (multi-method) — the certified frontier moved
  from the b-file's 48 to 52 in one day;
- **bases 53–64: solved, every one STRONG or CERTIFIED** (the engine's
  64-bit digit-mask ceiling was 64; the u128 widening lifted it to 89).
  Hardness was not one wall but three separable modes, and each fell to a
  different fix: *discovery churn* (fixed by feasible-subset enumeration —
  b60 went from no-value at 90 min to certified in 14s), *memory* (fixed by
  admission control), and *band depth* — the real one: when the answer's
  divergence from descending order exceeds the affordable search window
  (~9× cost per extra position), refutations become window-bounded and
  maximality is unprovable by brute search. For 54–64 band depth was
  dissolved without brute force: the forced-set cardinality argument
  (every provisional value used fewer digits than the forced set, so any
  full-cardinality completion supersedes it), exact terminal decomposition
  (b61), the outer branch-and-bound with lex-domination cutoffs (b63), and
  one arithmetic obstruction (b64);
- **bases 65–89: swept, no completion found** — and the campaign located
  the engine's true structural limit, the *irreducible wide-window regime*
  (`W = min W`, where terminal decomposition provably cannot apply — b82,
  b86). See [../results/HIGH-BASE-RANGE-65-89.md](../results/HIGH-BASE-RANGE-65-89.md);
- the real levers remain the arithmetic of B−1 (smooth ⇒ strong subset
  filtering; prime ⇒ deep bands), certification theory for the wide-window
  regime, and — as always — the luck of the nilpotent structure. Bases ≥ 90
  need bignum arithmetic (lcm(1..89) is the last to fit in 128 bits) and a
  genuinely different attack; proof-producing SAT is the standing proposal
  ([../reviews/SHOULD-THE-PROJECT-STOP-AT-BASE-64.md](../reviews/SHOULD-THE-PROJECT-STOP-AT-BASE-64.md)).
