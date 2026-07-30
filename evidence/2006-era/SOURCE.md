# 2006-era artifacts: provenance and verification

Vendored verbatim 2026-07-30 (wind-down landing of the 2026-07-26
provenance investigation; staged copies came from jes via boss-clod,
`/home/jes/a113028_evidence_staging/`). **Do not normalise any of
these files** — the run log's mid-run PST→GMT-8:00 timezone change,
ragged spacing and lowercase asides are authenticity marks.

| file | what it is | verification |
|---|---|---|
| `2006-02-20-run-log.txt` | Timestamped run log, one evening (13:37–21:13), bases 2–36, **Ruby solver** (the log's "YARV interpreter" line) | 35/35 terms parsed and matched against `b113028.txt`, three independent derivations; line-number == base on every line (internal invariant) |
| `2006-04-values-email.txt` | Values quoted in an April 2006 email about then-unsolved base 48; notation key included. **Producing program unknown — do not guess.** Lower bound on what was known, not exhaustive | 8/8 decode == stated decimal == canonical, verified 3× |
| `2006-04-finite.c` | The **April 2006 C rewrite** (GMP, own unit tests). Contains `// ten rule` (L147), `// nine rule` (L151), `five = base/2` (L481). NOT the program that made the run log | Compiles today (`g++ -w -fpermissive -lgmpxx -lgmp`); run verbatim it reproduces a(2)..a(42) char-exact except **b6, where it genuinely fails** (picks unpermutable {5,4,1}, no fallback — the February Ruby solved b6, so the rewrite regressed) |
| `2024-04-oeis-python.txt` | jes's 2024 OEIS-linked Python ("even faster python program") | Read-verified 2026-07-26: has the subset fallback April-2006 lacked; Nine Rule conditional-weakened; "Three Rule" subsumed by 2006's gcd form; naive `n -= lcm` scan; its enumeration order is the modern OLD-mode ancestor |
| `make_subset_2006_port.py` | Self-gating Python port of the April C's *selection* (only). Run from repo root: `python3 evidence/2006-era/make_subset_2006_port.py b113028.txt` | Gate: reproduces the 2006 log's own b36 set before any claim. Result: **29/29 — the 2006 selection picks the true answer digit set at every solved base 36–64** |

Analysis record and the measured "where does 2006 stop" bound:
[../../docs/results/PROVENANCE-EVIDENCE.md](../../docs/results/PROVENANCE-EVIDENCE.md).
