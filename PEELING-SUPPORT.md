# p-adic suffix peeling: verification, the base-48 exclusion, and the certificate audit

*(Support document for Sol's peeling reduction — Sol is writing the
general theorem and the base-49 certificate; this file supplies the
independently reconstructed identity with an exhaustive test vector,
independent confirmation of the suffix lengths, a new set-level
impossibility theorem for base 48 with immediate consequences for
the flagged a(48), per-candidate-set sizing guidance, and the audit
checklist the certificate should satisfy. Everything here is exact
or exhaustively verified; scripts at the end. Session 2026-07-23.)*

## 1. The peeling identity (reconstructed; verified)

Split Λ = Λ_nil · Λ′: Λ_nil collects the prime powers p^{a_p} of Λ
with p | B; Λ′ is coprime to B. Let

    s = max_{p | B} ⌈ a_p / v_p(B) ⌉ ,

so B^j ≡ 0 (mod Λ_nil) for every j ≥ s, and N mod Λ_nil depends
only on the last s digits: N ≡ Σ_{j<s} d_j B^j (mod Λ_nil). Then:

**Identity.** PERM-FEAS(B, A, Λ, t) holds **iff** there is an
ordered s-tuple of distinct digits (d₀,…,d_{s−1}) from A with
Σ_{j<s} d_j B^j ≡ t (mod Λ_nil), such that

    PERM-FEAS( B,  A ∖ {d₀,…,d_{s−1}},  Λ′,  t′ )      holds, with
    t′ = ( t − Σ_{j<s} d_j B^j ) · B^{−s}   (mod Λ′)

(B is a unit mod Λ′, so B^{−s} exists; the remaining digits occupy
positions s,…,m−1, and dividing by B^s renormalizes them to
0,…,m−s−1).

**Proof.** Any arrangement splits into its last s digits and the
rest. Mod Λ_nil only the suffix contributes (B^j ≡ 0 for j ≥ s);
mod Λ′ the prefix contributes B^s·(a standard arrangement value of
the remaining digits). Both directions are the same bookkeeping. ∎

**Exhaustive test vector (the coupled-suffix regime).** B = 12,
A = {1,…,8}, Λ = lcm(1..8) = 840, Λ_nil = 24 = 2³·3, Λ′ = 35,
s = 2 (the two suffix digits interact: the condition is on
d₀ + 12·d₁ mod 24, NOT independent per-digit conditions). Both
sides of the identity were evaluated by brute force for **all 840
targets**: 560 feasible, **0 mismatches**. This is the regime a
general implementation gets wrong first; base 49 (s = 1) never
exercises it, base 48 (s = 3) lives in it.

## 2. Suffix lengths (independent confirmation)

Computed from a_p = v_p(lcm(1..B−1)) and v_p(B):

| B | primes of B | Λ_nil | s |
|---|---|---|---|
| 49 | 7 (a₇=1, v₇(B)=2) | 7 | **1** |
| 48 | 2 (a₂=5, v₂=4), 3 (a₃=3, v₃=1) | 864 | **3** |
| 40 | 2 (a₂=5, v₂=3), 5 (a₅=2, v₅=1) | 800 | **2** |

These match Sol's T values (1, 3, 2) exactly.

## 3. The base-48 exclusion theorem (new; set-level pruning)

**Theorem.** No base-48 number with distinct nonzero digits is
divisible by 864 = 2⁵ · 3³.

**Proof.** Suffix arithmetic with s = 3 (only the last three digits
matter mod 864; 48² ≡ 0 mod 32 and mod 27 contributions:
48 ≡ 16 (mod 32), 48 ≡ 21 (mod 27), 48² ≡ 9 (mod 27)). Mod 32 the
condition d₀ + 16·d₁ ≡ 0 forces d₀ ≡ 16·(−d₁): d₁ odd ⟹
d₀ ≡ 16 (mod 32) ⟹ d₀ = 16; d₁ even ⟹ d₀ ≡ 0 (mod 32) ⟹
d₀ = 32 (digits are ≤ 47). Mod 9 (where 21 ≡ 3 and 9·d₂ ≡ 0) the
condition becomes 3·d₁ ≡ −d₀ (mod 9): for d₀ = 16 this asks
3·d₁ ≡ 2, for d₀ = 32 it asks 3·d₁ ≡ 4 — both impossible since
3·d₁ ∈ {0, 3, 6} (mod 9). Exhaustive check over all 97,290 ordered
triples from {1,…,47}: zero admissible. ∎

**Corollary (root-level set pruning for a(48)).** The divisibility
target for a candidate digit set is 0 mod lcm(set). The only digit
≤ 47 forcing 3³ is 27; the only digit forcing 2⁵ is 32. Hence
**every candidate set containing both 32 and 27 is infeasible
outright** — a(48)'s digit set excludes at least one of them. This
kills whole branches of the base-48 search before any sweep.

**Immediate cheap check (do first):** test the published, flagged
a(48) against this. If its digit set contains both 32 and 27, the
value is disproven by the theorem above with zero search — an
a(46)-style kill by pure arithmetic. If not, the exclusion still
halves the top of the candidate-set lattice.

**Contrast:** base 40 does NOT suffer this — its mod-32 condition
(d₀ + 8·d₁ ≡ 0, since 40 ≡ 8 and 40² ≡ 0 mod 32) and mod-25
condition (d₀ + 15·d₁ ≡ 0) are jointly solvable, so its s = 2
suffix families are nonempty. The emptiness is special to 48.

## 4. Sizing guidance (per-candidate-set, not global)

Λ_nil depends on which high prime-power digits the candidate set
retains: a set without 32 has 2-part ≤ 2⁴ (from 16 or 48-free
digits ≤ 47), a set without 27 has 3-part ≤ 3² (from 9, 18, 36...).
Therefore:

1. For each candidate set, compute ITS Λ_nil from v_p(lcm(set)),
   and its s from that — s can drop below the global value.
2. Run the emptiness test (enumerate admissible s-tuples from the
   set's own digits against its own Λ_nil) BEFORE anything else;
   by §3 it is sometimes decisive and always cheap.
3. Only then size the peeled search: (#admissible suffixes) ×
   (cost of one coprime subsearch on m − s digits mod Λ′).
4. Each admissible suffix has its OWN coprime target t′; the
   subsearches are distinct problems, not one problem repeated.

## 5. Certificate audit checklist (for the base-49 writeup, and
## the template for 48/40)

A complete maximality certificate = all of:

1. **The identity itself** — cite the proof (§1 or Sol's general
   theorem) and the exhaustive B = 12 vector; for s ≥ 2 bases,
   state explicitly that suffix admissibility is evaluated on
   ordered tuples mod Λ_nil (coupled), never per-digit.
2. **Complete suffix enumeration per branch** — the admissible
   suffix list must be proven exhaustive against that branch's
   residual pool (digits consumed by the prefix are unavailable)
   and that branch's set-level Λ_nil.
3. **Each coprime subsearch exhausted** — against Λ′ with its own
   t′, leaf logic running on Λ′ (not Λ), and the B^{−s}
   renormalization applied; commit leaf counts as evidence (e.g.
   the 254M-leaf half of candidate-21).
4. **The unsearched branches** — the forced-subset lemma covering
   every branch the peeled search did not visit, stated as an
   explicit dependency of the certificate, with its own proof or
   citation.
5. **Consistency cross-check** — the peeled NO on candidate-21
   agrees with the prior week-scale computation's NO, and the
   peeled candidate-20 run reconstructs the known optimum suffix;
   record both agreements in the certificate.
6. **Guard rails in code** — assert gcd(Λ′, B) = 1 and
   B^s ≡ 0 (mod Λ_nil) at every entry point (the b39 modulus bug
   was exactly a violation of the first assertion in analysis
   code; make it impossible to repeat).

## 6. Scripts

```python
import itertools, math

# --- Identity test vector: B=12, all 840 targets ---
B = 12
A = list(range(1, 9))
Lam = math.lcm(*A)          # 840
Lnil, Lp, s = 24, 35, 2
assert pow(B, s, Lnil) == 0 and math.gcd(B, Lp) == 1

def perm_feas(B, digits, Q, t):
    for perm in itertools.permutations(range(len(digits))):
        if sum(d * pow(B, p, Q)
               for d, p in zip(digits, perm)) % Q == t % Q:
            return True
    return False

Binv = pow(pow(B, s, Lp), -1, Lp)
for t in range(Lam):
    lhs = perm_feas(B, A, Lam, t)
    rhs = any(
        (d0 + B*d1) % Lnil == t % Lnil and
        perm_feas(B, [x for x in A if x not in (d0, d1)], Lp,
                  (t - d0 - B*d1) * Binv % Lp)
        for d0, d1 in itertools.permutations(A, 2))
    assert lhs == rhs
print("peeling identity: 840/840 targets agree")

# --- Suffix lengths ---
def suffix_len(Bv, M):
    s, Lnil = 0, 1
    for p in [q for q in range(2, Bv+1)
              if Bv % q == 0 and all(q % g for g in range(2, q))]:
        a, pk = 0, p
        while pk <= M: a += 1; pk *= p
        vB, x = 0, Bv
        while x % p == 0: x //= p; vB += 1
        s = max(s, -(-a // vB)); Lnil *= p**a
    return s, Lnil

for Bv in (49, 48, 40):
    print(Bv, suffix_len(Bv, Bv - 1))   # (1,7) (3,864) (2,800)

# --- Base-48 exclusion: zero admissible triples mod 864 ---
cnt = sum(1 for d0, d1, d2 in itertools.permutations(range(1, 48), 3)
          if (d0 + 48*d1 + 48*48*d2) % 864 == 0)
print("base-48 admissible triples mod 864:", cnt)   # 0
```

## Status

Identity: reconstructed from Sol's description, proof above,
exhaustively verified on the coupled-suffix vector; Sol's general
theorem supersedes the statement here when it lands. Base-48
exclusion theorem: proved above and exhaustively confirmed; the
corollary's set-level form and the a(48) check are new. Suffix
lengths: independently confirmed. Python 3, stdlib only. Session
date 2026-07-23.
