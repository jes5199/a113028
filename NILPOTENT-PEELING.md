# Nilpotent peeling: an exact coprime-core reduction

## Setting

Let

\[
N=\sum_{i=0}^{m-1}d_iB^i
\]

use a prescribed set of distinct nonzero base-\(B\) digits, and let \(L\)
be the required divisor.  Split

\[
L=L_{\mathrm{nil}}L_{\mathrm c},\qquad
L_{\mathrm{nil}}=\prod_{p\mid B}p^{v_p(L)},\qquad
\gcd(L_{\mathrm c},B)=1.
\]

Put

\[
T=\max_{p\mid B}\left\lceil\frac{v_p(L)}{v_p(B)}\right\rceil,
\]

so that \(L_{\mathrm{nil}}\mid B^T\).  Write an arrangement uniquely as

\[
N=S+B^TH,qquad
S=\sum_{i=0}^{T-1}d_iB^i,qquad
H=\sum_{i=T}^{m-1}d_iB^{i-T}.
\]

## Theorem (nilpotent peeling)

For every ordered choice of the last \(T\) digits,

\[
L\mid N
\]

if and only if

\[
S\equiv0\pmod{L_{\mathrm{nil}}}
\]

and

\[
H\equiv-S(B^T)^{-1}\pmod{L_{\mathrm c}}.
\]

Consequently the original arrangement problem is the disjoint union, over
the suffix words satisfying the first congruence, of permutation-feasibility
problems on \(m-T\) digits modulo the coprime core \(L_{\mathrm c}\).

### Proof

Modulo \(L_{\mathrm{nil}}\), the term \(B^TH\) vanishes, so divisibility
forces and is forced by \(S\equiv0\).  Modulo \(L_{\mathrm c}\), \(B^T\)
is a unit, and

\[
S+B^TH\equiv0
\quad\Longleftrightarrow\quad
H\equiv-S(B^T)^{-1}.
\]

The two moduli are coprime, so CRT combines the two conditions into
\(N\equiv0\pmod L\).  Each arrangement has exactly one ordered final
\(T\)-tuple, making the union disjoint. \(\square\)

## Lexicographic corollary

For each admissible suffix word \(u\), find the lexicographically largest
high word \(H_u\) on the complementary digit set satisfying its coprime-core
target.  The largest of the numbers

\[
B^T H_u+S_u
\]

is the global maximum.  Thus suffix cases may be solved independently and
in parallel.  The reduction is exact: it is not a relaxation or heuristic.

At a prefix-search node, fixed digits in positions at least \(T\) may simply
be included in \(H\)'s residual target.  This gives the same decomposition
inside Engine C's critical band.

## Why this changes Engine C

Engine C currently keeps the nilpotent prime powers in the full modulus
until residue-leaf decoding.  Peeling them first gives three simultaneous
reductions:

1. remove \(T\) digits from the permutation search;
2. replace \(L\) by the smaller coprime modulus \(L_{\mathrm c}\);
3. lower the leaf horizon from \(\lceil\log_B L\rceil\) to
   \(\lceil\log_B L_{\mathrm c}\rceil\).

Only the admissible suffix words are retained.  In A113028, \(T\) is small,
so enumerating them is cheap.

### Reserved-suffix pruning

Full peeling is not always necessary to get the main benefit.  For the
current remaining digit mask \(A\), precompute

\[
\mathcal F(A)=\{U\subseteq A:|U|=T,\text{ some ordering of }U
\text{ has }S\equiv0\pmod{L_{\mathrm{nil}}}\}.
\]

Every completion must reserve one member of \(\mathcal F(A)\) for the last
\(T\) positions.  During high-prefix DFS, choosing a digit deletes every
reservation containing it; if no reservation remains, the node is exactly
infeasible.  For small \(T\), store the family as bitsets indexed by digit.
Updating it costs a few word operations per node.

This is stronger than the existing `c_cntD1` rule: that rule handles only a
one-position nilpotent component, whereas \(\mathcal F\) handles all primes
dividing \(B\), jointly, at their full required depth.

## Base 40: the published value is certified, and Engine C drops from >1 h to 7.3 s

For the surviving 34-digit subset

\[
D=\{1,\ldots,39\}\setminus\{8,16,24,32,37\},
\]

\[
L=18050444111700,qquad L_{\mathrm{nil}}=100,qquad T=2.
\]

Writing the final two digits as \(d_1d_0\), nilpotent divisibility says

\[
d_0+40d_1\equiv0\pmod{100}.
\]

Reducing modulo 20 forces \(d_0=20\).  Substitution then gives
\(d_1\equiv2\pmod5\), so the only suffixes available in the critical
remaining set are

```
2,20   7,20   12,20   17,20
```

(written most-significant digit first).  Thus digit 20 is forced to remain
in the suffix.  The lexicographically larger critical-band choice that puts
20 in a high position is impossible without any search.  This explains the
18-digit divergence that had been treated as evidence that the published
value might be suboptimal.

Exact peeled searches for the four suffixes returned lexicographically
maximal high tails beginning respectively

```
18,15,9,...
18,15,14,...   <- published value
18,14,15,...
18,15,3,...
```

so the `7,20` suffix gives the global maximum for the isolated subset.
Direct divisibility verification succeeds for the published decimal value

```
2949491266532658135493053371770319593915307331801883500.
```

A prototype reserved-suffix family added to Engine C completed the entire
base-40 computation in **7.296 seconds**, visiting 2,504,310 nodes.  The
repository's previous bounded run exceeded one hour inside the same subset.

## Base 48: the second suspected published value is also certified

For

\[
D=\{1,\ldots,47\}\setminus\{16,32,46\},
\]

\[
L=110680160865928453800,quad
L_{\mathrm{nil}}=216=2^3 3^3,quad
L_{\mathrm c}=512408152157076175,quad T=3.
\]

Modulo both 8 and 3, all terms above the units digit vanish.  Hence the
units digit must be divisible by \(\operatorname{lcm}(8,3)=24\).  Of the
available digits only 24 qualifies, so **digit 24 is forced to be the last
digit**.  After dividing by 24, the remaining nilpotent condition is

\[
1+2d_1+6d_2\equiv0\pmod9.
\]

Therefore the lexicographically maximal descending branch that consumes
24 is immediately impossible.  This forces the observed 23-digit
divergence; again, the divergence-law outlier was a nilpotent suffix effect,
not evidence of a bad b-file value.

At the final nontrivial comparison, candidate 21 has 34 admissible ordered
nilpotent suffix patterns.  Exact peeled enumeration refuted all of them:

```
candidate 21: NO, 302,964,480 leaves, 102.7 s
```

Candidate 20 has 35 patterns.  Bounding each pattern by its descending high
word allows two dominated patterns to be skipped; exact enumeration of the
rest yields

```
candidate 20: YES, 266,679,257 leaves, 93.6 s
high tail: 21,1,19,9,2,11,3,17,18,8,12,6,7,5,13,14,10
low tail:  15,4,24
```

This reconstructs and proves maximal the published value

```
94237804886307950779486130179671488973571078333724597158459950718126090200.
```

The complete candidate-21/candidate-20 certificate takes about 3.3 minutes
serially in the specialized verifier, compared with the repository's prior
one-hour timeout.

## Base 49 certificate

For the forced digit set

\[
D=\{1,\ldots,48\}\setminus\{24\},
\]

\[
L=442720643463713815200,qquad
L_{\mathrm{nil}}=7,qquad
L_{\mathrm c}=L/7,qquad T=1.
\]

The original leaf horizon is 13.  After peeling it is 12, with

\[
B^{12}/L_{\mathrm c}\approx3.03
\]

candidate multiples per leaf instead of

\[
B^{13}/L\approx21.2.
\]

The known answer has the maximally descending prefix

```
48,47,...,25,23,22,20
```

and the remaining suffix

```
9,12,19,2,3,11,15,10,14,6,16,18,8,1,13,17,21,5,4,7.
```

To certify maximality it is enough to refute candidate 21 at the first
divergence and then find the lexicographically maximal completion under
candidate 20.

Exact peeled enumeration produced:

| branch | forced last digit | result | leaves | wall time |
|---|---:|---|---:|---:|
| candidate 21 | 7 | NO | 253,955,520 | 54.7 s |
| candidate 21 | 14 | NO | 253,955,520 | 62.7 s |
| candidate 20 | 7 | YES, known suffix | 153,051,722 | 34.0 s |
| candidate 20 | 14 | NO | 253,955,520 | 61.9 s |
| candidate 20 | 21 | NO | 253,955,520 | 54.4 s |

The NO runs visit exactly

\[
19!/12!=253955520
\]

high-prefix leaves, so their exhaustion count is independently checkable.
The five runs take about 4.5 minutes serially.  Running suffix cases in
parallel gives about 2.1 minutes over the two sequential lexicographic
branches.

The reconstructed 47-digit number is

```
27480664153312064994836939532520844560984511658005210290838348871641700981823200
```

and direct integer verification gives remainder zero modulo \(L\).

## Production implementation

1. Factor \(L_{\mathrm{nil}}\), compute \(T\), and enumerate ordered
   \(T\)-digit suffixes satisfying \(S\equiv0\pmod{L_{\mathrm{nil}}}\).
2. For each suffix, remove its digits and initialize a target-aware Engine C
   instance modulo \(L_{\mathrm c}\).
3. Use leaf horizon \(P_{\mathrm c}=\lceil\log_B L_{\mathrm c}\rceil\).
4. Compare the returned high words lexicographically; use the low suffix only
   to break an equal-high-word tie.
5. Parallelize the independent suffix instances.  If all instances under a
   guessed maximal band prefix fail, backtrack the prefix exactly as before.

The accompanying specialized verifier `nilpeel_b49.cpp` contains no
probabilistic step and no per-prime relaxation; its NO results come solely
from exhaustive residue-leaf decoding after the proved reduction.

## Reproduction

```sh
g++ -O3 -march=native -std=c++20 nilpeel_b40.cpp -o nilpeel_b40
g++ -O3 -march=native -std=c++20 nilpeel_b48.cpp -o nilpeel_b48
g++ -O3 -march=native -std=c++20 nilpeel_b49.cpp -o nilpeel_b49

# Four base-40 suffix cases (indices 0..3).
./nilpeel_b40 0
./nilpeel_b40 1
./nilpeel_b40 2
./nilpeel_b40 3

# Complete base-48 critical comparison.
./nilpeel_b48 21
./nilpeel_b48 20

# Complete base-49 critical comparison.
./nilpeel_b49 21
./nilpeel_b49 20
```

The modified `a113028/a113028_v11.c` is the production-style
reserved-suffix prototype.  Compiling and running it on base 40 reproduces
the 7-second full solve:

```sh
gcc -O3 -march=native -o a113028_v11_nil a113028/a113028_v11.c
./a113028_v11_nil 40 40 1
```
