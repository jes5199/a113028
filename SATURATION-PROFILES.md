# Saturation profiles of the peeled bases: b49, b48, b40

*(For the agent and Sol. Answers the second gating question on the
peeling program: after peeling, are the coprime bodies of the
flagged bases sub-saturated (layer conditions compound with
peeling) or saturated (peeling is the entire win)? Computed from
first principles: layer structure of the coprime modulus, interval
fill depths via exact class DP, window position. Three findings —
one of which qualifies a claim currently in the campaign record —
plus a concrete ordering recommendation. Conventions and caveats
are stated explicitly; scripts at the end. Session 2026-07-23.)*

## Conventions

- Retained modulus: prime powers q of lcm(1..B−1) with
  gcd(q, B) = 1 and ord_q(B) ≥ 2 (order-1 parts are invariant;
  p | B parts are peeled). **Prime powers are kept whole** here;
  the campaign's Q_e convention (reviewer, 2026-07) splits a prime
  power across layers by the largest sub-power with B^e ≡ 1 —
  e.g. 27 contributes its order-3 part 9 to b49's Q₃ = 7353, while
  this note books 27 at its full order 9. Both are valid; the
  difference is bookkeeping and is flagged wherever it matters.
- Layer at exponent e: Q_e = product of retained q with
  ord_q(B) | e; V_e = gcd(Q_e, B−1); fill depth = least m such
  that the interval set {1,…,m} achieves the full V_e-coset
  (exact class-partition DP, bitmask implementation).
- m* computed against the whole retained modulus (this books
  b49 at m* = 22 vs the campaign's 21 — the delta is the
  27-accounting above, not a discrepancy).

## Finding 1 — the "b49's Q₃ never fills" claim cannot be true of intervals

Interval fill depths at B = 49 (window m* ≈ 21–22):

| layer e | Q_e | V_e | coset | interval fill |
|---|---|---|---|---|
| 2 | 800 | 16 | 50 | m = 14 |
| 3 | 817 (= 19·43) | 1 | 817 | m = 12 |
| 4 | 800 | 16 | 50 | m = 14 |
| 5 | 11 | 1 | 11 | m = 5 |
| 7 | 29 | 1 | 29 | m = 7 |
| 8 | 13600 | 16 | 850 | m = 14 |
| 6 | 8,496,800 | 16 | 531,050 | too large to DP here |

Under the campaign's convention Q₃ = 9·19·43 = 7353 (coset 2451
after V = 3); partition counting puts its interval fill near
m ≈ 12 as well. Every DP-able layer saturates by m = 14, deep
inside the window. **Therefore, if the b49 sweep genuinely
encounters sub-saturated fibers in-band, that is a property of the
reachable (gap-scattered) digit sets, not of intervals** — the E4
phenomenon again. Consequence for the b49 instrumentation plan:
the window profile MUST be computed on actual visited sets; the
interval calculation predicts saturation and would wrongly declare
the base dead. Reconciling the recorded "never fills" claim with
these interval numbers (which convention, which digit sets) should
be an explicit item in the instrumentation run.

## Finding 2 — the flagged bases' peeled bodies are DP-inert

B = 48 (window m* ≈ 19):

| layer e | Q_e | coset | interval fill |
|---|---|---|---|
| 2 | 7 | 7 | m = 5 |
| 3 | 13 | 13 | m = 5 |
| 5 | 11 | 11 | m = 5 |
| 6 | 3367 | 3367 | m = 9 |
| 8 | 7 | 7 | m = 8 |

**72% of ln L_eff sits at orders e > 8**, beyond any affordable
layer DP; everything DP-able saturates by m = 9 against a window
of 19.

B = 40 (window m* ≈ 15): a single nontrivial small-order layer
(e = 6, Q = 7, fills at m = 6); **93% of ln L_eff beyond e = 8**.
Fine-splitting 27 (order-3 part 9) adds only a coset-3 sliver at
e = 3; the verdict is robust to the convention.

**Conclusion: at b40 and b48, layer conditions compound with
peeling by essentially nothing.** The remaining cost question is
purely (admissible suffix count) × (coprime sweep cost). Nothing
further is pending or conditional; no cheap-condition upgrade will
change these two bases.

## Finding 3 — b40 is the cheap flagged value

Three independent factors all favor b40 over b48:

| factor | b40 | b48 |
|---|---|---|
| window m* | ≈ 15 | ≈ 19 (≈ 10⁴× in factorial scale) |
| suffix branching cap (CORRECTION-SPECTRUM.md §4) | 8–38 | 240 |
| body | DP-inert | DP-inert |

**Recommendation: run the peeled b40 search first.** If Sol's
sizing pass concurs, a(40) is plausibly settled at desktop scale,
resolving one of the two flagged published values; a(48) likely
still needs monster scale even after peeling, and b49's
instrumentation should proceed in parallel with the
reachable-vs-interval reconciliation of Finding 1 built in.

## Caveats

1. All fill depths here are **interval-certified only** — that
   limitation is Finding 1's entire content, not a footnote.
2. Layer grouping convention as stated; the b49 m* delta (22 vs
   21) is bookkeeping.
3. The e = 6 layer of b49 (Q ≈ 8.5M) was not DP'd here; its
   components' orders divide 6 and its V-structure matches the
   e = 2/3 layers, so interval fill near m ≈ 14–16 is expected but
   not certified.
4. High-order fractions use whole-prime-power accounting; finer
   splitting moves bounded mass downward (quantified for b40
   above) without changing any verdict.

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

def layer_fill(B, e, Q, V, m_max):
    target = Q // V
    for m in range(max(e, 3), m_max + 1):
        ks = [sum(1 for i in range(m) if i % e == r) for r in range(e)]
        wts = [pow(B, r, Q) for r in range(e)]
        dp = {tuple(ks): 1}
        full = (1 << Q) - 1
        for d in range(1, m + 1):
            nxt = {}
            for caps, mask in dp.items():
                for r in range(e):
                    if caps[r]:
                        nc = list(caps); nc[r] -= 1; nc = tuple(nc)
                        a = d * wts[r] % Q
                        sh = ((mask << a) | (mask >> (Q - a))) & full
                        nxt[nc] = nxt.get(nc, 0) | sh
            dp = nxt
        (mask,) = dp.values()
        if bin(mask).count('1') == target:
            return m
    return None

def analyze(B, e_max=8, q_cap=60000):
    M = B - 1
    retained = [(q, ordq(B, q)) for q in prime_powers_of_lcm(M)]
    retained = [(q, e) for q, e in retained if e and e >= 2]
    L = math.prod(q for q, _ in retained)
    mstar, f = 1, 1
    while f < L: mstar += 1; f *= mstar
    print(f"Base {B}: ln L_eff={math.log(L):.1f}, m*={mstar}")
    for e in range(2, e_max + 1):
        comps = [q for q, eq in retained if e % eq == 0]
        if not comps: continue
        Q = math.prod(comps); V = math.gcd(Q, B - 1)
        if Q == V:
            print(f"  e={e}: Q={Q} invariant"); continue
        fill = layer_fill(B, e, Q, V, mstar + 3) if Q < q_cap else None
        print(f"  e={e}: Q={Q} coset={Q//V} fill={fill}")
    hi = math.log(L) - sum(math.log(q) for q, eq in retained
                           if eq <= e_max)
    print(f"  ln-fraction beyond e={e_max}: {hi/math.log(L):.0%}")

for Bv in (49, 48, 40):
    analyze(Bv)
```

## Status

Exact class-DP enumeration for all certified fill depths; interval
digit sets only, as emphasized. Python 3, stdlib only. Session
date 2026-07-23.
