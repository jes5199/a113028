# sol-reference: Sol's reference implementations — MINE, DON'T TRUST

Reference code contributed by Sol (external), kept for the record:

- `a113028_v11_sol.c` — reference build of the v11 engine. ⚠️ **KNOWN
  DEFECT: it DROPS the mandatory gcd guard. Never adopt as-is.**
  (Recorded when it arrived; kept because it is part of the review
  trail, not because it is usable.)
- `nilpeel_b40.cpp`, `nilpeel_b48.cpp`, `nilpeel_b49.cpp` — the
  bespoke fixed-prefix nilpotent-peeling certificate verifiers behind
  the a(40)/a(48) divergence-flag clearances and the a(49)
  certification (see docs/theory/NILPOTENT-PEELING.md and RESULTS.md).
  These are proof-bearing.
