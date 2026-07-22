# Errata and addendum: digit-stride isolation, the digit-gcd invariant, and reachable-node corrections

*(Adopts the external review of THEOREM-2-DOUBLE-PRIME.md in full
("Sol", second review, 2026-07-22). Part A: patches to
THEOREM-2-DOUBLE-PRIME.md. Part B: the **digit-gcd lemma** — a new,
strictly finer per-node invariant, proved and verified. Part C: a
constructed example that satisfies Theorem 2″'s actual hypotheses,
verified. Part D: collateral patches to BRIDGE-AND-HARDNESS.md.
Part E: an open design question for the maintainer, not an erratum.
Scripts at the end. Session 2026-07-22.)*

---

## Part A — Patches to THEOREM-2-DOUBLE-PRIME.md

### A1. Retitle

"Theorem 2″: unconditional joint exactness" → **"Theorem 2″: joint
exactness via digit-stride isolation."** The theorem carries
substantial AP and position-capacity hypotheses; "unconditional"
referred only to the absence of arithmetic side conditions and is
rhetorically misleading. Its accurate billing: a **CRT isolation
gadget** and a family of **explicit saturation bounds**, with a
partition/stride tradeoff (Remark 4) as the tuning lever.

### A2. Fix the containment paragraph

The sentence deriving ℓ ≥ 2 from "the required APs exist" is wrong
(an AP of stride > 1 contains no consecutive pair), and completeness
is not needed there. Replace the containment paragraph with:

> **Containment** follows directly from B ≡ 1 (mod V); no
> completeness clause from Theorem 1′ is needed. If every w_j = 0
> then Q_j = V_j for every group, hence Λ = V and containment is
> already equality. Otherwise the gadget construction below proves
> coverage.

(Completeness — the exact perturbation subgroup — is treated
separately and more sharply by the digit-gcd lemma, Part B.)

### A3. Downgrade Remark 2 ("qualitative half of Conjecture A")

Two corrections. (i) The k = 1 case of Theorem 2″ recovers corrected
Theorem 1′ with Q = Λ, so fixed-modulus eventual saturation was
already available; the new content is the isolation gadget and the
improved explicit depth via the partition/stride tradeoff. (ii) The
radix model requires A ⊆ {1,…,B−1}: for fixed B there is no
unbounded m, and m₀(Λ, B) may greatly exceed B−1; in the A113028
family Λ grows with B, so fixed-Λ saturation gives no uniform
information near m*(Λ). Replace the remark with:

> **In the generalized interval-coefficient model** (coefficient
> sets A ⊆ {1,…,M} of unbounded size, weights B^i mod Λ), every
> fixed pair (B, Λ) has an explicit finite saturation bound beyond
> which the invariant congruence is exactly sufficient. **Within
> the radix-digit model**, the conclusion applies whenever B−1 is
> large enough to contain the required gadgets.

### A4. Remove the anti-conjecture claims

Delete "the anti-conjecture cannot rest on any algebraic
obstruction at any depth" and "whatever hardness exists lives at
threshold scale." The theorem rules out a congruence obstruction
persisting beyond its (large) sufficient bound in the generalized
model; NP-hardness could occur entirely below that bound, or in a
family where Λ grows with B — which is the case of interest.

### A5. Engine C corrections (supersedes the E3/E4 wording)

Reachable DFS descendants in engineC_rec do **not** stay
interval-minus-C₀: every selected digit is cleared from c_avail,
and successive nonmaximal choices create arbitrarily many internal
gaps. Therefore: compute the actual longest run ℓ directly from
c_avail; never infer it from the root's C₀; do not enumerate only
bounded-hole patterns as though they covered reachable nodes.
Exact-mask caching is safe; interval-pattern caching would require
an additional proof. The per-node cheap condition that *does* work
for arbitrary c_avail is the digit-gcd condition of Part B.

### A6. Relabel the verification table

Retitle it "**Additional exact-coverage data**": those computations
are exact-enumeration evidence for early coset fill, but the depths
involved do not satisfy Theorem 2″'s AP/slot hypotheses, so they do
not exercise the construction. The example that does is Part C.

---

## Part B — The digit-gcd lemma (new invariant)

**Lemma.** For a digit set A with |A| ≥ 2, let
g(A) = gcd_{d,d′∈A}(d − d′) and

```
H = gcd(Λ, (B−1) · g(A)).
```

Then the differences N(σ) − N(σ′) over arrangements generate
exactly H·Z_Λ; equivalently, {N(σ) mod Λ} lies in a single coset of
H·Z_Λ, and no finer congruence holds across all arrangements.

**Proof.** *Upper bound:* any two arrangements are connected by
transpositions, and a transposition of d, d′ at positions j, j+Δ
changes N by (d−d′)·B^j·(B^Δ−1); here g | (d−d′) and
(B−1) | (B^Δ−1) as integers, so every difference lies in
(B−1)g·Z_Λ ⊆ H·Z_Λ. *Lower bound:* fix an adjacent position pair
(j, j+1) and perform transpositions there with varying digit pairs;
the effects add, and integer combinations of the pairwise
differences realize g itself, giving g·B^j·(B−1) in the difference
group for every j; the additive span of {B^j} contains 1, so the
group contains (B−1)g·Z_Λ, whose intersection with Z_Λ is H·Z_Λ. ∎

**Relation to prior results.** g(A) = 1 recovers the corrected
Theorem 1′(a) completeness (H = V) — and note ℓ ≥ 2 ⟹ g = 1, so
this lemma strictly subsumes that clause. Under Theorem 2″'s
hypotheses g(A) = 1 automatically: g divides every active stride
S_j (within-AP differences are multiples of S_j), and the gcd of
the active strides is 1 (each prime of Λ is absent from its own
group's stride). The odd-digit counterexample of the previous
errata is now *derived*: g = 2, H = gcd(800, 96) = 32, image =
one full coset of size 800/32 = 25.

**Verification** (exact enumeration / class DP):

| B | A | Λ | g | H predicted | observed |
|---|---|---|---|---|---|
| 4 | {2,5,8} | 27 | 3 | 9 | difference-gcd = 9 ✓ |
| 10 | {3,9,15,21} | 81 | 6 | 27 | difference-gcd = 27 ✓ |
| 49 | odd digits 1..27 | 800 | 2 | 32 | image = 25 = 800/32 ✓ |

**Engine C condition (per node, O(m)).** At a node with remaining
digits c_avail and residual target t_rem: compute g (gcd of
differences from the minimum), H = gcd(Λ_node, (B−1)g), evaluate N
of one canonical arrangement of c_avail, and prune unless
t_rem ≡ N_canonical (mod H). Unlike the mod-V invariant (root-only,
per the Bridge errata), **H changes as digits are removed** —
whenever the surviving digits share a common stride, H jumps and
the condition prunes. This is the correct cheap condition for the
gap-scattered node population identified in A5.

---

## Part C — Constructed example satisfying Theorem 2″'s hypotheses

Interval-coefficient model (necessarily — the radix constraint
makes small radix-model examples impossible, per A3(ii)):

- B = 3, Λ = 35 = 5·7, groups {5}, {7}; V = gcd(35, 2) = 1;
  E = lcm(ord₅(3), ord₇(3)) = lcm(4, 6) = 12.
- w₁ = 2 (w₁² = 4 ≥ 5−1), w₂ = 3 (w₂² = 9 ≥ 7−1);
  strides S₁ = 7, S₂ = 5.
- Digit APs: U₁ = {7, 14, 21, 28} (stride 7, length 2w₁ = 4),
  U₂ = {5, 10, 15, 20, 25, 30} (stride 5, length 2w₂ = 6),
  disjoint, inside A = {1,…,72}.
- Positions m = 72: class 0 = {0,12,…,60} and class 1 =
  {1,13,…,61} each have 6 slots ≥ Σw_j + 1 = 6 ✓; weights are
  constant on each class mod 35 (ord | 12 for both primes).
- All 62 non-gadget digits frozen in a fixed arbitrary assignment.

Sweeping the two gadgets independently (6 × 20 = 120
configurations) and computing N mod 35 for each: **all 35 residues
are attained by the gadget sweeps alone**, with everything else
frozen — verifying the isolation mechanism itself, not merely the
coverage phenomenon. (Script below.)

---

## Part D — Patches to BRIDGE-AND-HARDNESS.md

1. **Strike the shape parenthetical** in the Bridge Theorem proof
   ("the search removes the placed digit from the top of the
   residual interval, preserving the shape class") — false for
   engineC_rec descendants (A5). The theorem is unaffected: 𝒜_m is
   already defined as the *reachable* residual sets, whatever their
   shape, and T̄_m majorizes those. NC's setting paragraph should
   likewise describe A′ only as "the residual digit set" without a
   shape claim.
2. **Measurement protocol step 1**: replace "hole patterns" language
   with "residual sets actually occurring in the sweep logs"; add
   that per-node g(A′) and H should be recorded alongside |T(A′)| —
   stride sub-invariants are expected to be a major source of fiber
   shrinkage at gap-scattered nodes, and the H-condition (Part B)
   should be included among the cheap conditions being profiled.
3. **Part II consequence paragraph**: delete "any hardness of the
   interval family must be located at threshold scale." Retain the
   correct statement: hardness of unrestricted PERM-FEAS flows
   through adversarial digit sets; for the interval family nothing
   is proved in either direction.

---

## Part E — Open design question (maintainer decision, not an erratum)

The conjectures' instance class (OPEN-PROBLEM premise 1:
interval-minus-C₀) and the sweep's actual node population
(arbitrary gap patterns, A5) have diverged. Two coherent responses:

- **Widen the conjectures** to the reachable-set class, with the
  digit-gcd invariant H replacing V in the coset statements (the
  fiber structure then depends on per-node g); or
- **Restructure the sweep** so refutation frontiers are
  bounded-hole states (e.g. refute via largest-first completion
  with conjecture-grade pruning applied only at bounded-hole
  frontier nodes), keeping the current instance class honest.

The choice materially affects what the measurement protocol will
find (clumping vs stride-shrinkage are confounded if unprofiled)
and should be made before the protocol runs.

---

## Scripts

```python
import itertools, math

# Digit-gcd lemma checks
def image_bruteforce(B, A, Q):
    return {sum(d*pow(B,p,Q) for d,p in zip(A,perm)) % Q
            for perm in itertools.permutations(range(len(A)))}

def gA(A):
    m0 = min(A)
    return math.gcd(*[d - m0 for d in A if d != m0])

for B, A, Q in [(4, [2,5,8], 27), (10, [3,9,15,21], 81)]:
    img = sorted(image_bruteforce(B, A, Q))
    H = math.gcd(Q, (B-1)*gA(A))
    diffs = {(x-img[0]) % Q for x in img}
    gen = math.gcd(Q, *diffs)
    print(B, A, Q, "H:", H, "observed:", gen, gen == H)

# Odd-digit case via class DP (E = 2)
def class_image(B, digits, E, Q):
    m = len(digits)
    ks = [sum(1 for i in range(m) if i % E == r) for r in range(E)]
    wts = [pow(B, r, Q) for r in range(E)]
    dp = {tuple(ks): {0}}
    for d in digits:
        nxt = {}
        for caps, res in dp.items():
            for r in range(E):
                if caps[r]:
                    nc = list(caps); nc[r] -= 1
                    nxt.setdefault(tuple(nc), set()).update(
                        (x + d*wts[r]) % Q for x in res)
        dp = nxt
    (im,) = dp.values()
    return im

odd = list(range(1, 28, 2))
H = math.gcd(800, 48*gA(odd))
print("odd digits:", len(class_image(49, odd, 2, 800)), "== 800/H:",
      800 // H)

# Constructed example: B=3, Lambda=35, m=72
B, Lam = 3, 35
U1, w1 = [7,14,21,28], 2
U2, w2 = [5,10,15,20,25,30], 3
A_all = list(range(1, 73)); gad = set(U1) | set(U2)
rest = [d for d in A_all if d not in gad]
m = len(A_all)
class0 = [i for i in range(m) if i % 12 == 0]
class1 = [i for i in range(m) if i % 12 == 1]
other  = [i for i in range(m) if i % 12 not in (0, 1)]
slots_frozen = other + class0[5:] + class1[5:]
frozen = sum(d*pow(B, i, Lam) for d, i in zip(rest, slots_frozen)) % Lam
w0, w1w = pow(B, class0[0], Lam), pow(B, class1[0], Lam)
hits = set()
for S1 in itertools.combinations(U1, w1):
    for S2 in itertools.combinations(U2, w2):
        s1, s2 = sum(S1), sum(S2)
        N = (frozen + s1*w0 + (sum(U1)-s1)*w1w
                    + s2*w0 + (sum(U2)-s2)*w1w) % Lam
        hits.add(N)
print("constructed example covers", len(hits), "/ 35")
```

## Status

Digit-gcd lemma: elementary, proof above, verified by exact
enumeration; credit for the statement and the g | S_j observation:
external review ("Sol", 2026-07-22). Constructed example: verified,
script above. All other items are adopted review corrections.
Python 3, stdlib only. Session date 2026-07-22.
