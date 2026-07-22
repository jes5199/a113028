# The Transfer Principle and the Corridor Lemma

*(Companion to BRIDGE-AND-HARDNESS.md and the theorem files. Written
after the Bridge measurement campaign found NC grossly violated on
strong-layer bases (K ≈ 84–122 on b39) with bimodal "corridor"
survival, while weak-layer bases (b38) showed no profile decay at
all. This note gives the corridor phenomenon a mechanism. The core
result — the **Transfer Principle** — is exact and one line deep,
and it yields (a) an exact description of the surviving-children
set, (b) heredity of cheap-condition failure as a theorem, (c) a
forward/backward DP that computes ALL children's survival in ~2 DP
passes instead of m (the concrete candidate for the ~50× DP
speedup the 30× collapse is conditional on), and (d) a
reformulation of Conjecture B's volume as the measure of the rigid
boundary region of the witness structure, replacing NC. Each part
below is labeled **exact / proved / empirical**. Verification
scripts and results at the end. Session 2026-07-23.)*

## Setting

One cyclotomic layer at a time: order e, modulus Q, weights
B^r mod Q constant on position classes r = i mod e. A node at level
m has residual digit set A′ (arbitrary reachable set, per the
widened instance class) and residual target t. The layer's cheap
condition asks for a **witness**: a partition of A′ into classes of
sizes k_r = #{i < m : i ≡ r (mod e)} whose class sums satisfy
Σ_r B^r s_r ≡ t (mod Q). A child places digit d ∈ A′ at position
m−1, whose class is c := (m−1) mod e; the child's class sizes are
k with k_c reduced by one, and its target is t − d·B^c.

## The Transfer Principle  [exact]

**Theorem.** Child d survives the layer **iff** the parent has a
witness that places d in class c.

**Proof.** Given such a parent witness, delete d: class sums drop
by d in class c only, so Σ B^r s_r drops by dB^c, matching the
child target with the child class sizes. Given a child witness,
insert d into class c: the same computation in reverse produces a
parent witness with d in class c. ∎

**Corollary 1 (heredity of failure).** [exact] If the parent fails
the layer, every child fails, hence every descendant fails. (Child
survival ⟹ parent witness exists.) This is the monotonicity the
search's pruning already relies on, now a stated theorem.

**Corollary 2 (surviving-children set).** [exact] The children of
a surviving node that survive all binding layers are exactly

    { d ∈ A′ :  for every binding layer ℓ,
                some layer-ℓ witness places d in class c_ℓ }.

**Mechanism of corridors.** Bimodality follows from witness
*flexibility vs rigidity*. To move a digit d from class r into
class c inside a witness while preserving the congruence, it
suffices to perform two compensated swaps: exchange d ↔ d′
(d′ ∈ class c) and u ↔ v (u ∈ class r, v ∈ class c) with
v − u = d − d′; the two effects (d−d′)(B^c−B^r) cancel. When the
witness's class pools are difference-rich (in particular when they
contain runs), essentially every digit can be rotated into class c
— **all** children survive. When every witness is
difference-starved ("rigid"), few or none can — children die
together. All-or-nothing is what the exchange structure predicts;
the mixed band is exactly the rigid-witness regime.

## Corridor Lemma

**(i) Uniform survival with lookahead.** [proved] If A′ contains a
run of (h+1)(2w+1) consecutive digits with w² ≥ Q/H − 1
(H = gcd(Q, (B−1)·g(A′)), and in practice g = 1 so H = V), then
every descendant within h levels has full-coset layer fibers, so
every such descendant survives the layer. Proof: h arbitrary
deletions split the run into ≤ h+1 pieces, the longest of length
≥ 2w+1; Theorem 1′(b) (as corrected) then gives the full H-coset
at each such node, and residual targets remain in the correct
coset by the invariant. ∎ (This certifies corridor *interior* at
burn-in depths; it does not reach below fill depth.)

**(ii) Certified early death.** [proved] For any class-subset
C ⊆ {0,…,e−1} with total size K_C = Σ_{r∈C} k_r, any witness has
Σ_{r∈C} s_r ∈ [minsum_{K_C}(A′), maxsum_{K_C}(A′)] (sums of the
K_C smallest / largest remaining digits). Evaluating these box
conditions under the most favorable h future removals (bounds
computed from sorted prefix sums of A′ with the h extreme digits
discounted) yields a certificate that **no** level-(m−h)
descendant can survive, h levels before the DP would discover it.
Cost: O(e) per node with incrementally maintained sorted prefix
sums. ∎

**(iii) Band thickness.** [empirical + falsifiable form] No O(1)
bound is proved. What is proved is the characterization: a mixed
node (some children live, some die) requires that for each binding
layer, every witness is rigid for the dying digits — no
compensating difference pair exists in any witness's pools. The
falsifiable prediction for the logs: mixed nodes are exactly those
carrying such a **rigidity certificate**, and their frequency along
descents (the band measure) is what bounds refutation volume.

**Reformulation of Conjecture B.** [replaces NC] Refutation volume
is governed by the measure of the rigid boundary region: corridors
(flexible-witness components) are entered and die as units, so

    Volume  ≈  (# corridor entries) × (band thickness) × (corridor length),

and the analytic program's target becomes bounding corridor counts
and band measure — well-defined combinatorial quantities via
Corollary 2 — instead of the falsified uniform-scattering
hypothesis.

## The forward/backward algorithm  [exact; verified]

Computing children's survival one child at a time costs m DP runs
per node. The Transfer Principle gives all m in ~2 passes:

1. Sort A′ as d₁ < … < d_m. **Forward pass:** F[i] = map from
   class-usage vector to the set of residues achievable using
   d₁…d_i. **Backward pass:** Bk[i] = the same for d_i…d_m.
2. For each child d_i: it survives iff there exist
   (caps_f, r_f) ∈ F[i−1] and residue set Bk[i+1][need] with
   need = k − caps_f − e_c (one slot of class c consumed by d_i)
   containing (t − r_f − d_i·B^c) mod Q.

Correctness is the Transfer Principle: the join enumerates exactly
the parent witnesses with d_i in class c. Verified: 30 random
nodes (B = 10, Q = 91, e = 6, mixed digit sets with gaps), the
join agrees with direct per-child DPs on every child, zero
mismatches.

Engineering notes for Engine C: (a) the two passes share the
existing DP machinery; (b) F and Bk can be maintained incrementally
along the descent (child's forward pass extends the parent's up to
the deleted digit's index); (c) memory is the constraint — cap the
per-level state maps and fall back to direct DP on overflow;
(d) combined with (ii)'s O(e) early-death certificates gating when
any DP runs at all, this is the concrete path to making
per-node joint conditions cheaper than they prune. The 30×
collapse measured on strong-layer bases is conditional on exactly
this.

## Synthetic bimodality check  [empirical]

Random descents on a single layer (B = 10, Q = 91, e = 6, starting
from {1,…,16}, following random surviving children): distribution
of surviving-children fraction at surviving nodes, 720 node
samples:

    0.2–0.7 :  86 nodes  (mixed band)
    0.8–1.0 : 634 nodes  (dominated by 0.9–1.0: 617)

Strongly bimodal from the mechanism alone — no joint layers, no
real search dynamics. Caveats: one layer, small Q, and the walk
conditions on survival; this is mechanism-consistency, not a b39
replication.

## Test plan for the real logs (agent)

1. **Decile histogram** of surviving-children fraction at
   surviving nodes on b39 (and b38 for contrast), computed with
   the forward/backward algorithm — confirms or refutes that the
   real bimodality matches the witness-flexibility mechanism.
2. **Rigidity certificates at mixed nodes**: for a sample of mixed
   nodes, extract witnesses and test difference-completeness of
   class pools; prediction (iii): mixed ⟺ rigid.
3. **Band measure**: frequency of mixed nodes along descents, per
   level — the quantity that now stands where NC's K stood.
4. **Speedup measurement**: forward/backward vs per-child DP wall
   time at real node sizes; then re-run the b43-class comparison
   that previously timed out, with (ii)'s certificates gating DP
   invocation.
5. **Corridor census**: connected components of surviving nodes
   under the child relation on a manageable window slice — counts
   and lengths feed the reformulated volume bound.

## Verification script

```python
import itertools, random
from collections import Counter
random.seed(11)

def dp_feasible(B, digits, m_positions, Q, e, t):
    ks = [sum(1 for i in range(m_positions) if i % e == r)
          for r in range(e)]
    wts = [pow(B, r, Q) for r in range(e)]
    dp = {(tuple(ks), 0)}
    for d in digits:
        nxt = set()
        for caps, res in dp:
            for r in range(e):
                if caps[r]:
                    nc = list(caps); nc[r] -= 1
                    nxt.add((tuple(nc), (res + d*wts[r]) % Q))
        dp = nxt
    return any(caps == (0,)*e and res == t % Q for caps, res in dp)

def children_direct(B, digits, Q, e, t):
    m = len(digits); c = (m-1) % e
    return {d: dp_feasible(B, [x for x in digits if x != d], m-1,
                           Q, e, (t - d*pow(B, c, Q)) % Q)
            for d in digits}

def children_fb(B, digits, Q, e, t):
    m = len(digits); c = (m-1) % e
    ks = tuple(sum(1 for i in range(m) if i % e == r)
               for r in range(e))
    wts = [pow(B, r, Q) for r in range(e)]
    ds = sorted(digits)
    fwd = [dict() for _ in range(m+1)]; fwd[0][(0,)*e] = {0}
    for i, d in enumerate(ds):
        for caps, ress in fwd[i].items():
            for r in range(e):
                if caps[r] < ks[r]:
                    nc = list(caps); nc[r] += 1; nc = tuple(nc)
                    fwd[i+1].setdefault(nc, set()).update(
                        (x + d*wts[r]) % Q for x in ress)
    bwd = [dict() for _ in range(m+1)]; bwd[m][(0,)*e] = {0}
    for i in range(m-1, -1, -1):
        d = ds[i]
        for caps, ress in bwd[i+1].items():
            for r in range(e):
                if caps[r] < ks[r]:
                    nc = list(caps); nc[r] += 1; nc = tuple(nc)
                    bwd[i].setdefault(nc, set()).update(
                        (x + d*wts[r]) % Q for x in ress)
    out = {}
    for i, d in enumerate(ds):
        ok = False
        for caps_f, ress_f in fwd[i].items():
            if caps_f[c] >= ks[c]: continue
            need = tuple(ks[r] - caps_f[r] - (1 if r == c else 0)
                         for r in range(e))
            ress_b = bwd[i+1].get(need)
            if not ress_b: continue
            if any(((t - rf - d*wts[c]) % Q) in ress_b
                   for rf in ress_f):
                ok = True
            if ok: break
        out[d] = ok
    return out

# Exactness: 30 random nodes, gaps included
B, Q, e = 10, 91, 6
for _ in range(30):
    m = random.randint(7, 10)
    pool = sorted(random.sample(range(1, 25), m))
    t = random.randrange(Q)
    assert children_direct(B, pool, Q, e, t) == \
           children_fb(B, pool, Q, e, t)
print("fb == direct on 30 random nodes")

# Bimodality on synthetic descents
fracs = []
for _ in range(60):
    digits = list(range(1, 17)); t = random.randrange(Q)
    if not dp_feasible(B, digits, len(digits), Q, e, t): continue
    while len(digits) > 4:
        surv = children_fb(B, digits, Q, e, t)
        alive = [d for d, ok in surv.items() if ok]
        fracs.append(len(alive)/len(digits))
        if not alive: break
        d = random.choice(alive)
        c = (len(digits)-1) % e
        t = (t - d*pow(B, c, Q)) % Q
        digits.remove(d)
print(Counter(min(int(f*10), 9) for f in fracs))
```

## Status

Transfer Principle, Corollaries 1–2, Lemma (i), (ii), and the
forward/backward algorithm: exact/proved as labeled, verified by
enumeration where shown; not refereed. Lemma (iii) and the volume
reformulation: empirical with the stated falsifiable forms. Python
3, stdlib only. Session date 2026-07-23.
