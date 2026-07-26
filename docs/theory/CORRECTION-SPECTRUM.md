# CORRECTION to PEELING-SUPPORT.md §3, and the nilpotent spectrum results

*(Self-correction, session 2026-07-23. The "base-48 exclusion
theorem" of PEELING-SUPPORT.md §3 is TRUE BUT TRIVIAL, and its
corollary and suggested a(48) check are subsumed by a constraint
the engine has always enforced. Do not cite §3 as novel and do not
run the suggested check. The replacement content — the full
nilpotent spectra of bases 40 and 48, a proved no-nontrivial-
exclusions result, and the suffix branching bounds Sol's sizing
pass needs — is below.)*

## 1. The retraction

PEELING-SUPPORT.md §3 proved: no base-48 number with distinct
nonzero digits is divisible by 864 = 2⁵·3³. The proof is correct
and the statement is worthless: 48 | 864, so divisibility by 864
implies divisibility by B = 48, which forces last digit 0 —
impossible for nonzero digits. The "corollary" (a(48)'s set cannot
contain both 32 and 27) is strictly weaker than the classical
constraint B ∤ lcm(set) (already violated by {16, 3} ⊆ set), which
every candidate set satisfies by construction. The suggested check
of the published a(48) against §3 will pass trivially and should
not be run. Strike §3; keep §§1–2 and 4–6, which are unaffected.

How the error was caught: computing the full nilpotent spectrum
(below) returned the minimal infeasible exponent vectors, which
turned out to be exactly (v_p(B))_p — the trivial ones — exposing
§3 as a disguised instance.

## 2. Nilpotent Spectrum Theorem

For base B with global suffix length s, define the **spectrum** as
the set of capped valuation vectors
(min(v_p(Σ_{j<s} d_j B^j), a_p))_{p|B} over ordered tuples of
distinct digits. Then:

(i) [exact, by the peeling identity] A candidate digit set can meet
its nilpotent target iff its exponent vector
(v_p(lcm(set)))_{p|B} is dominated by some spectrum element.

(ii) [exact] Feasibility is downward closed, so the spectrum is
characterized by the minimal INFEASIBLE vectors (an antichain).

(iii) [exact] One O((B−1)^s) enumeration per base computes it.

## 3. Computed spectra: no nontrivial exclusions exist

Exhaustive enumeration (scripts below):

| base | s | minimal infeasible vectors | interpretation |
|---|---|---|---|
| 40 | 2 | (3, 1) only | = (v₂(40), v₅(40)) — the trivial B∤N constraint |
| 48 | 3 | (4, 1) only | = (v₂(48), v₃(48)) — the trivial B∤N constraint |

**Consequence (proved, negative, useful):** for bases 40 and 48
there are NO set-level nilpotent obstructions beyond the classical
B ∤ lcm(set). Every legal candidate set has a nonempty admissible
suffix family — Sol's sizing pass is pure counting, never
existence-checking, and no root-level nilpotent pruning beyond the
classical constraint is available at these bases.

## 4. Suffix branching bounds (the sizing numbers)

Witness counts at the maximal feasible vectors, over the full digit
range (per-set counts are subsets of these, since suffix digits
must lie in the set — so these are upper bounds on peeled branching
at the most constrained candidate sets):

| base | profile (exponent vector) | required by sets with… | witnesses | of |
|---|---|---|---|---|
| 40 | (2, 2) | 25 and a v₂=2 digit, no v₂≥3 | **8** | 1482 |
| 40 | (5, 0) | 32, no multiples of 5 | 38 | 1482 |
| 48 | (3, 3) | 27 and a v₂=3 digit, no v₂≥4 | **240** | 97290 |
| 48 | (5, 0) | 32, no multiples of 27-forcing… (3-free) | 2070 | 97290 |

The base-40 (2,2) profile is near-forced: at most 8 last-digit
pairs across the whole base. Agent: list the 8 pairs and intersect
with each live candidate set — for such sets the peeled search's
root branching is single digits.

## 5. Agent actions

1. Apply §1: strike PEELING-SUPPORT.md §3, do not run the a(48)
   check, annotate the file with a pointer here.
2. Add the spectrum computation to the per-base preprocessing:
   minimal infeasible vectors + witness counts at maximal feasible
   vectors, cached per base (one enumeration each).
3. Feed §4's counts into Sol's b48/b40 sizing pass; per candidate
   set, filter the witness lists to the set's digits for exact
   root branching.
4. Pattern note for the audit culture: this is the third
   correction cycle of the campaign and the first sourced from
   this side of the collaboration. The common shape each time:
   a correct derivation whose conclusion restates something
   already known or already enforced. Suggested guard: before
   flagging any new necessary condition as novel, test whether it
   is implied by (a) B ∤ lcm(set), (b) the invariant coset, or
   (c) an existing engine check, on a small brute-forced instance.

## Scripts

```python
import itertools

def vP(n, p):
    v = 0
    while n % p == 0: n //= p; v += 1
    return v

def spectrum_minimal_infeasible(B, primes_caps, s):
    ach = set()
    for tup in itertools.permutations(range(1, B), s):
        x = sum(d * B**j for j, d in enumerate(tup))
        ach.add(tuple(min(vP(x, p), c) for p, c in primes_caps))
    caps = [c for _, c in primes_caps]
    infeasible = [v for v in itertools.product(*[range(c+1) for c in caps])
                  if not any(all(a >= b for a, b in zip(w, v))
                             for w in ach)]
    minimal = [v for v in infeasible
               if not any(w != v and all(a <= b for a, b in zip(w, v))
                          for w in infeasible)]
    return minimal

print(spectrum_minimal_infeasible(40, [(2,5),(5,2)], 2))  # [(3, 1)]
print(spectrum_minimal_infeasible(48, [(2,5),(3,3)], 3))  # [(4, 1)]

def count_at_least(B, s, reqs):
    return sum(1 for tup in itertools.permutations(range(1, B), s)
               if all(vP(sum(d*B**j for j,d in enumerate(tup)), p) >= w
                      for p, w in reqs))

print(count_at_least(40, 2, [(2,2),(5,2)]))  # 8
print(count_at_least(48, 3, [(2,3),(3,3)]))  # 240
```

## Status

Retraction: definitive. Spectrum theorem: exact as labeled;
spectra and counts: exhaustive enumeration, Python 3, stdlib only.
Session date 2026-07-23.
