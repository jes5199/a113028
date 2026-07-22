Yes. I found one implementable strengthening and one promising proof language—but also a gap in what the conjecture currently claims to accomplish.

1. Combine low-order CRT factors before running the DP

The current “cheap conditions” test each prime power separately. Instead, define

Q_e=\prod_p q_{p,e},

where q_{p,e} is the largest relevant p-power satisfying B^e\equiv1\pmod{q_{p,e}}. Since

Q_e\mid B^e-1,

Q_e<B^e. Thus, for fixed e\le3, the exact class-partition DP modulo the entire Q_e remains polynomial. This captures CRT coupling that is currently being discarded even though it is cheap.

Base 16 is a perfect demonstration. Its order-3 factors are 9,7,13:

test at m=9	admissible targets
Separate DPs mod 9,7,13	3\cdot7\cdot13=273
Joint DP mod 819	253
After the independent mod-11 factor	253\cdot11=2783
Actual permutation image	2783

The joint order-3 condition accounts for every one of the 220 residues that looked mysteriously absent. In my exhaustive scan of bases 7–18, with A=\{1,\ldots,m\} or one deletion from \{1,\ldots,m+1\}, the grouped conditions became sufficient by m^*+2, and usually by m^*+1. Under the current separate conditions, base 16 appeared to require +3.

For base 49 the natural joint moduli are

Q_2=32\cdot25=800,\qquad
Q_3=9\cdot19\cdot43=7353.

At m=20, their DP state spaces are only about 11\cdot800 and 8\cdot8\cdot7353=470{,}592, respectively—comfortably inside Engine C’s existing two-million-state cap. The known dead “choose 21” branch still passes both joint tests at its root, so this is not magic, but it should prune descendants strictly more strongly.

I would redefine the cheap conditions in terms of these cyclotomic layers, not individual prime powers.

2. Use the distinct-coordinate sieve, not just a permanent bound

For an additive character \chi, the Fourier coefficient has the exact expansion

\widehat\mu(\chi)=\frac1{m!}
\sum_{\tau\in S_m}(-1)^{m-c(\tau)}
\prod_{C\in\operatorname{cycles}(\tau)}
S_A\!\left(\sum_{i\in C}B^i\right),

where

S_A(u)=\sum_{d\in A}\chi(ud).

This is the weighted Li–Wan distinct-coordinate sieve. For A=\{1,\ldots,M\}\setminus R,

S_A(u)=
\frac{\chi(u)\bigl(1-\chi(Mu)\bigr)}{1-\chi(u)}
-\sum_{r\in R}\chi(ru),

so it is an explicit geometric sum plus O(C_0) holes. Large Fourier contributions can therefore occur only when many cycles satisfy the resonance

\sum_{i\in C}B^i\equiv0\pmod q.

That turns the analytic problem into a concrete combinatorial lemma about zero-sum subsets of a geometric orbit. Low-order cyclotomic layers are exactly where such resonant cycles proliferate—and those are precisely the layers handled by the joint DPs above.

This route is closely aligned with Li and Yu’s distinct-coordinate formulas⁠￼ and much closer to your statistic than a generic circle-method permanent estimate. Nagy’s Permutations over cyclic groups⁠￼ should also be a primary literature pointer; its “braid trick” explicitly studies how transpositions alter permutational sums. The recent Littlewood–Offord theory on the symmetric group⁠￼ studies exactly \sum w_i v_{\pi(i)}, though its polynomial-scale anti-concentration is not yet fine enough for the 1/L_{\mathrm{eff}} regime.

3. The coverage theorem does not dissolve the computational frontier

This part of OPEN-PROBLEM.md⁠￼ needs revision.

At base 49, m^*=21. The expensive fact is that choosing digit 21 leaves an infeasible 20-digit child. A theorem guaranteeing feasibility for every admissible instance with m\ge21+c says nothing about certifying that 20-digit child as impossible. That certification is exactly the enormous Engine C sweep.

So the coverage conjecture proves the descending prefix is safe, but ALGORITHMS.md⁠￼ is right: the top descent is already essentially free, while critical-band refutation remains expensive.

The algorithmically sufficient conjecture would instead be something like:

Every infeasible prefix-induced node has only B^{O(c)}\operatorname{poly}(B) descendants that continue to satisfy the joint cyclotomic-layer conditions.

That is a bounded refutation-volume conjecture. It directly implies the claimed running time; the present coverage conjecture does not.

Two smaller formal fixes:

* At the frontier, the remaining interval has M\asymp m\asymp B/\log B, while \log L_{\mathrm{eff}}\asymp B, so the statement \log L_{\mathrm{eff}}\approx M overloads M and describes the wrong asymptotic family.
* If T_m is the set surviving the cheap conditions, the counting main term is m!/|T_m|, not m!/L_{\mathrm{eff}}, until T_m fills the entire effective quotient.

My recommended next move is a small PR containing the joint Q_2/Q_3 DPs, a full-support scanner, and a rewrite separating the coverage conjecture from the computational refutation conjecture.