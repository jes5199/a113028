# The Irreducibility Law

*(Capstone negative of the cheap-conditions program. The campaign
found empirically — b38, b39, then the saturation profiles of
peeled b40/b48 — that congruence pruning cannot compress the
refutation sweep. This note proves that this was never
base-specific bad luck: it is a two-line counting law. Every
bounded-order congruence condition, jointly, addresses a vanishing
fraction of the modulus, so profile decay in the Bridge sense is
impossible for this condition class at every base. The law also
delimits itself: it bounds marginal congruence conditions, not
problem transformations — and nilpotent peeling, which escaped it,
is the existence proof that outside-class wins exist. Session
2026-07-24.)*

## Setting

Fix base B. As in OPEN-PROBLEM.md, L_eff is the product of the
prime powers q of lcm(1,…,B−1) with gcd(q, B) = 1 and
ord_q(B) ≥ 2; ln L_eff = ψ(B−1) − O(log² B), and by Chebyshev/PNT
ψ(x) ~ x (with explicit lower bounds ψ(x) ≥ c₀·x available for a
positive constant c₀; asymptotically c₀ → 1). The window sits at
m* = min{m : m! ≥ L_eff} ≈ B/ln B.

**Condition class.** A *bounded-order congruence condition of
order ≤ E₀* is any necessary condition on a node that factors
through N mod Q for some Q dividing

    Q_{≤E₀} := Π_{q ∥ L_eff, ord_q(B) ≤ E₀} q .

This class contains everything the cheap-conditions program ever
fielded: the order-1 invariants, the class-partition DPs, the
joint cyclotomic-layer DPs (reviewer, 2026-07), the exact-coset
conditions of Theorem 1′, the digit-gcd condition, and any future
condition computed by a DP over position classes mod e ≤ E₀ —
regardless of how cleverly it is evaluated.

## The Law

**Theorem.** ln Q_{≤E₀} < E₀² · ln B. Consequently the fraction of
the modulus (in log measure) addressable by ALL bounded-order
conditions jointly satisfies

    ln Q_{≤E₀} / ln L_eff  <  E₀² ln B / (c₀ B)  →  0

for every E₀ = o(√(B / log B)).

**Proof.** Every prime power q with ord_q(B) = e divides B^e − 1.
Hence Q_{≤E₀} divides Π_{e≤E₀}(B^e − 1) (with multiplicity
bookkeeping only weakening the bound), so

    ln Q_{≤E₀} ≤ Σ_{e=1}^{E₀} ln(B^e − 1) < ln B · Σ_{e=1}^{E₀} e
               < E₀² ln B .

The denominator is ln L_eff ≥ c₀ B − O(log² B) by Chebyshev. ∎

## Corollary: no profile decay, hence no sweep compression

In the Bridge Theorem's terms, decay (β_m < 1) at level m requires
the conditions to shrink fibers to |T_m| < L_eff/(V·m), i.e. to
capture all but ~ln m of ln L_eff. Bounded-order conditions
capture at most E₀² ln B ≪ c₀ B, so decay cannot begin above
levels m with ln m! ≲ E₀² ln B — i.e. m = O(E₀² ln B / ln ln B),
far below the window. Under the lower-bound direction of NC
(uniformity of survivors — *measured* on b38/b39 at K ≈ 2, not
proved; this corollary is conditional on it in the same way all
campaign volume statements are), the refutation volume satisfies

    Volume  ≥  m₀! / B^{O(E₀²)} ,

which is exp(c·B − O(E₀² log B)) at the window — superpolynomial
for every E₀ = o(√(B/log B)), i.e. for every remotely computable
condition order. **The cheap-conditions program could never have
dissolved the frontier, at any base, and this is why.**

## Per-base instances (computed, bases 38–65)

Captured fraction of ln L_eff by orders e ≤ E₀ (exact, whole
prime-power accounting; finer cyclotomic splitting moves bounded
mass without changing any conclusion):

| B | ln L_eff | m* | E₀=3 | E₀=5 | E₀=8 | E₀=12 |
|---|---|---|---|---|---|---|
| 38 | 26.2 | 15 | 10% | 30% | 37% | 37% |
| 39 | 27.4 | 15 | 7% | 32% | 32% | 64% |
| 40 | 27.0 | 15 | 0% | 0% | 7% | 28% |
| 41* | 36.2 | 18 | 5% | 33% | 33% | 55% |
| 43* | 38.0 | 19 | 6% | 24% | 48% | 64% |
| 47* | 40.6 | 20 | 17% | 45% | 59% | 66% |
| 48 | 36.9 | 19 | 12% | 19% | 28% | 37% |
| 49 | 45.6 | 22 | 29% | 35% | 54% | 76% |
| 53* | 46.9 | 22 | 7% | 12% | 33% | 40% |
| 59* | 50.1 | 23 | 0% | 12% | 32% | 50% |
| 61* | 57.5 | 25 | 10% | 16% | 22% | 37% |
| 64 | 58.2 | 26 | 15% | 30% | 49% | 74% |
| 65 | 52.4 | 24 | 5% | 5% | 17% | 17% |

(* = prime base; full table for 38–65 in the script's output.)
Maximum captured anywhere, even at the unaffordable E₀ = 12: 76%.
Decay needs ~100%. The campaign's measured instances — b38/b39
saturation with NC holding at K ≈ 2, and the peeled-body profiles
of SATURATION-PROFILES.md (7%, 28%, 54% at E₀ = 8 for b40/48/49)
— are rows of this table behaving exactly as the Law requires.

## What escapes the Law (and must, for any future speedup)

The Law bounds necessary conditions that are *functions of
N mod Q_{≤E₀}* — marginal congruence filters. It does NOT bound:

1. **Exact problem transformations.** Nilpotent peeling is not a
   pruning condition; it rewrites the problem (removing digits,
   shrinking the modulus, lowering the leaf horizon) and delivered
   the campaign's only large measured win (~2000× at b49, 7.3 s
   full solve at b40). Peeling is the existence proof that
   outside-class speedups are real. It is inert at prime bases
   (T = 0), which is why the frontier now lives there.
2. **Meet-in-the-middle.** Splitting positions and hashing one
   half attacks m! directly (≈ square root, memory h!), touching
   the factorial rather than the modulus — orthogonal to the Law.
   Untested in this campaign; the designated next experiment.
3. **Algebraic infeasibility certificates.** Nullstellensatz-style
   proofs of non-existence for specific window instances (cf.
   Nagy's methods) would replace sweeps with proofs; unexplored.
4. **High-order information used non-marginally.** The Law caps
   what conditions of order ≤ E₀ can see; methods that use the
   full arithmetic of high-order layers other than as per-node
   filters (e.g. steering the descent by reachable-set fiber
   structure, if b49's in-band sub-saturation is confirmed on
   visited sets) are outside its scope.

## Status and scope notes

The Theorem is unconditional and elementary. The Corollary's
volume lower bound is conditional on lower-NC exactly as labeled;
its conclusion matches every measured sweep in the campaign. The
constant c₀ is Chebyshev's; nothing here needs more than
ln L_eff ≥ c₀B. Whole-prime-power layer accounting as in
SATURATION-PROFILES.md ("Conventions"). Table computed by exact
enumeration; script below. Python 3, stdlib only. Session date
2026-07-24.

## Script

```python
import math

def ordq(B, q):
    if math.gcd(B, q) != 1: return None
    x, e = B % q, 1
    while x != 1:
        x = x * B % q; e += 1
    return e

def prime_powers_of_lcm(M):
    out = []
    for p in range(2, M + 1):
        if all(p % g for g in range(2, int(p**0.5) + 1)):
            pk = p
            while pk * p <= M: pk *= p
            out.append(pk)
    return out

for B in range(38, 66):
    retained = [(q, ordq(B, q)) for q in prime_powers_of_lcm(B - 1)]
    retained = [(q, e) for q, e in retained if e and e >= 2]
    lnL = sum(math.log(q) for q, _ in retained)
    mstar, lf = 1, 0.0
    while lf < lnL:
        mstar += 1; lf += math.log(mstar)
    fr = {E0: sum(math.log(q) for q, e in retained if e <= E0) / lnL
          for E0 in (3, 5, 8, 12)}
    print(B, f"{lnL:.1f}", mstar,
          *(f"{fr[E0]:.0%}" for E0 in (3, 5, 8, 12)))
```
