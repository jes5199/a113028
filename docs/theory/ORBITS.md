# The orbit: where the nine and eleven rules come from, and why only three rules have names

*(Written 2026-07-26 at jes's request, for the reader who knew the nine
and eleven rules in 2006 and is meeting the general case. Read once,
keep.)*

## The object

Fix the base B and a modulus q. Write down the powers of B mod q:

```
B⁰, B¹, B², B³, …   (mod q)
```

That sequence is **the orbit**. It is the complete answer to the question
"how much does the digit in position i matter, mod q?" — because any
number N = Σ dᵢ·Bⁱ satisfies

```
N ≡ Σ dᵢ · (Bⁱ mod q)    (mod q)
```

The orbit is the list of per-position weights. Everything else in this
file is just looking at its shape.

**The part that makes it click: the orbit is a fact about B and q only.**
It does not depend on the digits, the number, or the answer. You can
compute every orbit you will ever need before touching the search — and
the engine does.

## The three shapes you can do in your head

The classical divisibility rules are not different rules. They are the
three orbit shapes simple enough to use without paper:

**1. All ones.** If q | B−1 then B ≡ 1, so the orbit is `1, 1, 1, …`.
Every position weighs the same, so N ≡ (digit sum) mod q. In base 10,
q = 9 or 3: **the nine rule.** Order 1.

**2. Alternating.** If q | B+1 then B ≡ −1, so the orbit is
`1, −1, 1, −1, …`. N ≡ (alternating digit sum) mod q. In base 10,
q = 11: **the eleven rule.** Order 2.

**3. Hits zero and stays.** If q shares a prime with B, some power of B
is ≡ 0 mod q, and once the orbit hits 0 it stays there — every higher
position weighs nothing. Only the bottom T digits matter, where T is the
step at which 0 arrives. In base 10, q = 4: orbit `1, 2, 0, 0, …` —
**"check the last two digits."** For q = 5: orbit `1, 0, 0, …` — **"look
at the last digit"** — which is exactly what the variable named `five`
in the April 2006 solver knew. The nilpotent shape.

Every other orbit is longer, has no name, and admits no mental trick.
That is the whole reason school arithmetic teaches exactly these three
and stops. (The folklore seven rule — "double the last digit, subtract"
— is what a length-6 orbit costs: base 10 mod 7 runs `1, 3, 2, 6, 4, 5`
before repeating, so the "rule" is a multi-step procedure nobody quite
remembers. The orbit got too long for a name.)

## What a computer does instead

A computer never needed the names. It computes the actual orbit —
T+period, tail and cycle, whatever shape it has — and then carries a
residue across positions: at each position, add dᵢ times the precomputed
weight, reduce mod q. The three named rules fall out as special cases
that nobody has to special-case. This is the positional-residue
treatment in the engine (the per-prime-power DPs of
[../engine/METHOD.md](../engine/METHOD.md)); there is no eleven rule
anywhere in `carrytrie.cpp` because order 2 is not special there —
orders 3, 4, 5, 6 are handled by the same machinery and have never had
names to lose.

A composite q mixes shapes: split q into its part sharing primes with B
(nilpotent — shape 3) and the rest (a unit — cyclic shape), and the two
parts can be handled independently. Taken across all prime powers of the
lcm at once, that split is
[NILPOTENT-PEELING.md](NILPOTENT-PEELING.md).

## Base 6, the unpermutable set — and the gap between the two named rules

The April 2006 solver fails at exactly one base, and the reason lives in
the shape its two named rules didn't cover.

Its selection picks {5, 4, 1} in base 6 (the first descending set
passing the ten and nine rules; lcm = 20). Divisibility by 20 needs
divisibility by 4 — and for q = 4, B = 6 the orbit is:

```
6⁰, 6¹, 6², …  ≡  1, 2, 0, 0, …   (mod 4)
```

Third shape: only the bottom two digits matter, N ≡ d₀ + 2d₁ (mod 4).
Run the six arrangements of {5, 4, 1}:

| arrangement | value | mod 4 |
|---|---:|---:|
| 541 | 205 | 1 |
| 514 | 190 | 2 |
| 451 | 175 | 3 |
| 415 | 155 | 3 |
| 154 | 70 | 2 |
| 145 | 65 | 1 |

Never 0. **The set is unpermutable** — no arrangement works, not because
any digit is wrong, but because the mod-4 weights (1 and 2, then
nothing) can't reach 0 from these digits. Note 6 ≡ 2 (mod 4): neither
+1 nor −1. The obstruction sits precisely in the gap between the nine
rule and the eleven rule — the right frontier, named in 2006, crossed
here.

(The true a(6) = 412₆ = 152 uses the *next* set, {4, 2, 1}, where the
same orbit is satisfiable: 4·36 + 1·6 + 2 has d₀ + 2d₁ = 2 + 2 ≡ 0.)

## The first base where the names run out

Base 7 — immediately after the alphabet gets interesting. The answer's
digit set is {1, 2, 4, 5, 6}, lcm 60, and 60's prime powers split
across all three vocabularies plus the one that has none:

- q = 3: 7 ≡ 1 → digit sum (nine rule)
- q = 4: 7 ≡ −1 → alternating sum (eleven rule)
- **q = 5: 7 ≡ 2 → orbit `1, 2, 4, 3`, period 4 — no name, no trick**

And the nameless one *discriminates*: of all 120 arrangements, the
largest satisfying the two named rules (N ≡ 0 mod 12) is
`65421₇ = 16332` — which is not divisible by 5. The true
a(7) = `65142₇ = 16200`. **The 2006 vocabulary, applied at base 7,
points at the wrong arrangement.** (Verified by enumeration; every base
from 7 through 64 has at least one nameless orbit over its answer's
digit set, and bases 2–6 have none — it starts at 7 and never reverts.)

Two qualifications, so the example doesn't overclaim:

1. **Necessary as a *rule*, not as a computation.** At base 7 you can
   brute-force 120 permutations without ever thinking about orbits.
   Base 7 is where the classical vocabulary stops being able to
   *express* the constraint — not where the problem gets hard.
2. **Base 6 is earlier, and is a different milestone.** Its obstruction
   (q = 4, orbit `1, 2, 0, 0, …`) is the *degenerate* shape — the third
   *named* one. So **6 is where the two congruence rules run out, and 7
   is where the naming runs out.** Distinct events; collapsing them
   would be wrong.

## The same shape, twenty years apart

The orbit `1, 0, 0, …` — weight on the units digit, nothing anywhere
else — appears three times in this project's history:

- **base 10, q = 5:** every multiple of 5 ends in 0 or 5; with zero
  excluded, a valid answer must end in 5. The 2006 solver reserved that
  digit for the last slot and called it `five`.
- **base 64, q = 32:** v₂(lcm(1..63)) = 5 and 64 ≡ 0 (mod 32), so the
  orbit is `1, 0, 0, …` — N ≡ d₀ (mod 32), and 32 is the only digit
  divisible by 32. **Every full-alphabet completion ends in 32**, which
  discharges ~3.29×10¹¹ search regions on paper and closes the project
  at its capstone ([../results/A64-MAXIMALITY.md](../results/A64-MAXIMALITY.md)).
- everywhere in between, generalized: the engine's `SB_D1` gate is the
  product of the prime powers whose orbit is exactly `1, 0, 0, …`, and
  any digit set containing no digit divisible by it is rejected before
  any search.

Same orbit shape. In 2006 it was a variable; in 2026 it was a theorem.
The distance between those two sentences is the distance the orbit idea
covers: the named rules are the orbits short enough to see, and
computing the orbit is how you stop needing them to be short.
