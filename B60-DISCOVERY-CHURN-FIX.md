# B60 Discovery-Churn Fix — Sol (codex) design plan

**Status:** design plan (Sol/codex, read-only exploration of the repo). Mine-don't-trust; implementing engineer validates + refines. Correctness paramount (unpublished first-ever-value territory).
**Problem:** base-60 "discovery churn" — the descending-k subset enumerator churns ~C(59,10)≈6×10¹⁰ ten-rule-INFEASIBLE subsets before reaching the first feasible family (drop all 11 multiples of 5 → |D|≈48), so a 90-min W=21 run scanned ZERO subsets. Engine limitation (enumeration ORDER), not intrinsic hardness.
**Date:** 2026-07-23

---

### 1. Feasibility predicate (ten-rule)
D ⊆ {1..B−1} is ten-rule-feasible iff:
- **1a (B-divisibility):** B ∤ lcm(D)  (gcd(B, lcm(D)) ≠ B)
- **1b (prime-power elimination):** for each p^β || B, either no digit in D is divisible by p^β, OR some digit has v_p(d) < β. I.e. drop enough digits that max{v_p(d): d∈D} < β for at least one prime power of B.
- **Digit-sum congruences (order-independent, checked after selection in SB_subset_filters):** for each prime-power q | (B−1), digit-sum mod q must satisfy the arrangement-independent condition.

For b60 = 2²·3·5 (B−1=59 prime): cheapest feasible = drop all multiples of 5 (11 digits) → |D|=48. Descending-k reaches k=49 before this family → the churn.

### 2. Direct enumeration algorithm
Replace `for k=n..1: enumerate all C(n,k)` with: enumerate only the prime-power-elimination families, descending-lex within each.
```
factor B → {(p_i, β_i)}
for each subset droppedPrimes ⊆ {primes dividing B}:
    allowed = {d ∈ 1..B−1 : ∀p∈droppedPrimes, p^{β_p} ∤ d}   // eliminate that prime power
    for k = |allowed| down to 1:
        for each k-subset S of allowed, descending-lex:
            if SB_build_pps(S,k) && SB_subset_filters(S,k):   // reuse existing gates
                onSubset(S,k); return    // first hit is the max
```
Skips the ~6×10¹⁰ infeasible churn; jumps straight to the feasible family.

### 3. Integration (carrytrie.cpp)
- Add `subsetdisc::forEachTenRuleFeasibleSubset(int B, std::function<bool(int k,const int* S)> onSubset)` (namespace subsetdisc ~L3751).
- Replace the raw `for(k=n;k>=1;k--){ nextComb()... }` loop in **runCert (~L3940), runCertFM (~L4114), runCertAuto (~L4276)** with a call to the new iterator; body (SB_S fill, subsetsChecked++, wrong-turn search) unchanged.
- **DO NOT** change SB_build_pps (~L3776, ten-rule) or SB_subset_filters (~L3903, digit-sum) — they stay as the authoritative gate after each emitted subset.
- Preserve EXACT ordering: descending k, then descending-lex over kept digits (S[i]=B−1−comb[i]).

### 4. Complexity
b60: old ≈ 6×10¹⁰ checked (0 scanned in 90min) → new = enumerate ≤ 2^{#primes|B} families, jump to k=48; still needs prefix-sum DP pruning within a family to be fully tractable (codex flags C(48,24) is large — layer the digit-sum-congruence DP to prune branches). Projected 50–100× discovery-phase speedup.

### 5. Validation gates (MUST reproduce exactly)
- a(50) drop{24,25}|D|47, a(51) drop{17,24,34}|D|47, a(52) drop{13,24,26,39}|D|47 → identical value + subsetsScanned=1, wall ±5%.
- DEBUG_SUBSET_ORDER flag: diff old-vs-new subset-emission order → **byte-identical** across all bases ≤52 BEFORE deploying to b60+.
- Edge cases: B−1 prime (few/no forced drops), b2 (single subset), highly-composite B (family count).
- CERTPOS-invariance: first subset independent of CERTPOS.

### 6. Risks (correctness paramount)
1. **ORDERING REGRESSION (highest):** skipping a feasible family or wrong order → silently misses the true max. Mitigate: prove/exhaustively-test order vs old solver on all bases ≤52; don't merge until byte-identical order logs.
2. **Prime-power profile assignment** bug → includes infeasible / excludes feasible. Unit-test profile subroutine on b60/b49.
3. digit-sum modulus depends on lcm(D) per-subset — keep SB_build_pps called AFTER each emitted subset (don't precompute).
4. Nilpotent-suffix / deriveConstantsGen unchanged (low risk).
5. subsetsChecked counter semantics change (now counts generated not raw-churned) — relabel/document, not a regression.

### Implementation order
unit-test enum on b2–b10 → implement forEachTenRuleFeasibleSubset → regression a50/51/52 with DEBUG_SUBSET_ORDER byte-diff → verify b60 drop-5 family enumerated first → dry-run b60 (expect subsetsScanned small, wall ≪90min) → deploy.

---

## Review note (Fable, 2026-07-23, pre-implementation)

§2's pseudocode as written fails §5's own byte-identical-order gate: iterating
`droppedPrimes` families in the OUTER loop emits each family's subsets
contiguously, but the old enumerator's order is GLOBAL descending-k then
descending-lex — families have different max-k and must be interleaved, and a
subset satisfying several families (e.g. one that eliminates both 2² and 5)
would be emitted more than once. Correct shape: a k-major merge — for k
descending, enumerate the union of families' k-subsets in descending-lex with
dedup (or equivalently, enumerate k-subsets of {1..B−1} whose kept-digit
prime-power profile is infeasible-free, using the profile as a pruning mask
inside the combination walk rather than as an outer partition). The validation
gate stands as the arbiter either way.
