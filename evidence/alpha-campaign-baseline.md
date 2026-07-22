# Carry-trie α campaign — baseline (weakest pruning), 2026-07-22

Candidate-21 (known-NO) branch, base 49, C++ port (carrytrie.cpp),
nice -19, serial, this box. Soundness gate passed beforehand
(candidate-20: exactly 1 survivor = known completion, direct-verified).

| split | |X| | |Y| | trie nodes | visits/x | total visits | wall | RSS | survivors |
|-------|-----|-----|-----------|----------|--------------|------|-----|-----------|
| 3+4 | 5814 | 93024 | 1,093,819 | 12,511.6 | 72,742,203 | 12.2s | 135MB | 0 |
| 2+5 | 342 | 1,395,360 | 15,300,916 | 85,391.9 | 29,204,016 | 15.0s | 1.23GB | 0 |

1+6 skipped: projected 16GB trie > 8GB ceiling (80.7 B/node marginal,
~199M projected nodes).

**Fitted α = 0.7092** (visits/x = 3.745·|Y|^0.7092).
b53 projection (|X|≈|Y|≈3e6, Π≈2.7e12): total ≈ 4.4e11 ≈ **6.1× under Π**.
Verdict per §5 pre-registration: in-between (≥30× GO not met at zero
upgrades; not a NO-GO — §4 pruning rungs unimplemented). Baseline
established for per-upgrade attribution.
