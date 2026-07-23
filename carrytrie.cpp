// carrytrie.cpp — carry-trie MITM join experiment for base 49 (CARRY-TRIE-JOIN.md §1/§6).
//
// Self-contained C++20 program. All arithmetic constants (L, L_c, T_target,
// c_x, u_y, ...) are DERIVED FROM FIRST PRINCIPLES at runtime; no target
// constant is copied from the docs. Guard rails from CARRY-TRIE-JOIN.md §5
// and TASK-MITM-FP.md are asserted at startup and before every run.
//
// Build:
//   g++ -O3 -march=native -std=c++20 carrytrie.cpp -o carrytrie
//
// Usage:
//   ./carrytrie smoke                          // candidate-21, 3+4 split, first 50 x's
//   ./carrytrie gate                            // candidate-20, 3+4 split, FULL soundness gate
//   ./carrytrie full <cand:20|21> <NX> <NY>     // full measurement run (HOLD-gated for cand=21)
//
// nice -n 19 is expected to be applied by the caller.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
#include <string>
#include <numeric>
#include <functional>
#include <sys/stat.h>

using u64 = uint64_t;
using u128 = __uint128_t;
using i128 = __int128;

static const int B = 49;

// fwd decls (defined later, near runPatFull): Phase-C memory-discipline guards.
static long peakRssKB();
static void checkRssBudget(long maxRssKB, const char *where);

// ---------- u128 printing ----------
static std::string u128_to_string(u128 v) {
    if (v == 0) return "0";
    std::string s;
    while (v > 0) {
        s.push_back(char('0' + int(v % 10)));
        v /= 10;
    }
    std::reverse(s.begin(), s.end());
    return s;
}

// ---------- gcd / lcm on u128 ----------
static u128 gcd_u128(u128 a, u128 b) {
    while (b) { u128 t = a % b; a = b; b = t; }
    return a;
}
static u128 lcm_u128(u128 a, u128 b) {
    u128 g = gcd_u128(a, b);
    return (a / g) * b;
}

// ---------- extended gcd on i128, for modular inverse ----------
// returns g = gcd(a,b), and x,y with a*x + b*y = g
static i128 egcd_i128(i128 a, i128 b, i128 &x, i128 &y) {
    if (b == 0) { x = 1; y = 0; return a; }
    i128 x1, y1;
    i128 g = egcd_i128(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return g;
}

static u128 modinv_u128(u128 a, u128 m) {
    i128 x, y;
    i128 g = egcd_i128((i128)(a % m), (i128)m, x, y);
    if (g != 1 && g != -1) {
        fprintf(stderr, "FATAL: modinv: gcd(a,m) != 1 (g=%s)\n", u128_to_string((u128)(g<0?-g:g)).c_str());
        exit(1);
    }
    i128 mi = (i128)m;
    x %= mi;
    if (x < 0) x += mi;
    return (u128)x;
}

static u128 mulmod_u128(u128 a, u128 b, u128 m) {
    // a,b < m <= ~6.3e19 < 2^66; product fits in u128 (up to ~2^132 worst-case
    // for m near 2^64, but our m ~ 2^66 so a*b ~< 2^132... u128 max is 2^128-1)
    // m here is L_c ~ 6.3e19 ~ 2^65.8, so a,b < 2^66, product < 2^132 which
    // OVERFLOWS u128 (max 2^128). Must use a safe mulmod.
    // Use 128-bit safe approach via __int128 with binary splitting is complex;
    // instead use the fact that m < 2^66 and do a mod-multiply via
    // decomposition: a*b mod m using unsigned __int128 with a manual
    // "multiply and reduce" that avoids overflow by splitting b into hi/lo
    // 32-bit-safe chunks relative to m. Simpler: use double-width via
    // compiler __int128 is already 128 bits and our numbers need up to
    // ~132 bits worst case -> use a schoolbook mulmod with 4x64-bit limbs
    // is overkill; instead reduce a modulo m first (a<m already) and use
    // binary multiplication (Russian peasant) with mod at each doubling,
    // which only ever needs values < m (<2^66), so intermediate sums stay
    // well within u128 (< 2^128).
    u128 result = 0;
    a %= m;
    while (b > 0) {
        if (b & 1) {
            result += a;
            if (result >= m) result -= m;
        }
        a += a;
        if (a >= m) a -= m;
        b >>= 1;
    }
    return result;
}

static u128 powmod_u128(u128 base, u128 exp, u128 m) {
    u128 result = 1 % m;
    base %= m;
    while (exp > 0) {
        if (exp & 1) result = mulmod_u128(result, base, m);
        base = mulmod_u128(base, base, m);
        exp >>= 1;
    }
    return result;
}

// ================= Global derived constants =================
struct Constants {
    std::vector<int> D;              // full digit set {1..48}\{24}
    u128 L;
    u128 L_nil;
    u128 L_c;
    int P;                            // leaf horizon = 12
    int lifts;                        // ceil(B^P / L_c)
    u128 Bp;                          // B^P
    std::vector<int> prefix;          // fixed prefix digits, MSB..LSB order (46 downto 21)
};

static Constants deriveConstants() {
    Constants c;
    for (int d = 1; d <= 48; d++) if (d != 24) c.D.push_back(d);

    // L = lcm(D)
    u128 L = 1;
    for (int d : c.D) L = lcm_u128(L, (u128)d);
    c.L = L;

    // 442720643463713815200 > 2^64 (~1.8e19), so a plain integer literal would
    // overflow; build the expected value from a decimal string instead.
    u128 expectedL;
    {
        const char *lit = "442720643463713815200";
        u128 v = 0;
        for (const char *p = lit; *p; p++) v = v * 10 + (u128)(*p - '0');
        expectedL = v;
    }
    if (c.L != expectedL) {
        fprintf(stderr, "FATAL: L mismatch: computed=%s expected=%s\n",
                u128_to_string(c.L).c_str(), u128_to_string(expectedL).c_str());
        exit(1);
    }
    fprintf(stderr, "[guard] L = %s  (assert OK)\n", u128_to_string(c.L).c_str());

    c.L_nil = 7;
    if (c.L % c.L_nil != 0) { fprintf(stderr, "FATAL: L_nil does not divide L\n"); exit(1); }
    c.L_c = c.L / c.L_nil;
    fprintf(stderr, "[guard] L_c = %s\n", u128_to_string(c.L_c).c_str());

    // guard: gcd(L_c mod 49, 49) == 1
    u64 Lc_mod_B = (u64)(c.L_c % (u128)B);
    u64 g = std::gcd((u64)Lc_mod_B, (u64)B);
    if (g != 1) { fprintf(stderr, "FATAL: gcd(L_c mod B, B) = %llu != 1\n", (unsigned long long)g); exit(1); }
    fprintf(stderr, "[guard] gcd(L_c mod %d, %d) = 1  (assert OK)\n", B, B);

    c.P = 12;
    u128 Bp = 1;
    for (int i = 0; i < c.P; i++) Bp *= (u128)B;
    c.Bp = Bp;
    // lifts = ceil(B^P / L_c)
    u128 lifts128 = (Bp + c.L_c - 1) / c.L_c;
    c.lifts = (int)lifts128;
    if (c.lifts < 3 || c.lifts > 5) {
        fprintf(stderr, "FATAL: lifts=%d not within tolerance of 4 (expected ~4 +/-1)\n", c.lifts);
        exit(1);
    }
    fprintf(stderr, "[guard] lifts = ceil(B^%d / L_c) = %d  (assert OK, target ~4)\n", c.P, c.lifts);

    // fixed prefix digits, MSB-first: 48,47,...,25, then 23,22 (24 excluded, not in D)
    for (int d = 48; d >= 22; d--) if (d != 24) c.prefix.push_back(d);
    if ((int)c.prefix.size() != 26) {
        fprintf(stderr, "FATAL: prefix size = %zu, expected 26\n", c.prefix.size());
        exit(1);
    }
    fprintf(stderr, "[guard] fixed prefix digit count = %zu (24 + 2 = 26, assert OK)\n", c.prefix.size());

    // digit-count assertion: 26 fixed + 1 candidate + 19 free + 1 suffix = 47
    int total = 26 + 1 + 19 + 1;
    if (total != 47) { fprintf(stderr, "FATAL: digit count assertion failed: %d != 47\n", total); exit(1); }
    fprintf(stderr, "[guard] digit count: 26 fixed + 1 candidate + 19 free + 1 suffix = 47  (assert OK)\n");

    return c;
}

// ================= Branch setup =================
struct Branch {
    int candidate;          // 21 or 20
    std::vector<int> A19;   // free digit pool (19 digits), sorted ascending
    u128 T_target;          // required M (mod L_c)

    // RUNG 2 support: since leaf ∪ x ∪ y digits exactly partition A19 (proved in
    // TASK-MITM-FP.md §1, "moments are additive across halves"), sum(y) =
    // S_A19 - sum(x) - sum(leaf), and likewise for sum-of-squares. S_A19/S2_A19
    // are the fixed totals; ascAsc/descPrefix give, for any remaining leaf-slot
    // count R, a valid (loose but sound) [min,max] envelope on what R additional
    // *distinct* digits drawn from A19 could still sum/sum-of-squares to —
    // computed ignoring which specific digits are still available, which is a
    // safe over-approximation (excluding digits can only shrink the true
    // achievable range, never grow it).
    int32_t S_A19 = 0;
    int64_t S2_A19 = 0;
    std::vector<int32_t> prefixP1Asc, prefixP1Desc;   // prefixP1Asc[k] = sum of k smallest A19 digits
    std::vector<int64_t> prefixP2Asc, prefixP2Desc;   // prefixP2Asc[k] = sum of squares of k smallest
};

static void computeMomentBounds(Branch &br) {
    std::vector<int> asc = br.A19;
    std::sort(asc.begin(), asc.end());
    std::vector<int> desc = asc;
    std::reverse(desc.begin(), desc.end());
    int n = (int)asc.size();
    br.prefixP1Asc.assign(n + 1, 0);
    br.prefixP1Desc.assign(n + 1, 0);
    br.prefixP2Asc.assign(n + 1, 0);
    br.prefixP2Desc.assign(n + 1, 0);
    for (int k = 0; k < n; k++) {
        br.prefixP1Asc[k + 1]  = br.prefixP1Asc[k]  + asc[k];
        br.prefixP1Desc[k + 1] = br.prefixP1Desc[k] + desc[k];
        br.prefixP2Asc[k + 1]  = br.prefixP2Asc[k]  + (int64_t)asc[k] * asc[k];
        br.prefixP2Desc[k + 1] = br.prefixP2Desc[k] + (int64_t)desc[k] * desc[k];
    }
    br.S_A19 = br.prefixP1Asc[n];
    br.S2_A19 = br.prefixP2Asc[n];
}

static Branch buildBranch(const Constants &c, int candidate) {
    Branch br;
    br.candidate = candidate;

    // C = (sum prefix_digit * B^pos + candidate * B^20 + 7 * B^0) mod L_c
    u128 Lc = c.L_c;
    u128 C = 0;
    int pos = 46;
    for (int dd : c.prefix) {
        u128 term = mulmod_u128((u128)dd, powmod_u128((u128)B, (u128)pos, Lc), Lc);
        C = (C + term) % Lc;
        pos--;
    }
    C = (C + mulmod_u128((u128)candidate, powmod_u128((u128)B, 20, Lc), Lc)) % Lc;
    C = (C + 7) % Lc;

    u128 Binv = modinv_u128((u128)B, Lc);
    // T_target = (-C) * Binv mod Lc
    u128 negC = (Lc - (C % Lc)) % Lc;
    br.T_target = mulmod_u128(negC, Binv, Lc);

    // A19 = D \ prefix \ {candidate} \ {7}
    std::vector<bool> used(49, false);
    for (int d : c.prefix) used[d] = true;
    used[candidate] = true;
    used[7] = true;
    for (int d : c.D) if (!used[d]) br.A19.push_back(d);

    if (br.A19.size() != 19) {
        fprintf(stderr, "FATAL: A19 size = %zu, expected 19 (candidate=%d)\n", br.A19.size(), candidate);
        exit(1);
    }
    computeMomentBounds(br);
    return br;
}

// ================= Permutation generator =================
// Generates all ordered k-permutations of the given pool (in the given order,
// not necessarily sorted) via Heap-ish recursive selection; calls cb(vec) for each.
template <typename CB>
static void forEachPermutation(const std::vector<int> &pool, int k, CB cb) {
    int n = (int)pool.size();
    std::vector<int> chosen(k);
    std::vector<bool> used(n, false);
    std::function<void(int)> rec = [&](int depth) {
        if (depth == k) { cb(chosen); return; }
        for (int i = 0; i < n; i++) {
            if (used[i]) continue;
            used[i] = true;
            chosen[depth] = pool[i];
            rec(depth + 1);
            used[i] = false;
        }
    };
    rec(0);
}

// ================= Trie =================
// Pruning ladder (CARRY-TRIE-JOIN.md §4):
//   0 = no pruning beyond the base walk (digit-membership + distinctness)
//   1 = + rung 1: subtree mask-union bitsets (andMask/orMask over descendant y-masks)
//   2 = + rung 2a: p1 (first-moment / digit-sum) subtree budgets
//   3 = + rung 2b: p2 (second-moment / digit-square-sum) subtree budgets
static int PRUNE_LEVEL = 3; // set from CLI; see main()

struct TrieNode {
    // children: (edge digit 0..48, child node index)
    std::vector<std::pair<uint8_t,int32_t>> children;
    // present only at depth==DEPTH nodes: ordered y-digit-lists (positions 12..12+NY-1,
    // i.e. index0 = relative position 12) that hash to this trie leaf.
    std::vector<std::vector<int>> terminalY;

    // RUNG 1 (§4.1): OR/AND of every descendant terminal's y-digit-mask.
    //   andMask: bits set in EVERY descendant y-mask (empty subtree -> all-ones,
    //            i.e. no constraint, via the identity element of AND).
    //   orMask:  bits set in AT LEAST ONE descendant y-mask.
    uint64_t andMask = ~0ULL;
    uint64_t orMask  = 0;

    // RUNG 2 (§4.2): min/max of p1 (=sum of digits) and p2 (=sum of squares of
    // digits) of the y-digit-set, taken over every descendant terminal.
    int32_t minP1 = INT32_MAX, maxP1 = INT32_MIN;
    int64_t minP2 = INT64_MAX, maxP2 = INT64_MIN;
};

static const int DEPTH = 14;

struct Trie {
    std::vector<TrieNode> nodes;
    Trie() { nodes.emplace_back(); } // root = 0

    int32_t childOrCreate(int32_t node, uint8_t dig) {
        for (auto &pr : nodes[node].children) if (pr.first == dig) return pr.second;
        int32_t idx = (int32_t)nodes.size();
        nodes.emplace_back();
        nodes[node].children.push_back({dig, idx});
        return idx;
    }

    void insert(const std::array<uint8_t, DEPTH> &digitsLSD, const std::vector<int> &ydigitsOrdered) {
        int32_t cur = 0;
        for (int d = 0; d < DEPTH; d++) cur = childOrCreate(cur, digitsLSD[d]);
        nodes[cur].terminalY.push_back(ydigitsOrdered);
    }

    // Bottom-up (post-order) aggregation of the rung-1/rung-2 subtree summaries.
    // Recursion depth is bounded by DEPTH=14, so this is safe even with tens of
    // millions of nodes (breadth, not depth, is what's large).
    void computeAggregates(int32_t idx) {
        TrieNode &node = nodes[idx];
        for (auto &pr : node.children) {
            computeAggregates(pr.second);
            TrieNode &child = nodes[pr.second];
            node.andMask &= child.andMask;
            node.orMask  |= child.orMask;
            node.minP1 = std::min(node.minP1, child.minP1);
            node.maxP1 = std::max(node.maxP1, child.maxP1);
            node.minP2 = std::min(node.minP2, child.minP2);
            node.maxP2 = std::max(node.maxP2, child.maxP2);
        }
        for (auto &ydigits : node.terminalY) {
            uint64_t m = 0;
            int32_t p1 = 0;
            int64_t p2 = 0;
            for (int d : ydigits) { m |= ((uint64_t)1 << d); p1 += d; p2 += (int64_t)d * d; }
            node.andMask &= m;
            node.orMask  |= m;
            node.minP1 = std::min(node.minP1, p1);
            node.maxP1 = std::max(node.maxP1, p1);
            node.minP2 = std::min(node.minP2, p2);
            node.maxP2 = std::max(node.maxP2, p2);
        }
    }
};

static inline uint64_t maskOf(const std::vector<int> &digits) {
    uint64_t m = 0;
    for (int d : digits) m |= ((uint64_t)1 << d);
    return m;
}

static inline uint64_t bit(int d) { return (uint64_t)1 << d; }

// Sum of the k smallest / k largest set-bit positions (=digit values) in mask.
// mask has at most 19 bits set (digits from A19), so k <= 12 always terminates
// in <= k <= 12 iterations. Used for the per-path rung-2' remaining-leaf-digit
// sum envelope (see walkOne).
static inline int32_t sumSmallestK(uint64_t mask, int k) {
    int32_t s = 0;
    for (int i = 0; i < k && mask; i++) {
        int p = __builtin_ctzll(mask);
        s += p;
        mask &= mask - 1;
    }
    return s;
}
static inline int32_t sumLargestK(uint64_t mask, int k) {
    int32_t s = 0;
    for (int i = 0; i < k && mask; i++) {
        int p = 63 - __builtin_clzll(mask);
        s += p;
        mask &= ~((uint64_t)1 << p);
    }
    return s;
}

// digits of a u128 value n, LSD-first, exactly ndig digits base B
static std::array<uint8_t, DEPTH> digitsLSDof(u128 n) {
    std::array<uint8_t, DEPTH> out{};
    for (int i = 0; i < DEPTH; i++) {
        out[i] = (uint8_t)(n % (u128)B);
        n /= (u128)B;
    }
    return out;
}

// ================= Walk stats =================
struct WalkStats {
    u64 visits = 0;
    u64 survivors = 0;
};

struct StackFrame {
    int32_t node;
    int depth;
    int borrow;
    uint64_t used;
    int32_t sumP1;   // running sum of leaf digits chosen so far (depth<=12)
    int64_t sumP2;   // running sum of squares of leaf digits chosen so far
};

// Walk the trie for one (x, lift j) pair. allowedMinusX = A19mask & ~xmask.
// xP1/xP2 are the (fixed, per-x) sum and sum-of-squares of the x digits.
static void walkOne(const Trie &trie, u128 W, uint64_t allowedMinusX, uint64_t xmask,
                     uint64_t A19mask, WalkStats &stats,
                     const Branch &br, int32_t xP1, int64_t xP2) {
    auto Wdig = digitsLSDof(W);
    std::vector<StackFrame> stack;
    stack.reserve(64);
    stack.push_back({0, 0, 0, 0, 0, 0});
    while (!stack.empty()) {
        StackFrame f = stack.back();
        stack.pop_back();
        stats.visits++;
        const TrieNode &node = trie.nodes[f.node];

        // ---- Pruning ladder (sound at every rung; see CARRY-TRIE-JOIN.md §4) ----
        if (PRUNE_LEVEL >= 1) {
            // RUNG 1a: if some digit is present in EVERY descendant y-mask
            // (bit set in andMask) and that digit is already unavailable
            // (claimed by x, or already used as a leaf digit on this path),
            // then EVERY descendant fails the terminal disjointness/leftover
            // check -> the whole subtree is dead. Sound because andMask only
            // has a bit set when literally all descendants share it.
            uint64_t forbidden = xmask | f.used;
            if (node.andMask & forbidden) continue;
        }
        int R = (f.depth <= 12) ? (12 - f.depth) : 0; // remaining leaf slots to be chosen
        if (PRUNE_LEVEL >= 1 && f.depth >= 12) {
            // RUNG 1b: once depth>=12, 'used' is final (leaf digits fixed), so
            // the exact required y-mask R_exact = A19mask & ~xmask & ~used is
            // known. If some digit in R_exact is absent from EVERY descendant
            // (bit missing from orMask), no descendant can equal R_exact ->
            // prune. Sound: orMask is the union over all descendants, so a
            // missing bit there is missing everywhere.
            uint64_t Rexact = A19mask & ~xmask & ~f.used;
            if (Rexact & ~node.orMask) continue;
        }
        // RUNG 2' (per-path availability envelope, replaces the global-A19
        // envelope of the first rung-2 attempt, which measured as a no-op).
        // Exact partition identity: sum(leaf)+sum(y)+sum(x) = S_A19 (additive,
        // TASK-MITM-FP.md §1) => sum(y) = S_A19 - xP1 - sum(leaf). sum(leaf) is
        // f.sumP1 (placed so far) plus whatever the remaining R leaf digits
        // contribute. Those R digits are drawn from avail = A19\x\used(so far)
        // -- the CURRENT live mask, not the static A19 set -- so their sum lies
        // in [sumSmallestK(avail,R), sumLargestK(avail,R)], a per-path bound
        // strictly at least as tight as the old global one (avail ⊆ A19mask).
        // Soundness subtlety (per Lead's note): y's own digits also come out of
        // avail, so the TRUE remaining-leaf pool is avail minus whatever y ends
        // up taking -- smaller than avail. Using the full avail mask (not that
        // smaller true pool) can only WIDEN [min,max], never narrow it past the
        // true range, so the bound stays sound (never rejects a real survivor).
        uint64_t avail = A19mask & ~xmask & ~f.used;
        if (PRUNE_LEVEL >= 2) {
            int32_t loRemain = sumSmallestK(avail, R);
            int32_t hiRemain = sumLargestK(avail, R);
            int32_t loY1 = br.S_A19 - xP1 - f.sumP1 - hiRemain;
            int32_t hiY1 = br.S_A19 - xP1 - f.sumP1 - loRemain;
            if (node.maxP1 < loY1 || node.minP1 > hiY1) continue;
        }
        if (PRUNE_LEVEL >= 3) {
            // p2' analog, same avail-based per-path envelope but over squares.
            int64_t loRemain2 = 0, hiRemain2 = 0;
            {
                uint64_t m1 = avail; for (int i = 0; i < R && m1; i++) { int p = __builtin_ctzll(m1); loRemain2 += (int64_t)p*p; m1 &= m1-1; }
                uint64_t m2 = avail; for (int i = 0; i < R && m2; i++) { int p = 63-__builtin_clzll(m2); hiRemain2 += (int64_t)p*p; m2 &= ~((uint64_t)1<<p); }
            }
            int64_t loY2 = br.S2_A19 - xP2 - f.sumP2 - hiRemain2;
            int64_t hiY2 = br.S2_A19 - xP2 - f.sumP2 - loRemain2;
            if (node.maxP2 < loY2 || node.minP2 > hiY2) continue;
        }
        // ---- end pruning ladder ----

        if (f.depth == DEPTH) {
            if (f.borrow == 0) {
                for (const auto &ydigits : node.terminalY) {
                    uint64_t ymask = maskOf(ydigits);
                    if (ymask & xmask) continue;
                    if (f.used == (A19mask & ~ymask & ~xmask)) {
                        stats.survivors++;
                    }
                }
            }
            continue;
        }
        for (auto &pr : node.children) {
            uint8_t dig = pr.first;
            int32_t child = pr.second;
            int wd = (int)Wdig[f.depth] - (int)dig - f.borrow;
            int nb = 0;
            if (wd < 0) { wd += B; nb = 1; }
            if (f.depth < 12) {
                if (wd == 0) continue;
                uint64_t wdbit = bit(wd);
                if (!(allowedMinusX & wdbit)) continue;
                if (f.used & wdbit) continue;
                stack.push_back({child, f.depth + 1, nb, f.used | wdbit,
                                  f.sumP1 + wd, f.sumP2 + (int64_t)wd * wd});
            } else {
                if (wd != 0) continue;
                stack.push_back({child, f.depth + 1, nb, f.used, f.sumP1, f.sumP2});
            }
        }
    }
}

// ================= Trie build for a given (NX, NY) split =================
static Trie buildTrie(const Branch &br, const Constants &c, int NX, int NY, u64 &yCount) {
    Trie trie;
    u128 Lc = c.L_c;
    u128 B12modLc = powmod_u128((u128)B, 12, Lc);
    yCount = 0;
    forEachPermutation(br.A19, NY, [&](const std::vector<int> &ydigits) {
        // y_val = sum ydigits[i] * B^i  (i=0..NY-1), relative positions 12..12+NY-1
        u128 y_val = 0, Bpow = 1;
        for (int i = 0; i < NY; i++) {
            y_val += (u128)ydigits[i] * Bpow;
            Bpow *= (u128)B;
        }
        u128 u_y = mulmod_u128(B12modLc, y_val % Lc, Lc);
        auto digs = digitsLSDof(u_y);
        trie.insert(digs, ydigits);
        yCount++;
    });
    trie.computeAggregates(0);
    return trie;
}

// ================= Main measurement driver =================
struct RunResult {
    u64 totalVisits = 0;
    u64 totalSurvivors = 0;
    u64 xCount = 0;
    u64 yCount = 0;
    double wallSeconds = 0;
};

static RunResult runMeasurement(const Branch &br, const Constants &c, int NX, int NY,
                                 long xLimit /* -1 = unlimited */, bool verboseEachSurvivor,
                                 std::vector<std::vector<int>> *survivorFreeDigits) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();

    u128 Lc = c.L_c;
    u64 yCount = 0;
    Trie trie = buildTrie(br, c, NX, NY, yCount);
    fprintf(stderr, "[trie] NY=%d built: %llu y-arrangements, %zu nodes\n",
            NY, (unsigned long long)yCount, trie.nodes.size());

    uint64_t A19mask = 0;
    for (int d : br.A19) A19mask |= bit(d);

    u128 Bexp = powmod_u128((u128)B, (u128)(12 + NY), Lc); // B^{12+NY} mod Lc

    RunResult rr;
    rr.yCount = yCount;

    long xCounted = 0;
    forEachPermutation(br.A19, NX, [&](const std::vector<int> &xdigits) {
        if (xLimit >= 0 && xCounted >= xLimit) return;
        xCounted++;

        u128 x_val = 0, Bpow = 1;
        for (int i = 0; i < NX; i++) { x_val += (u128)xdigits[i] * Bpow; Bpow *= (u128)B; }
        u128 term = mulmod_u128(Bexp, x_val % Lc, Lc);
        u128 c_x = (br.T_target + Lc - (term % Lc)) % Lc;

        uint64_t xmask = 0;
        for (int d : xdigits) xmask |= bit(d);
        uint64_t allowedMinusX = A19mask & ~xmask;
        int32_t xP1 = 0; int64_t xP2 = 0;
        for (int d : xdigits) { xP1 += d; xP2 += (int64_t)d * d; }

        WalkStats stats;
        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            walkOne(trie, W, allowedMinusX, xmask, A19mask, stats, br, xP1, xP2);
        }
        rr.totalVisits += stats.visits;
        rr.totalSurvivors += stats.survivors;
    });
    // NOTE: forEachPermutation has no early-exit; xLimit is honored via the
    // xCounted guard above but we still enumerate all permutations internally
    // for simplicity in the smoke test (cheap since NX is small there). For
    // full runs xLimit is -1 (unlimited) so this is moot.
    rr.xCount = xCounted;

    auto t1 = clock::now();
    rr.wallSeconds = std::chrono::duration<double>(t1 - t0).count();
    return rr;
}

// ================= Soundness gate: full verification via direct u128 arithmetic =================
// Recomputes N directly for a full 47-digit assignment and checks N % L == 0,
// independent of the trie machinery. Used to verify every reported survivor.
static bool verifySurvivorDirect(const Constants &c, const Branch &br,
                                  const std::vector<int> &prefix, int candidate,
                                  const std::vector<int> &freeDigitsAbsPos1to19 /* index0=pos1..index18=pos19 */) {
    // Build full digit-by-position map and compute N mod L via Horner in u128
    // reduced mod L at each step (L fits comfortably; use mulmod/L).
    u128 L = c.L;
    // Horner from MSB (pos46) down to LSD (pos0): N = ((...)*B + d)
    std::vector<int> digitsMSBfirst(47);
    int idx = 0;
    for (int dd : prefix) digitsMSBfirst[idx++] = dd;        // positions 46..21
    digitsMSBfirst[idx++] = candidate;                        // position 20
    for (int p = 19; p >= 1; p--) digitsMSBfirst[idx++] = freeDigitsAbsPos1to19[p - 1]; // positions19..1
    digitsMSBfirst[idx++] = 7;                                 // position 0
    if (idx != 47) { fprintf(stderr, "FATAL verify: idx=%d != 47\n", idx); exit(1); }

    u128 acc = 0;
    for (int d : digitsMSBfirst) {
        acc = mulmod_u128(acc, (u128)B, L);
        acc = (acc + (u128)d) % L;
    }
    return acc == 0;
}

// ================= Modes =================
static void printGuardSummary(const Constants &c) {
    fprintf(stderr, "\n=== Guard-rail summary ===\n");
    fprintf(stderr, "B=%d, |D|=%zu, L=%s\n", B, c.D.size(), u128_to_string(c.L).c_str());
    fprintf(stderr, "L_nil=%s, L_c=%s\n", u128_to_string(c.L_nil).c_str(), u128_to_string(c.L_c).c_str());
    fprintf(stderr, "P=%d, lifts=%d\n", c.P, c.lifts);
    fprintf(stderr, "===========================\n\n");
}

static void runSmoke(const Constants &c) {
    Branch br = buildBranch(c, 21);
    fprintf(stderr, "[smoke] candidate=21, split 3+4 (NX=3,NY=4), first 50 x's\n");
    RunResult rr = runMeasurement(br, c, 3, 4, 50, false, nullptr);
    fprintf(stderr, "[smoke] x sampled=%llu, |Y|=%llu, total visits=%llu, visits/x=%.1f, survivors=%llu, wall=%.3fs\n",
            (unsigned long long)rr.xCount, (unsigned long long)rr.yCount,
            (unsigned long long)rr.totalVisits,
            rr.xCount ? (double)rr.totalVisits / rr.xCount : 0.0,
            (unsigned long long)rr.totalSurvivors, rr.wallSeconds);
}

// Known base-49 candidate-20 YES completion, sourced from NILPOTENT-PEELING.md
// ("Base 49 certificate" preamble): prefix 48,47,...,25,23,22,20 then suffix
// 9,12,19,2,3,11,15,10,14,6,16,18,8,1,13,17,21,5,4,7. Verified independently
// above (N mod L == 0) before use here; this is a REFERENCE value used only
// to check that the trie join reproduces a known-true solution, not a value
// baked into the join arithmetic itself.
static const std::vector<int> KNOWN_FREE_POS19_TO_1 = {
    9,12,19,2,3,11,15,10,14,6,16,18,8,1,13,17,21,5,4
};

static void runGate(const Constants &c) {
    Branch br = buildBranch(c, 20);
    fprintf(stderr, "[gate] candidate=20 (known YES), split 3+4 (NX=3,NY=4), FULL run\n");

    // Derive known x/y/leaf split (relative positions) for reporting/matching.
    // KNOWN_FREE_POS19_TO_1 is listed MSB..LSB (index0 = abs pos19); rel[]
    // re-indexes to index0 = abs pos1, which is what verifySurvivorDirect expects.
    std::vector<int> rel(19);
    for (int i = 0; i < 19; i++) rel[i] = KNOWN_FREE_POS19_TO_1[18 - i]; // rel[0]=abs pos1

    // direct-arithmetic check of the known completion, independent of the join
    bool knownOk = verifySurvivorDirect(c, br, c.prefix, 20, rel);
    fprintf(stderr, "[gate] known completion direct verification (N mod L == 0): %s\n", knownOk ? "PASS" : "FAIL");
    if (!knownOk) {
        fprintf(stderr, "[gate] FATAL: known completion does not satisfy N mod L == 0; aborting gate.\n");
        exit(1);
    }

    std::vector<int> knownLeaf(rel.begin(), rel.begin() + 12);
    std::vector<int> knownY(rel.begin() + 12, rel.begin() + 16);
    std::vector<int> knownX(rel.begin() + 16, rel.begin() + 19);
    uint64_t knownYmask = 0; for (int d : knownY) knownYmask |= bit(d);
    uint64_t knownXmask = 0; for (int d : knownX) knownXmask |= bit(d);
    uint64_t knownLeafmask = 0; for (int d : knownLeaf) knownLeafmask |= bit(d);

    // Full run, but capture whether the known (x) permutation + resulting
    // survivor with knownYmask/knownLeafmask occurs.
    u128 Lc = c.L_c;
    u64 yCount = 0;
    Trie trie = buildTrie(br, c, 3, 4, yCount);
    fprintf(stderr, "[gate] trie built: %llu y-arrangements, %zu nodes\n",
            (unsigned long long)yCount, trie.nodes.size());

    uint64_t A19mask = 0; for (int d : br.A19) A19mask |= bit(d);
    u128 Bexp = powmod_u128((u128)B, 16, Lc); // 12+NY=16 for 3+4 split

    u64 totalVisits = 0, totalSurvivors = 0, verifiedOk = 0, verifiedBad = 0;
    bool foundKnown = false;
    u64 xCounted = 0;

    // Decode-capable stack frame: tracks leaf digits in position order (0..11)
    // alongside the (node, depth, borrow, used-mask) state, so every survivor
    // can be reconstructed into a full 47-digit number and checked directly.
    struct DecodeFrame {
        int32_t node;
        int depth;
        int borrow;
        uint64_t used;
        int32_t sumP1;
        int64_t sumP2;
        std::array<int,12> leaf; // leaf[0..depth-1] valid when depth<=12
    };

    forEachPermutation(br.A19, 3, [&](const std::vector<int> &xdigits) {
        xCounted++;
        u128 x_val = 0, Bpow = 1;
        for (int i = 0; i < 3; i++) { x_val += (u128)xdigits[i] * Bpow; Bpow *= (u128)B; }
        u128 term = mulmod_u128(Bexp, x_val % Lc, Lc);
        u128 c_x = (br.T_target + Lc - (term % Lc)) % Lc;

        uint64_t xmask = 0; for (int d : xdigits) xmask |= bit(d);
        uint64_t allowedMinusX = A19mask & ~xmask;
        bool thisIsKnownX = (xdigits[0]==knownX[0] && xdigits[1]==knownX[1] && xdigits[2]==knownX[2]);
        int32_t xP1 = 0; int64_t xP2 = 0;
        for (int d : xdigits) { xP1 += d; xP2 += (int64_t)d * d; }

        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            auto Wdig = digitsLSDof(W);
            std::vector<DecodeFrame> stack;
            stack.push_back({0,0,0,0,0,0,{}});
            while (!stack.empty()) {
                DecodeFrame f = stack.back(); stack.pop_back();
                totalVisits++;
                const TrieNode &node = trie.nodes[f.node];

                // Same pruning ladder as walkOne(); see comments there for the
                // soundness argument of each rung.
                if (PRUNE_LEVEL >= 1) {
                    uint64_t forbidden = xmask | f.used;
                    if (node.andMask & forbidden) continue;
                }
                int R = (f.depth <= 12) ? (12 - f.depth) : 0;
                if (PRUNE_LEVEL >= 1 && f.depth >= 12) {
                    uint64_t Rexact = A19mask & ~xmask & ~f.used;
                    if (Rexact & ~node.orMask) continue;
                }
                uint64_t avail = A19mask & ~xmask & ~f.used;
                if (PRUNE_LEVEL >= 2) {
                    int32_t loRemain = sumSmallestK(avail, R);
                    int32_t hiRemain = sumLargestK(avail, R);
                    int32_t loY1 = br.S_A19 - xP1 - f.sumP1 - hiRemain;
                    int32_t hiY1 = br.S_A19 - xP1 - f.sumP1 - loRemain;
                    if (node.maxP1 < loY1 || node.minP1 > hiY1) continue;
                }
                if (PRUNE_LEVEL >= 3) {
                    int64_t loRemain2 = 0, hiRemain2 = 0;
                    {
                        uint64_t m1 = avail; for (int i = 0; i < R && m1; i++) { int p = __builtin_ctzll(m1); loRemain2 += (int64_t)p*p; m1 &= m1-1; }
                        uint64_t m2 = avail; for (int i = 0; i < R && m2; i++) { int p = 63-__builtin_clzll(m2); hiRemain2 += (int64_t)p*p; m2 &= ~((uint64_t)1<<p); }
                    }
                    int64_t loY2 = br.S2_A19 - xP2 - f.sumP2 - hiRemain2;
                    int64_t hiY2 = br.S2_A19 - xP2 - f.sumP2 - loRemain2;
                    if (node.maxP2 < loY2 || node.minP2 > hiY2) continue;
                }

                if (f.depth == DEPTH) {
                    if (f.borrow == 0) {
                        for (const auto &ydigits : node.terminalY) {
                            uint64_t ymask = maskOf(ydigits);
                            if (ymask & xmask) continue;
                            if (f.used != (A19mask & ~ymask & ~xmask)) continue;
                            totalSurvivors++;

                            // Reconstruct full free-digit vector (positions 1..19) and
                            // verify N mod L == 0 directly, independent of trie logic.
                            std::vector<int> freePos1to19(19);
                            for (int i = 0; i < 12; i++) freePos1to19[i] = f.leaf[i];       // pos1..12
                            for (int i = 0; i < 4; i++) freePos1to19[12+i] = ydigits[i];    // pos13..16
                            for (int i = 0; i < 3; i++) freePos1to19[16+i] = xdigits[i];    // pos17..19
                            bool ok = verifySurvivorDirect(c, br, c.prefix, 20, freePos1to19);
                            if (ok) verifiedOk++; else verifiedBad++;

                            if (ymask == knownYmask && xmask == knownXmask &&
                                f.used == knownLeafmask && thisIsKnownX) {
                                foundKnown = true;
                            }
                        }
                    }
                    continue;
                }
                for (auto &pr : node.children) {
                    uint8_t dig = pr.first; int32_t child = pr.second;
                    int wd = (int)Wdig[f.depth] - (int)dig - f.borrow;
                    int nb = 0;
                    if (wd < 0) { wd += B; nb = 1; }
                    if (f.depth < 12) {
                        if (wd == 0) continue;
                        uint64_t wdbit = bit(wd);
                        if (!(allowedMinusX & wdbit)) continue;
                        if (f.used & wdbit) continue;
                        DecodeFrame nf = f;
                        nf.node = child; nf.depth = f.depth+1; nf.borrow = nb;
                        nf.used = f.used | wdbit;
                        nf.leaf[f.depth] = wd;
                        nf.sumP1 = f.sumP1 + wd;
                        nf.sumP2 = f.sumP2 + (int64_t)wd * wd;
                        stack.push_back(nf);
                    } else {
                        if (wd != 0) continue;
                        DecodeFrame nf = f;
                        nf.node = child; nf.depth = f.depth+1; nf.borrow = nb;
                        stack.push_back(nf);
                    }
                }
            }
        }
    });

    fprintf(stderr, "[gate] x permutations=%llu, |Y|=%llu, total visits=%llu, survivors=%llu\n",
            (unsigned long long)xCounted, (unsigned long long)yCount,
            (unsigned long long)totalVisits, (unsigned long long)totalSurvivors);
    fprintf(stderr, "[gate] direct-arithmetic verification of survivors: %llu OK, %llu FAILED\n",
            (unsigned long long)verifiedOk, (unsigned long long)verifiedBad);
    fprintf(stderr, "[gate] known completion found among survivors: %s\n", foundKnown ? "YES" : "NO");
    bool pass = foundKnown && (verifiedBad == 0) && (totalSurvivors == verifiedOk);
    fprintf(stderr, "[gate] SOUNDNESS GATE: %s\n", pass ? "PASS" : "FAIL");
}

static bool fileExists(const char *path); // fwd decl; defined below, used by runPatFull's HOLD gate

// ================= Patricia (path-compressed) trie — PATRICIA-CARRY-TRIE.md Phase A =================
//
// Flat (radix_key, y_mask) records (§4.1) are generated directly (no ordinary
// pointer trie is ever materialized), sorted lexicographically by their
// 14-digit LSD-first radix key (§4.2), and folded into a compact LCP-based
// trie (§4.3). Compressed edges are walked digit-by-digit with the full
// subtraction/borrow/membership/distinctness logic (§4.4) — compression
// removes node dispatch and stack-frame overhead along unary chains, it does
// not skip any semantic check. The classic pointer-trie path above is left
// completely untouched for A/B comparison.

struct PatRecord {
    std::array<uint8_t, DEPTH> digs;
    uint64_t ymask = 0;
    std::array<uint8_t, 6> ydigits{}; // actual ordered y digits, kept only so the
                                       // soundness gate can reconstruct the exact
                                       // 47-digit assignment for direct u128
                                       // re-verification (the search itself only
                                       // ever needs ymask, per doc §4.1).
    uint8_t ny = 0;
};

struct PatNode {
    uint8_t edgeLen = 0;
    std::array<uint8_t, DEPTH> edge{};
    bool isTerminal = false;
    uint64_t termYmask = 0;
    std::array<uint8_t, 6> termYdigits{};
    uint8_t termNY = 0;
    int32_t childrenStart = -1;
    uint8_t childCount = 0;
    // Same rung-1/rung-2 subtree summaries as TrieNode, aggregated over this
    // compact node's descendant terminals.
    uint64_t andMask = ~0ULL, orMask = 0;
    int32_t minP1 = INT32_MAX, maxP1 = INT32_MIN;
    int64_t minP2 = INT64_MAX, maxP2 = INT64_MIN;
};

struct PatriciaTrie {
    std::vector<PatNode> nodes;
    std::vector<std::pair<uint8_t, int32_t>> childPool;
};

// Recursive LCP-based compact-trie builder over a sorted [lo,hi) range of
// records, starting at depth `depth`. Returns the new node's index. Since
// distinct ordered y-arrangements give distinct u_y (gcd(49,L_c)=1, doc §4.1),
// every record's 14-digit key is globally unique, so hi-lo==1 unambiguously
// means "this range is exactly one terminal" (never a key collision).
static int32_t patBuild(PatriciaTrie &pt, std::vector<PatRecord> &recs, int lo, int hi, int depth) {
    int32_t idx = (int32_t)pt.nodes.size();
    pt.nodes.emplace_back();

    if (hi - lo == 1) {
        PatNode nd;
        nd.edgeLen = (uint8_t)(DEPTH - depth);
        for (int i = 0; i < nd.edgeLen; i++) nd.edge[i] = recs[lo].digs[depth + i];
        nd.isTerminal = true;
        nd.termYmask = recs[lo].ymask;
        nd.termYdigits = recs[lo].ydigits;
        nd.termNY = recs[lo].ny;
        int32_t p1 = 0; int64_t p2 = 0;
        for (int i = 0; i < nd.termNY; i++) { p1 += nd.termYdigits[i]; p2 += (int64_t)nd.termYdigits[i] * nd.termYdigits[i]; }
        nd.andMask = nd.termYmask;
        nd.orMask  = nd.termYmask;
        nd.minP1 = nd.maxP1 = p1;
        nd.minP2 = nd.maxP2 = p2;
        pt.nodes[idx] = nd;
        return idx;
    }

    // Sorted range => LCP of the whole range equals LCP of its first and last
    // element; O(DEPTH) instead of O(range) per node.
    int cp = depth;
    while (cp < DEPTH && recs[lo].digs[cp] == recs[hi - 1].digs[cp]) cp++;

    std::vector<std::pair<uint8_t, int32_t>> kids;
    int i = lo;
    while (i < hi) {
        uint8_t bd = recs[i].digs[cp];
        int j = i + 1;
        while (j < hi && recs[j].digs[cp] == bd) j++;
        int32_t childIdx = patBuild(pt, recs, i, j, cp + 1);
        kids.push_back({bd, childIdx});
        i = j;
    }

    int32_t childrenStart = (int32_t)pt.childPool.size();
    for (auto &kv : kids) pt.childPool.push_back(kv);

    PatNode nd;
    nd.edgeLen = (uint8_t)(cp - depth);
    for (int k = 0; k < nd.edgeLen; k++) nd.edge[k] = recs[lo].digs[depth + k];
    nd.isTerminal = false;
    nd.childrenStart = childrenStart;
    nd.childCount = (uint8_t)kids.size();
    for (auto &kv : kids) {
        const PatNode &ch = pt.nodes[kv.second];
        nd.andMask &= ch.andMask;
        nd.orMask  |= ch.orMask;
        nd.minP1 = std::min(nd.minP1, ch.minP1);
        nd.maxP1 = std::max(nd.maxP1, ch.maxP1);
        nd.minP2 = std::min(nd.minP2, ch.minP2);
        nd.maxP2 = std::max(nd.maxP2, ch.maxP2);
    }
    pt.nodes[idx] = nd;
    return idx;
}

static PatriciaTrie buildPatriciaTrie(const Branch &br, const Constants &c, int NY, u64 &yCount,
                                       u64 &branchNodes, u64 &terminalNodes, double &buildSortSeconds,
                                       double &genSeconds, long rssBudgetKB = -1) {
    using clock = std::chrono::steady_clock;
    auto tg0 = clock::now();
    u128 Lc = c.L_c;
    u128 B12modLc = powmod_u128((u128)B, 12, Lc);
    std::vector<PatRecord> recs;
    u64 genCounter = 0;
    forEachPermutation(br.A19, NY, [&](const std::vector<int> &ydigits) {
        u128 y_val = 0, Bpow = 1;
        for (int i = 0; i < NY; i++) { y_val += (u128)ydigits[i] * Bpow; Bpow *= (u128)B; }
        u128 u_y = mulmod_u128(B12modLc, y_val % Lc, Lc);
        PatRecord rec;
        rec.digs = digitsLSDof(u_y);
        uint64_t m = 0; for (int d : ydigits) m |= bit(d);
        rec.ymask = m;
        rec.ny = (uint8_t)NY;
        for (int i = 0; i < NY; i++) rec.ydigits[i] = (uint8_t)ydigits[i];
        recs.push_back(rec);
        // Lightweight mid-generation guard (Phase C hard memory constraint):
        // record generation is the dominant linear-growth phase before the
        // structural build even starts, so check periodically rather than
        // only at the end. A full streaming/batched generator was judged
        // over-engineering given the ~0.6GB transient buffer size and 7GB+
        // headroom on this box; this periodic check is the cheap middle path.
        if (rssBudgetKB > 0 && (++genCounter % 2000000 == 0)) {
            checkRssBudget(rssBudgetKB, "mid-generation");
        }
    });
    yCount = recs.size();
    auto tg1 = clock::now();
    genSeconds = std::chrono::duration<double>(tg1 - tg0).count();
    checkRssBudget(rssBudgetKB, "post-generation");

    std::sort(recs.begin(), recs.end(), [](const PatRecord &a, const PatRecord &b) { return a.digs < b.digs; });
    checkRssBudget(rssBudgetKB, "post-sort");

    PatriciaTrie pt;
    pt.nodes.reserve(2 * recs.size() + 2);
    pt.childPool.reserve(2 * recs.size() + 2);
    patBuild(pt, recs, 0, (int)recs.size(), 0);

    branchNodes = 0; terminalNodes = 0;
    for (auto &n : pt.nodes) { if (n.isTerminal) terminalNodes++; else branchNodes++; }

    auto tg2 = clock::now();
    buildSortSeconds = std::chrono::duration<double>(tg2 - tg1).count();
    checkRssBudget(rssBudgetKB, "post-build");
    return pt;
}

struct PStackFrame {
    int32_t node;
    int depth;
    int borrow;
    uint64_t used;
    int32_t sumP1;
    int64_t sumP2;
    std::array<int, 12> leaf;
};

// Walk the compact trie for one (x, lift j) pair. Mirrors walkOne()'s pruning
// ladder exactly, but a node's compressed edge digits are consumed in a tight
// inline loop (no stack push, no node dispatch) before the ladder is
// evaluated at the node's true branch/terminal point. onSurvivor(leaf,node,
// xdigits) is invoked for every accepted terminal (used by the gate to
// reconstruct and directly re-verify).
template <typename OnSurvivor>
static void patWalkOne(const PatriciaTrie &pt, u128 W, uint64_t allowedMinusX, uint64_t xmask,
                        uint64_t A19mask, WalkStats &stats, const Branch &br, int32_t xP1, int64_t xP2,
                        OnSurvivor onSurvivor) {
    auto Wdig = digitsLSDof(W);
    std::vector<PStackFrame> stack;
    stack.reserve(64);
    stack.push_back({0, 0, 0, 0, 0, 0, {}});
    while (!stack.empty()) {
        PStackFrame f = stack.back();
        stack.pop_back();
        stats.visits++;
        const PatNode &node = pt.nodes[f.node];

        int depth = f.depth;
        int borrow = f.borrow;
        uint64_t used = f.used;
        int32_t sumP1 = f.sumP1;
        int64_t sumP2 = f.sumP2;
        std::array<int, 12> leaf = f.leaf;
        bool dead = false;
        for (int k = 0; k < node.edgeLen; k++) {
            uint8_t dig = node.edge[k];
            int wd = (int)Wdig[depth] - (int)dig - borrow;
            int nb = 0;
            if (wd < 0) { wd += B; nb = 1; }
            if (depth < 12) {
                if (wd == 0) { dead = true; break; }
                uint64_t wdbit = bit(wd);
                if (!(allowedMinusX & wdbit)) { dead = true; break; }
                if (used & wdbit) { dead = true; break; }
                used |= wdbit;
                leaf[depth] = wd;
                sumP1 += wd;
                sumP2 += (int64_t)wd * wd;
            } else {
                if (wd != 0) { dead = true; break; }
            }
            borrow = nb;
            depth++;
        }
        if (dead) continue;

        if (PRUNE_LEVEL >= 1) {
            uint64_t forbidden = xmask | used;
            if (node.andMask & forbidden) continue;
        }
        int R = (depth <= 12) ? (12 - depth) : 0;
        if (PRUNE_LEVEL >= 1 && depth >= 12) {
            uint64_t Rexact = A19mask & ~xmask & ~used;
            if (Rexact & ~node.orMask) continue;
        }
        uint64_t avail = A19mask & ~xmask & ~used;
        if (PRUNE_LEVEL >= 2) {
            int32_t loRemain = sumSmallestK(avail, R);
            int32_t hiRemain = sumLargestK(avail, R);
            int32_t loY1 = br.S_A19 - xP1 - sumP1 - hiRemain;
            int32_t hiY1 = br.S_A19 - xP1 - sumP1 - loRemain;
            if (node.maxP1 < loY1 || node.minP1 > hiY1) continue;
        }
        if (PRUNE_LEVEL >= 3) {
            int64_t loRemain2 = 0, hiRemain2 = 0;
            {
                uint64_t m1 = avail; for (int i = 0; i < R && m1; i++) { int p = __builtin_ctzll(m1); loRemain2 += (int64_t)p*p; m1 &= m1-1; }
                uint64_t m2 = avail; for (int i = 0; i < R && m2; i++) { int p = 63-__builtin_clzll(m2); hiRemain2 += (int64_t)p*p; m2 &= ~((uint64_t)1<<p); }
            }
            int64_t loY2 = br.S2_A19 - xP2 - sumP2 - hiRemain2;
            int64_t hiY2 = br.S2_A19 - xP2 - sumP2 - loRemain2;
            if (node.maxP2 < loY2 || node.minP2 > hiY2) continue;
        }

        if (node.isTerminal) {
            if (borrow == 0) {
                uint64_t ymask = node.termYmask;
                if (!(ymask & xmask) && used == (A19mask & ~ymask & ~xmask)) {
                    stats.survivors++;
                    onSurvivor(leaf, node, used);
                }
            }
            continue;
        }
        for (int ci = 0; ci < node.childCount; ci++) {
            auto &kv = pt.childPool[node.childrenStart + ci];
            uint8_t dig = kv.first; int32_t child = kv.second;
            int wd = (int)Wdig[depth] - (int)dig - borrow;
            int nb = 0;
            if (wd < 0) { wd += B; nb = 1; }
            if (depth < 12) {
                if (wd == 0) continue;
                uint64_t wdbit = bit(wd);
                if (!(allowedMinusX & wdbit)) continue;
                if (used & wdbit) continue;
                PStackFrame nf;
                nf.node = child; nf.depth = depth + 1; nf.borrow = nb;
                nf.used = used | wdbit; nf.sumP1 = sumP1 + wd; nf.sumP2 = sumP2 + (int64_t)wd * wd;
                nf.leaf = leaf; nf.leaf[depth] = wd;
                stack.push_back(nf);
            } else {
                if (wd != 0) continue;
                PStackFrame nf;
                nf.node = child; nf.depth = depth + 1; nf.borrow = nb;
                nf.used = used; nf.sumP1 = sumP1; nf.sumP2 = sumP2; nf.leaf = leaf;
                stack.push_back(nf);
            }
        }
    }
}

static long peakRssKB(); // fwd decl; defined below

// If maxRssKB > 0, abort cleanly (not a crash) once VmHWM crosses it. Used by
// Phase C's hard memory-discipline gate: NEVER let this process approach the
// live-trading BEAM's headroom.
static void checkRssBudget(long maxRssKB, const char *where) {
    if (maxRssKB <= 0) return;
    long kb = peakRssKB();
    if (kb >= 0 && kb > maxRssKB) {
        fprintf(stderr, "[ABORT] peak RSS %ldKB exceeded budget %ldKB at %s -- stopping cleanly (memory discipline is a hard constraint).\n",
                kb, maxRssKB, where);
        exit(2);
    }
}

static void runPatFull(const Constants &c, int candidate, int NX, int NY, long rssBudgetKB = -1) {
    if (candidate == 21) {
        if (!fileExists("/home/jes/a113028/alpha_go")) {
            fprintf(stderr, "[HOLD] alpha_go not present; candidate-21 patricia full run (NX=%d,NY=%d) withheld.\n", NX, NY);
            fprintf(stderr, "[HOLD] Reporting readiness and stopping per task spec.\n");
            return;
        }
        fprintf(stderr, "[full] alpha_go present; proceeding with candidate-21 patricia NX=%d,NY=%d\n", NX, NY);
    }
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();

    Branch br = buildBranch(c, candidate);
    u128 Lc = c.L_c;
    u64 yCount = 0, branchNodes = 0, terminalNodes = 0;
    double buildSortSeconds = 0, genSeconds = 0;
    PatriciaTrie pt = buildPatriciaTrie(br, c, NY, yCount, branchNodes, terminalNodes, buildSortSeconds, genSeconds, rssBudgetKB);
    u64 totalNodes = pt.nodes.size();
    fprintf(stderr, "[patricia] NY=%d: %llu y-arrangements, %llu compact nodes (branch=%llu terminal=%llu), "
                    "gen=%.3fs sort+build=%.3fs sizeof(PatNode)=%zu bytes childPool=%zu entries (%zu bytes)\n",
            NY, (unsigned long long)yCount, (unsigned long long)totalNodes,
            (unsigned long long)branchNodes, (unsigned long long)terminalNodes,
            genSeconds, buildSortSeconds, sizeof(PatNode), pt.childPool.size(),
            pt.childPool.size() * sizeof(std::pair<uint8_t,int32_t>));
    fprintf(stderr, "[patfull] peakRSS at build-end = %ldKB\n", peakRssKB());
    checkRssBudget(rssBudgetKB, "build-end");

    uint64_t A19mask = 0; for (int d : br.A19) A19mask |= bit(d);
    u128 Bexp = powmod_u128((u128)B, (u128)(12 + NY), Lc);

    u64 totalVisits = 0, totalSurvivors = 0;
    u64 xCounted = 0;
    forEachPermutation(br.A19, NX, [&](const std::vector<int> &xdigits) {
        xCounted++;
        u128 x_val = 0, Bpow = 1;
        for (int i = 0; i < NX; i++) { x_val += (u128)xdigits[i] * Bpow; Bpow *= (u128)B; }
        u128 term = mulmod_u128(Bexp, x_val % Lc, Lc);
        u128 c_x = (br.T_target + Lc - (term % Lc)) % Lc;
        uint64_t xmask = 0; for (int d : xdigits) xmask |= bit(d);
        uint64_t allowedMinusX = A19mask & ~xmask;
        int32_t xP1 = 0; int64_t xP2 = 0;
        for (int d : xdigits) { xP1 += d; xP2 += (int64_t)d * d; }

        WalkStats stats;
        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            patWalkOne(pt, W, allowedMinusX, xmask, A19mask, stats, br, xP1, xP2,
                       [](const std::array<int,12>&, const PatNode&, uint64_t){});
        }
        totalVisits += stats.visits;
        totalSurvivors += stats.survivors;
    });

    auto t1 = clock::now();
    double wall = std::chrono::duration<double>(t1 - t0).count();
    double meanVisitsPerX = xCounted ? (double)totalVisits / xCounted : 0.0;
    fprintf(stderr, "[patfull] candidate=%d NX=%d NY=%d: |X|=%llu |Y|=%llu total_visits=%llu mean_visits/x=%.2f "
                    "survivors=%llu wall=%.3fs (gen=%.3fs build=%.3fs search=%.3fs)\n",
            candidate, NX, NY, (unsigned long long)xCounted, (unsigned long long)yCount,
            (unsigned long long)totalVisits, meanVisitsPerX, (unsigned long long)totalSurvivors, wall,
            genSeconds, buildSortSeconds, wall - genSeconds - buildSortSeconds);
    fprintf(stderr, "[patfull] peakRSS at walk-end = %ldKB\n", peakRssKB());
    checkRssBudget(rssBudgetKB, "walk-end");
}

// Peak RSS of THIS process so far, in KB (Linux /proc self status VmHWM).
static long peakRssKB() {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[256];
    long kb = -1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmHWM:", 6) == 0) { sscanf(line + 6, "%ld", &kb); break; }
    }
    fclose(f);
    return kb;
}

static void runPatGate(const Constants &c, int NX, int NY, long rssBudgetKB = -1) {
    if (NX + NY != 7) { fprintf(stderr, "FATAL: pgate requires NX+NY==7 (got %d+%d)\n", NX, NY); exit(1); }
    Branch br = buildBranch(c, 20);
    fprintf(stderr, "[pgate] candidate=20 (known YES), split %d+%d (NX=%d,NY=%d), FULL run, PATRICIA builder\n", NX, NY, NX, NY);

    std::vector<int> rel(19);
    for (int i = 0; i < 19; i++) rel[i] = KNOWN_FREE_POS19_TO_1[18 - i];
    bool knownOk = verifySurvivorDirect(c, br, c.prefix, 20, rel);
    fprintf(stderr, "[pgate] known completion direct verification (N mod L == 0): %s\n", knownOk ? "PASS" : "FAIL");
    if (!knownOk) {
        fprintf(stderr, "[pgate] FATAL: known completion does not satisfy N mod L == 0; aborting gate.\n");
        exit(1);
    }
    // rel[0..11]=positions1..12 (leaf, fixed at P=12 regardless of split);
    // rel[12..12+NY-1]=y-block; rel[12+NY..18]=x-block (size NX=7-NY).
    std::vector<int> knownLeaf(rel.begin(), rel.begin() + 12);
    std::vector<int> knownY(rel.begin() + 12, rel.begin() + 12 + NY);
    std::vector<int> knownX(rel.begin() + 12 + NY, rel.begin() + 19);
    uint64_t knownYmask = 0; for (int d : knownY) knownYmask |= bit(d);
    uint64_t knownXmask = 0; for (int d : knownX) knownXmask |= bit(d);
    uint64_t knownLeafmask = 0; for (int d : knownLeaf) knownLeafmask |= bit(d);

    u128 Lc = c.L_c;
    u64 yCount = 0, branchNodes = 0, terminalNodes = 0;
    double buildSortSeconds = 0, genSeconds = 0;
    PatriciaTrie pt = buildPatriciaTrie(br, c, NY, yCount, branchNodes, terminalNodes, buildSortSeconds, genSeconds, rssBudgetKB);
    u64 totalNodes = pt.nodes.size();
    checkRssBudget(rssBudgetKB, "pgate-build-end");
    fprintf(stderr, "[pgate] patricia trie built: %llu y-arrangements, %llu compact nodes (branch=%llu terminal=%llu), "
                    "gen=%.3fs sort+build=%.3fs peakRSS=%ldKB\n",
            (unsigned long long)yCount, (unsigned long long)totalNodes,
            (unsigned long long)branchNodes, (unsigned long long)terminalNodes, genSeconds, buildSortSeconds,
            peakRssKB());

    uint64_t A19mask = 0; for (int d : br.A19) A19mask |= bit(d);
    u128 Bexp = powmod_u128((u128)B, (u128)(12 + NY), Lc);

    u64 totalVisits = 0, totalSurvivors = 0, verifiedOk = 0, verifiedBad = 0;
    bool foundKnown = false;
    u64 xCounted = 0;

    forEachPermutation(br.A19, NX, [&](const std::vector<int> &xdigits) {
        xCounted++;
        u128 x_val = 0, Bpow = 1;
        for (int i = 0; i < NX; i++) { x_val += (u128)xdigits[i] * Bpow; Bpow *= (u128)B; }
        u128 term = mulmod_u128(Bexp, x_val % Lc, Lc);
        u128 c_x = (br.T_target + Lc - (term % Lc)) % Lc;
        uint64_t xmask = 0; for (int d : xdigits) xmask |= bit(d);
        uint64_t allowedMinusX = A19mask & ~xmask;
        bool thisIsKnownX = true;
        for (int i = 0; i < NX; i++) if (xdigits[i] != knownX[i]) { thisIsKnownX = false; break; }
        int32_t xP1 = 0; int64_t xP2 = 0;
        for (int d : xdigits) { xP1 += d; xP2 += (int64_t)d * d; }

        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            WalkStats stats;
            patWalkOne(pt, W, allowedMinusX, xmask, A19mask, stats, br, xP1, xP2,
                [&](const std::array<int,12> &leaf, const PatNode &node, uint64_t used) {
                    std::vector<int> freePos1to19(19);
                    for (int i = 0; i < 12; i++) freePos1to19[i] = leaf[i];
                    for (int i = 0; i < node.termNY; i++) freePos1to19[12 + i] = node.termYdigits[i];
                    for (int i = 0; i < NX; i++) freePos1to19[12 + NY + i] = xdigits[i];
                    bool ok = verifySurvivorDirect(c, br, c.prefix, 20, freePos1to19);
                    if (ok) verifiedOk++; else verifiedBad++;
                    if (node.termYmask == knownYmask && xmask == knownXmask &&
                        used == knownLeafmask && thisIsKnownX) {
                        foundKnown = true;
                    }
                });
            totalVisits += stats.visits;
            totalSurvivors += stats.survivors;
        }
    });

    fprintf(stderr, "[pgate] x permutations=%llu, |Y|=%llu, total visits=%llu, survivors=%llu peakRSS=%ldKB\n",
            (unsigned long long)xCounted, (unsigned long long)yCount,
            (unsigned long long)totalVisits, (unsigned long long)totalSurvivors, peakRssKB());
    fprintf(stderr, "[pgate] direct-arithmetic verification of survivors: %llu OK, %llu FAILED\n",
            (unsigned long long)verifiedOk, (unsigned long long)verifiedBad);
    fprintf(stderr, "[pgate] known completion found among survivors: %s\n", foundKnown ? "YES" : "NO");
    bool pass = foundKnown && (verifiedBad == 0) && (totalSurvivors == verifiedOk) && (totalSurvivors == 1);
    fprintf(stderr, "[pgate] SOUNDNESS GATE: %s\n", pass ? "PASS" : "FAIL");
    checkRssBudget(rssBudgetKB, "pgate-walk-end");
}

static bool fileExists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static void runFull(const Constants &c, int candidate, int NX, int NY) {
    if (candidate == 21) {
        if (!fileExists("/home/jes/a113028/alpha_go")) {
            fprintf(stderr, "[HOLD] alpha_go not present; candidate-21 full run (NX=%d,NY=%d) withheld.\n", NX, NY);
            fprintf(stderr, "[HOLD] Reporting readiness and stopping per task spec.\n");
            return;
        }
        fprintf(stderr, "[full] alpha_go present; proceeding with candidate-21 NX=%d,NY=%d\n", NX, NY);
    }
    Branch br = buildBranch(c, candidate);
    RunResult rr = runMeasurement(br, c, NX, NY, -1, false, nullptr);
    double meanVisitsPerX = rr.xCount ? (double)rr.totalVisits / rr.xCount : 0.0;
    fprintf(stderr, "[full] candidate=%d NX=%d NY=%d: |X|=%llu |Y|=%llu total_visits=%llu mean_visits/x=%.2f survivors=%llu wall=%.3fs\n",
            candidate, NX, NY,
            (unsigned long long)rr.xCount, (unsigned long long)rr.yCount,
            (unsigned long long)rr.totalVisits, meanVisitsPerX,
            (unsigned long long)rr.totalSurvivors, rr.wallSeconds);
}

int main(int argc, char **argv) {
    Constants c = deriveConstants();
    printGuardSummary(c);

    if (argc < 2) {
        fprintf(stderr, "usage: %s smoke|gate|full|pgate|pfull <cand> <NX> <NY> [pruneLevel]\n", argv[0]);
        fprintf(stderr, "  pgate/pfull: PATRICIA-CARRY-TRIE.md Phase A path-compressed builder/walker\n");
        fprintf(stderr, "  pruneLevel: 0=none 1=rung1(mask-union) 2=+rung2a(p1) 3=+rung2b(p2) [default 3]\n");
        return 1;
    }
    std::string mode = argv[1];
    if (mode == "smoke") {
        if (argc >= 3) PRUNE_LEVEL = atoi(argv[2]);
        fprintf(stderr, "[prune] level=%d\n", PRUNE_LEVEL);
        runSmoke(c);
    } else if (mode == "gate") {
        if (argc >= 3) PRUNE_LEVEL = atoi(argv[2]);
        fprintf(stderr, "[prune] level=%d\n", PRUNE_LEVEL);
        runGate(c);
    } else if (mode == "full") {
        if (argc < 5) { fprintf(stderr, "full requires <cand> <NX> <NY>\n"); return 1; }
        int cand = atoi(argv[2]);
        int NX = atoi(argv[3]);
        int NY = atoi(argv[4]);
        if (argc >= 6) PRUNE_LEVEL = atoi(argv[5]);
        fprintf(stderr, "[prune] level=%d\n", PRUNE_LEVEL);
        runFull(c, cand, NX, NY);
    } else if (mode == "pgate") {
        // pgate [NX] [NY] [pruneLevel] [rssBudgetKB]   (default 3 4, for Phase-A back-compat)
        int NX = 3, NY = 4;
        if (argc >= 4) { NX = atoi(argv[2]); NY = atoi(argv[3]); }
        if (argc >= 5) PRUNE_LEVEL = atoi(argv[4]);
        long rssBudgetKB = -1;
        if (argc >= 6) rssBudgetKB = atol(argv[5]);
        fprintf(stderr, "[prune] level=%d rssBudgetKB=%ld\n", PRUNE_LEVEL, rssBudgetKB);
        runPatGate(c, NX, NY, rssBudgetKB);
    } else if (mode == "pfull") {
        // pfull <cand> <NX> <NY> [pruneLevel] [rssBudgetKB]
        if (argc < 5) { fprintf(stderr, "pfull requires <cand> <NX> <NY>\n"); return 1; }
        int cand = atoi(argv[2]);
        int NX = atoi(argv[3]);
        int NY = atoi(argv[4]);
        if (argc >= 6) PRUNE_LEVEL = atoi(argv[5]);
        long rssBudgetKB = -1;
        if (argc >= 7) rssBudgetKB = atol(argv[6]);
        fprintf(stderr, "[prune] level=%d rssBudgetKB=%ld\n", PRUNE_LEVEL, rssBudgetKB);
        runPatFull(c, cand, NX, NY, rssBudgetKB);
    } else {
        fprintf(stderr, "unknown mode %s\n", mode.c_str());
        return 1;
    }
    return 0;
}
