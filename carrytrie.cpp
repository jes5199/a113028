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
#include <set>
#include <unordered_map>
#include <optional>
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

// ================= Lift-envelope sieve (LIFT-ENVELOPE-SIEVE.md) =================
// SIEVE_LEVEL is a bitmask, set from CLI:
//   bit0 (1) = root envelope sieve (§3/§4): reject an (x,lift) root before it
//              ever enters the trie, using only the leaf's rearrangement
//              envelope over the live 12-digit pool.
//   bit1 (2) = every-node envelope (§6): the same test re-applied at every
//              trie node using the node's aggregated u_y residue range.
//   bit2 (4) = joint depth-3 mask-and-envelope certificate (§7): at depth==3
//              nodes, ask the joint question over each distinct descendant
//              y-mask instead of the coarse andMask/orMask summary.
// These compose with PRUNE_LEVEL (CARRY-TRIE-JOIN.md's existing ladder); the
// sieve is strictly additive pruning on top of it, never a replacement.
static int SIEVE_LEVEL = 0; // set from CLI; see main()

struct LeafEnvelope { u128 lo; u128 hi; };

// Extremal rearrangement envelope (§2/§4): given a live digit pool and a slot
// count, the minimum arrangement places the `count` smallest digits in
// increasing order from most- to least-significant; the maximum places the
// `count` largest digits in decreasing order. Sound over-approximation for
// any subset/arrangement drawn from `available` (see doc's proof, §2).
static LeafEnvelope leafEnvelope(uint64_t available, int count) {
    // digits are naturally ascending: __builtin_ctzll walks low-to-high bits.
    int digs[19]; int nd = 0;
    uint64_t av = available;
    while (av) { digs[nd++] = __builtin_ctzll(av); av &= av - 1; }
    LeafEnvelope out{0, 0};
    for (int i = 0; i < count; i++) out.lo = out.lo * (u128)B + (u128)digs[i];
    for (int i = 0; i < count; i++) out.hi = out.hi * (u128)B + (u128)digs[nd - 1 - i];
    return out;
}

// Generalized interval-intersection test (§3, generalized to an arbitrary
// closed residue-domain [domLo,domHi] instead of just [0,L_c)): does some
// u in [domLo,domHi] satisfy leaf_lo <= W-u <= leaf_hi? Equivalently, does
// [W-leaf_hi, W-leaf_lo] intersect [domLo,domHi]? All unsigned-safe.
static inline bool envelopeIntersects(u128 W, u128 leaf_lo, u128 leaf_hi, u128 domLo, u128 domHi) {
    if (W < leaf_lo) return false; // required u = W-leaf_lo would be negative
    u128 hiU = W - leaf_lo;
    u128 loU = (W > leaf_hi) ? (W - leaf_hi) : (u128)0;
    if (hiU < domLo) return false;
    if (loU > domHi) return false;
    return true;
}

// Root-level test (§3/§4): specializes envelopeIntersects to domain [0,L_c).
static inline bool liftCouldContainLeaf(u128 W, u128 Lc, const LeafEnvelope &env) {
    return envelopeIntersects(W, env.lo, env.hi, 0, Lc - 1);
}

// B^0..B^12 as exact (unreduced) u128 integers, used to place a remaining
// leaf block (positions d..11) at its true weight B^d. 49^12 ~ 2^67, so this
// fits comfortably in u128 with no overflow risk.
static std::array<u128, 13> gBpow12;
static void initBpow12() {
    u128 p = 1;
    for (int i = 0; i <= 12; i++) { gBpow12[i] = p; p *= (u128)B; }
}

// Root-level sieve bookkeeping, reset per run.
struct SieveStats {
    u64 rootsTotal = 0;
    u64 rootsRejected = 0;
};

struct TrieNode {
    // children: (edge digit 0..48, child node index)
    std::vector<std::pair<uint8_t,int32_t>> children;
    // present only at depth==DEPTH nodes: ordered y-digit-lists (positions 12..12+NY-1,
    // i.e. index0 = relative position 12) that hash to this trie leaf.
    std::vector<std::vector<int>> terminalY;
    // parallel to terminalY: each terminal's exact u_y residue (used by the
    // every-node envelope sieve, SIEVE_LEVEL bit1).
    std::vector<u128> terminalUY;

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

    // LIFT-ENVELOPE-SIEVE.md §6: min/max u_y residue over every descendant
    // terminal, used for the every-node envelope test.
    u128 minU = ~(u128)0;
    u128 maxU = 0;
};

static const int DEPTH = 14;

struct Trie {
    std::vector<TrieNode> nodes;
    // §7 joint depth-3 mask-and-envelope certificate: node index (always at
    // tree-level 3, since the classic trie is one digit per level) -> sorted
    // unique y-masks among its descendant terminals. Built once, post-aggregate.
    std::unordered_map<int32_t, std::vector<uint64_t>> depth3Masks;

    Trie() { nodes.emplace_back(); } // root = 0

    int32_t childOrCreate(int32_t node, uint8_t dig) {
        for (auto &pr : nodes[node].children) if (pr.first == dig) return pr.second;
        int32_t idx = (int32_t)nodes.size();
        nodes.emplace_back();
        nodes[node].children.push_back({dig, idx});
        return idx;
    }

    void insert(const std::array<uint8_t, DEPTH> &digitsLSD, const std::vector<int> &ydigitsOrdered, u128 u_y) {
        int32_t cur = 0;
        for (int d = 0; d < DEPTH; d++) cur = childOrCreate(cur, digitsLSD[d]);
        nodes[cur].terminalY.push_back(ydigitsOrdered);
        nodes[cur].terminalUY.push_back(u_y);
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
            node.minU = std::min(node.minU, child.minU);
            node.maxU = std::max(node.maxU, child.maxU);
        }
        for (size_t i = 0; i < node.terminalY.size(); i++) {
            const auto &ydigits = node.terminalY[i];
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
            u128 uy = node.terminalUY[i];
            node.minU = std::min(node.minU, uy);
            node.maxU = std::max(node.maxU, uy);
        }
    }

    // §7: collect the set of unique descendant y-masks under `node`.
    void collectMasks(int32_t node, std::set<uint64_t> &out) const {
        const TrieNode &n = nodes[node];
        for (auto &ydigits : n.terminalY) {
            uint64_t m = 0; for (int d : ydigits) m |= ((uint64_t)1 << d);
            out.insert(m);
        }
        for (auto &pr : n.children) collectMasks(pr.second, out);
    }

    // Populate depth3Masks by a single DFS from the root; every node at
    // tree-level 3 gets its descendant-terminal mask set recorded.
    void buildDepth3Masks(int32_t node, int depth) {
        if (depth == 3) {
            std::set<uint64_t> s;
            collectMasks(node, s);
            depth3Masks[node] = std::vector<uint64_t>(s.begin(), s.end());
            return;
        }
        const TrieNode &n = nodes[node];
        for (auto &pr : n.children) buildDepth3Masks(pr.second, depth + 1);
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
    u128 leafVal;    // running ordinary integer value of leaf digits fixed so
                      // far, at their TRUE weight (digit at position i * B^i);
                      // used by the every-node/depth-3 lift-envelope sieve.
};

// Walk the trie for one (x, lift j) pair. allowedMinusX = A19mask & ~xmask.
// xP1/xP2 are the (fixed, per-x) sum and sum-of-squares of the x digits.
static void walkOne(const Trie &trie, u128 W, uint64_t allowedMinusX, uint64_t xmask,
                     uint64_t A19mask, WalkStats &stats,
                     const Branch &br, int32_t xP1, int64_t xP2) {
    auto Wdig = digitsLSDof(W);
    std::vector<StackFrame> stack;
    stack.reserve(64);
    stack.push_back({0, 0, 0, 0, 0, 0, 0});
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
        // ---- lift-envelope sieve (LIFT-ENVELOPE-SIEVE.md §6/§7), additive on
        // top of the pruning ladder above ----
        if ((SIEVE_LEVEL & 2) && f.depth <= 12) {
            // Every-node envelope (§6): bound the completed leaf value using
            // the R still-open positions' extremal rearrangement of `avail`,
            // then test against this node's aggregated u_y residue range.
            LeafEnvelope blockEnv = leafEnvelope(avail, R);
            u128 leaf_lo = f.leafVal + blockEnv.lo * gBpow12[f.depth];
            u128 leaf_hi = f.leafVal + blockEnv.hi * gBpow12[f.depth];
            if (!envelopeIntersects(W, leaf_lo, leaf_hi, node.minU, node.maxU)) continue;
        }
        if ((SIEVE_LEVEL & 4) && f.depth == 3) {
            // Joint depth-3 mask-and-envelope certificate (§7): ask, per
            // distinct descendant y-mask compatible with digits used so far,
            // whether ITS exact remaining-leaf envelope intersects the node's
            // residue range. Retain only if at least one mask survives.
            auto it = trie.depth3Masks.find(f.node);
            bool anyOK = (it == trie.depth3Masks.end()); // no table entry (shouldn't
                                                           // happen) -> don't prune
            if (it != trie.depth3Masks.end()) {
                uint64_t usedOrX = xmask | f.used;
                for (uint64_t m : it->second) {
                    if (m & usedOrX) continue;
                    uint64_t remaining = A19mask & ~xmask & ~f.used & ~m;
                    if (__builtin_popcountll(remaining) != R) continue; // exact-partition guard
                    LeafEnvelope be = leafEnvelope(remaining, R);
                    u128 leaf_lo = f.leafVal + be.lo * gBpow12[f.depth];
                    u128 leaf_hi = f.leafVal + be.hi * gBpow12[f.depth];
                    if (envelopeIntersects(W, leaf_lo, leaf_hi, node.minU, node.maxU)) { anyOK = true; break; }
                }
            }
            if (!anyOK) continue;
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
                                  f.sumP1 + wd, f.sumP2 + (int64_t)wd * wd,
                                  f.leafVal + (u128)wd * gBpow12[f.depth]});
            } else {
                if (wd != 0) continue;
                stack.push_back({child, f.depth + 1, nb, f.used, f.sumP1, f.sumP2, f.leafVal});
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
        trie.insert(digs, ydigits, u_y);
        yCount++;
    });
    trie.computeAggregates(0);
    if (SIEVE_LEVEL & 4) trie.buildDepth3Masks(0, 0);
    return trie;
}

// ================= Main measurement driver =================
struct RunResult {
    u64 totalVisits = 0;
    u64 totalSurvivors = 0;
    u64 xCount = 0;
    u64 yCount = 0;
    double wallSeconds = 0;
    u64 rootsTotal = 0;
    u64 rootsRejected = 0;
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

        // Root envelope sieve (LIFT-ENVELOPE-SIEVE.md §3/§4): computed once
        // per x, outside the lift loop.
        LeafEnvelope env{};
        if (SIEVE_LEVEL & 1) env = leafEnvelope(allowedMinusX, 12);

        WalkStats stats;
        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            if (SIEVE_LEVEL & 1) {
                rr.rootsTotal++;
                if (!liftCouldContainLeaf(W, Lc, env)) { rr.rootsRejected++; continue; }
            }
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

static void runGate(const Constants &c, int NX = 3, int NY = 4) {
    if (NX + NY != 7) { fprintf(stderr, "FATAL: gate requires NX+NY==7 (got %d+%d)\n", NX, NY); exit(1); }
    Branch br = buildBranch(c, 20);
    fprintf(stderr, "[gate] candidate=20 (known YES), split %d+%d (NX=%d,NY=%d), FULL run\n", NX, NY, NX, NY);

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

    // rel[0..11]=leaf (fixed, P=12); rel[12..12+NY-1]=y-block; rel[12+NY..18]=x-block.
    std::vector<int> knownLeaf(rel.begin(), rel.begin() + 12);
    std::vector<int> knownY(rel.begin() + 12, rel.begin() + 12 + NY);
    std::vector<int> knownX(rel.begin() + 12 + NY, rel.begin() + 19);
    uint64_t knownYmask = 0; for (int d : knownY) knownYmask |= bit(d);
    uint64_t knownXmask = 0; for (int d : knownX) knownXmask |= bit(d);
    uint64_t knownLeafmask = 0; for (int d : knownLeaf) knownLeafmask |= bit(d);

    // Full run, but capture whether the known (x) permutation + resulting
    // survivor with knownYmask/knownLeafmask occurs.
    u128 Lc = c.L_c;
    u64 yCount = 0;
    Trie trie = buildTrie(br, c, NX, NY, yCount);
    fprintf(stderr, "[gate] trie built: %llu y-arrangements, %zu nodes\n",
            (unsigned long long)yCount, trie.nodes.size());

    uint64_t A19mask = 0; for (int d : br.A19) A19mask |= bit(d);
    u128 Bexp = powmod_u128((u128)B, (u128)(12 + NY), Lc);

    u64 totalVisits = 0, totalSurvivors = 0, verifiedOk = 0, verifiedBad = 0;
    u64 rootsTotal = 0, rootsRejected = 0;
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
        u128 leafVal;
        std::array<int,12> leaf; // leaf[0..depth-1] valid when depth<=12
    };

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

        LeafEnvelope env{};
        if (SIEVE_LEVEL & 1) env = leafEnvelope(allowedMinusX, 12);

        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            if (SIEVE_LEVEL & 1) {
                rootsTotal++;
                if (!liftCouldContainLeaf(W, Lc, env)) { rootsRejected++; continue; }
            }
            auto Wdig = digitsLSDof(W);
            std::vector<DecodeFrame> stack;
            stack.push_back({0,0,0,0,0,0,0,{}});
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
                if ((SIEVE_LEVEL & 2) && f.depth <= 12) {
                    LeafEnvelope blockEnv = leafEnvelope(avail, R);
                    u128 leaf_lo = f.leafVal + blockEnv.lo * gBpow12[f.depth];
                    u128 leaf_hi = f.leafVal + blockEnv.hi * gBpow12[f.depth];
                    if (!envelopeIntersects(W, leaf_lo, leaf_hi, node.minU, node.maxU)) continue;
                }
                if ((SIEVE_LEVEL & 4) && f.depth == 3) {
                    auto it = trie.depth3Masks.find(f.node);
                    bool anyOK = (it == trie.depth3Masks.end());
                    if (it != trie.depth3Masks.end()) {
                        uint64_t usedOrX = xmask | f.used;
                        for (uint64_t m : it->second) {
                            if (m & usedOrX) continue;
                            uint64_t remaining = A19mask & ~xmask & ~f.used & ~m;
                            if (__builtin_popcountll(remaining) != R) continue;
                            LeafEnvelope be = leafEnvelope(remaining, R);
                            u128 leaf_lo = f.leafVal + be.lo * gBpow12[f.depth];
                            u128 leaf_hi = f.leafVal + be.hi * gBpow12[f.depth];
                            if (envelopeIntersects(W, leaf_lo, leaf_hi, node.minU, node.maxU)) { anyOK = true; break; }
                        }
                    }
                    if (!anyOK) continue;
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
                            for (int i = 0; i < NY; i++) freePos1to19[12+i] = ydigits[i];   // y-block
                            for (int i = 0; i < NX; i++) freePos1to19[12+NY+i] = xdigits[i]; // x-block
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
                        nf.leafVal = f.leafVal + (u128)wd * gBpow12[f.depth];
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
    if (SIEVE_LEVEL & 1) {
        double pct = rootsTotal ? 100.0 * (double)rootsRejected / (double)rootsTotal : 0.0;
        fprintf(stderr, "[gate][sieve] roots total=%llu rejected=%llu (%.1f%%)\n",
                (unsigned long long)rootsTotal, (unsigned long long)rootsRejected, pct);
    }
    fprintf(stderr, "[gate] direct-arithmetic verification of survivors: %llu OK, %llu FAILED\n",
            (unsigned long long)verifiedOk, (unsigned long long)verifiedBad);
    fprintf(stderr, "[gate] known completion found among survivors: %s\n", foundKnown ? "YES" : "NO");
    bool pass = foundKnown && (verifiedBad == 0) && (totalSurvivors == verifiedOk) && (totalSurvivors == 1);
    fprintf(stderr, "[gate] SOUNDNESS GATE: %s\n", pass ? "PASS" : "FAIL");
}

static bool fileExists(const char *path); // fwd decl; defined below, used by runPatFull's HOLD gate

// ================= Shallow radix buckets — SHALLOW-RADIX-BUCKET-JOIN.md Phase A =================
//
// Third A/B arm, independent of both the classic pointer trie and the
// Patricia path above. No trie of any kind is built: a flat (u_y, y_mask)
// record array is counting-sorted into 49^K buckets keyed by u_y mod 49^K.
// For each surviving (x,lift) root (root envelope sieve still applies), the
// first K leaf digits are enumerated directly (ordered K-permutations of the
// digits available to the leaf), the subtraction/borrow recurrence derives
// the bucket key exactly (§4), and every record in that bucket is tested
// directly (§5) with a full 12-digit decode — no approximate terminal
// predicate, no trie dispatch, no per-node summaries.

struct BucketRecord {
    u128 u;
    uint64_t yMask;
};

struct BucketIndex {
    std::vector<uint32_t> offsets; // size bucketCount+1
    std::vector<BucketRecord> records;
    int K = 3;
    u64 bucketCount = 0;
};

static BucketIndex buildBucketIndex(const Branch &br, const Constants &c, int NY, int K,
                                     u64 &yCount, double &genSeconds, double &sortSeconds,
                                     long rssBudgetKB = -1) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    u128 Lc = c.L_c;
    u128 B12modLc = powmod_u128((u128)B, 12, Lc);

    u64 bucketCount = 1;
    for (int i = 0; i < K; i++) bucketCount *= (u64)B;
    u128 bucketMod = (u128)bucketCount;

    // Pass 1: generate all records + their bucket key, tally counts.
    std::vector<BucketRecord> raw;
    std::vector<uint32_t> key;
    std::vector<uint32_t> counts(bucketCount + 1, 0);
    forEachPermutation(br.A19, NY, [&](const std::vector<int> &ydigits) {
        u128 y_val = 0, Bpow = 1;
        for (int i = 0; i < NY; i++) { y_val += (u128)ydigits[i] * Bpow; Bpow *= (u128)B; }
        u128 u_y = mulmod_u128(B12modLc, y_val % Lc, Lc);
        uint64_t m = 0; for (int d : ydigits) m |= bit(d);
        raw.push_back({u_y, m});
        uint32_t k = (uint32_t)(u_y % bucketMod);
        key.push_back(k);
        counts[k + 1]++;
    });
    yCount = raw.size();
    auto t1 = clock::now();
    genSeconds = std::chrono::duration<double>(t1 - t0).count();
    checkRssBudget(rssBudgetKB, "bucket-post-generation");

    // Pass 2: prefix-sum offsets, scatter into final bucket-sorted array.
    for (u64 i = 0; i < bucketCount; i++) counts[i + 1] += counts[i];
    BucketIndex idx;
    idx.K = K;
    idx.bucketCount = bucketCount;
    idx.offsets = counts; // counts is now the exclusive-prefix-sum offsets array
    idx.records.resize(raw.size());
    std::vector<uint32_t> cursor(idx.offsets.begin(), idx.offsets.end());
    for (size_t i = 0; i < raw.size(); i++) {
        uint32_t k = key[i];
        idx.records[cursor[k]++] = raw[i];
    }
    auto t2 = clock::now();
    sortSeconds = std::chrono::duration<double>(t2 - t1).count();
    checkRssBudget(rssBudgetKB, "bucket-post-build");
    return idx;
}

// Enumerate ordered K-permutations of `allowed` leaf digits, propagating the
// subtraction borrow exactly (§4) to derive each candidate bucket key, and
// invoke cb(key, lowLeafMask) for every one. No std::function: CB is a
// template parameter captured by reference through the whole recursion, so
// this compiles down to a tight inlined loop nest for the small K in use.
template <typename CB>
static void enumLeafPrefix(int depth, int K, int borrow, uint64_t used, uint64_t key, uint64_t keyMul,
                            const std::array<uint8_t, DEPTH> &Wdig, uint64_t allowed, CB &&cb) {
    if (depth == K) { cb(key, used); return; }
    uint64_t rem = allowed & ~used;
    while (rem) {
        int l = __builtin_ctzll(rem);
        rem &= rem - 1;
        int raw = (int)Wdig[depth] - borrow - l;
        int u_i, nb;
        if (raw < 0) { u_i = raw + B; nb = 1; } else { u_i = raw; nb = 0; }
        enumLeafPrefix(depth + 1, K, nb, used | bit(l), key + (uint64_t)u_i * keyMul, keyMul * (uint64_t)B,
                       Wdig, allowed, cb);
    }
}

// Decode a full 12-digit base-49 leaf value, rejecting zero/unavailable/
// duplicate digits (§5 steps 5-6). `allowed` is A19mask & ~xmask (the y-mask
// disjointness half of the test is applied separately by the caller).
static inline bool decodeDistinctLeaf12(u128 leaf, uint64_t allowed, uint64_t &outMask) {
    uint64_t mask = 0;
    for (int i = 0; i < 12; i++) {
        int d = (int)(leaf % (u128)B);
        leaf /= (u128)B;
        if (d == 0) return false;
        uint64_t db = bit(d);
        if (!(allowed & db)) return false;
        if (mask & db) return false;
        mask |= db;
    }
    outMask = mask;
    return true;
}

struct BucketStats {
    u64 roots = 0;
    u64 rootsRejected = 0;
    u64 lookups = 0;
    u64 scans = 0;
    u64 maskPasses = 0;
    u64 survivors = 0;
};

// Search all surviving roots for one x (all lifts) against the bucket index,
// accumulating stats. onSurvivor(xdigits, r, decodedLeafMask-not-needed) is
// invoked per accepted record for gate-mode reconstruction/verification.
template <typename OnSurvivor>
static void bucketSearchX(const BucketIndex &idx, u128 c_x, u128 Lc, int lifts,
                           uint64_t xmask, uint64_t A19mask, int K, BucketStats &stats,
                           OnSurvivor &&onSurvivor) {
    uint64_t allowed = A19mask & ~xmask; // digits available to the whole leaf
    LeafEnvelope env{};
    if (SIEVE_LEVEL & 1) env = leafEnvelope(allowed, 12);

    for (int j = 0; j < lifts; j++) {
        u128 W = c_x + (u128)j * Lc;
        if (SIEVE_LEVEL & 1) {
            stats.roots++;
            if (!liftCouldContainLeaf(W, Lc, env)) { stats.rootsRejected++; continue; }
        } else {
            stats.roots++;
        }
        auto Wdig = digitsLSDof(W);
        enumLeafPrefix(0, K, 0, 0, 0, 1, Wdig, allowed,
            [&](uint64_t key, uint64_t lowLeafMask) {
                stats.lookups++;
                uint32_t lo = idx.offsets[key], hi = idx.offsets[key + 1];
                for (uint32_t p = lo; p < hi; p++) {
                    stats.scans++;
                    const BucketRecord &r = idx.records[p];
                    if (r.yMask & (xmask | lowLeafMask)) continue;
                    if (W < r.u) continue;
                    u128 leaf = W - r.u;
                    if (leaf >= gBpow12[12]) continue;
                    stats.maskPasses++;
                    uint64_t decodedMask;
                    if (!decodeDistinctLeaf12(leaf, allowed, decodedMask)) continue;
                    uint64_t required = A19mask & ~xmask & ~r.yMask;
                    if (decodedMask != required) continue;
                    stats.survivors++;
                    onSurvivor(r, leaf, decodedMask);
                }
            });
    }
}

static void runBucketFull(const Constants &c, int candidate, int NX, int NY, int K = 3, long rssBudgetKB = -1) {
    if (candidate == 21) {
        if (!fileExists("/home/jes/a113028/alpha_go")) {
            fprintf(stderr, "[HOLD] alpha_go not present; candidate-21 bucket full run (NX=%d,NY=%d) withheld.\n", NX, NY);
            return;
        }
        fprintf(stderr, "[full] alpha_go present; proceeding with candidate-21 bucket NX=%d,NY=%d K=%d\n", NX, NY, K);
    }
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();

    Branch br = buildBranch(c, candidate);
    u128 Lc = c.L_c;
    u64 yCount = 0;
    double genSeconds = 0, sortSeconds = 0;
    BucketIndex idx = buildBucketIndex(br, c, NY, K, yCount, genSeconds, sortSeconds, rssBudgetKB);
    fprintf(stderr, "[bucket] NY=%d K=%d: %llu records, %llu buckets, gen=%.3fs sort/scatter=%.3fs "
                    "sizeof(BucketRecord)=%zu bytes recordsBytes=%zu offsetsBytes=%zu peakRSS=%ldKB\n",
            NY, K, (unsigned long long)yCount, (unsigned long long)idx.bucketCount, genSeconds, sortSeconds,
            sizeof(BucketRecord), idx.records.size() * sizeof(BucketRecord),
            idx.offsets.size() * sizeof(uint32_t), peakRssKB());
    checkRssBudget(rssBudgetKB, "bucket-build-end");

    uint64_t A19mask = 0; for (int d : br.A19) A19mask |= bit(d);
    u128 Bexp = powmod_u128((u128)B, (u128)(12 + NY), Lc);

    BucketStats stats;
    u64 xCounted = 0;
    forEachPermutation(br.A19, NX, [&](const std::vector<int> &xdigits) {
        xCounted++;
        u128 x_val = 0, Bpow = 1;
        for (int i = 0; i < NX; i++) { x_val += (u128)xdigits[i] * Bpow; Bpow *= (u128)B; }
        u128 term = mulmod_u128(Bexp, x_val % Lc, Lc);
        u128 c_x = (br.T_target + Lc - (term % Lc)) % Lc;
        uint64_t xmask = 0; for (int d : xdigits) xmask |= bit(d);
        bucketSearchX(idx, c_x, Lc, c.lifts, xmask, A19mask, K, stats,
                      [](const BucketRecord &, u128, uint64_t) {});
    });

    auto t1 = clock::now();
    double wall = std::chrono::duration<double>(t1 - t0).count();
    fprintf(stderr, "[bucketfull] candidate=%d NX=%d NY=%d K=%d: |X|=%llu |Y|=%llu roots=%llu rootsRejected=%llu "
                    "lookups=%llu scans=%llu maskPasses=%llu survivors=%llu wall=%.3fs (gen=%.3fs build=%.3fs search=%.3fs)\n",
            candidate, NX, NY, K, (unsigned long long)xCounted, (unsigned long long)yCount,
            (unsigned long long)stats.roots, (unsigned long long)stats.rootsRejected,
            (unsigned long long)stats.lookups, (unsigned long long)stats.scans,
            (unsigned long long)stats.maskPasses, (unsigned long long)stats.survivors, wall,
            genSeconds, sortSeconds, wall - genSeconds - sortSeconds);
    fprintf(stderr, "[bucketfull] peakRSS at walk-end = %ldKB\n", peakRssKB());
    checkRssBudget(rssBudgetKB, "bucket-walk-end");
}

static void runBucketGate(const Constants &c, int NX, int NY, int K = 3, long rssBudgetKB = -1) {
    if (NX + NY != 7) { fprintf(stderr, "FATAL: bgate requires NX+NY==7 (got %d+%d)\n", NX, NY); exit(1); }
    Branch br = buildBranch(c, 20);
    fprintf(stderr, "[bgate] candidate=20 (known YES), split %d+%d (NX=%d,NY=%d) K=%d, FULL run, SHALLOW-BUCKET\n",
            NX, NY, NX, NY, K);

    std::vector<int> rel(19);
    for (int i = 0; i < 19; i++) rel[i] = KNOWN_FREE_POS19_TO_1[18 - i];
    bool knownOk = verifySurvivorDirect(c, br, c.prefix, 20, rel);
    fprintf(stderr, "[bgate] known completion direct verification (N mod L == 0): %s\n", knownOk ? "PASS" : "FAIL");
    if (!knownOk) { fprintf(stderr, "[bgate] FATAL: known completion fails; aborting.\n"); exit(1); }

    std::vector<int> knownLeaf(rel.begin(), rel.begin() + 12);
    std::vector<int> knownY(rel.begin() + 12, rel.begin() + 12 + NY);
    std::vector<int> knownX(rel.begin() + 12 + NY, rel.begin() + 19);
    uint64_t knownYmask = 0; for (int d : knownY) knownYmask |= bit(d);
    uint64_t knownXmask = 0; for (int d : knownX) knownXmask |= bit(d);
    uint64_t knownLeafmask = 0; for (int d : knownLeaf) knownLeafmask |= bit(d);

    u128 Lc = c.L_c;
    u64 yCount = 0;
    double genSeconds = 0, sortSeconds = 0;
    BucketIndex idx = buildBucketIndex(br, c, NY, K, yCount, genSeconds, sortSeconds, rssBudgetKB);
    fprintf(stderr, "[bgate] bucket index built: %llu records, %llu buckets, gen=%.3fs sort/scatter=%.3fs peakRSS=%ldKB\n",
            (unsigned long long)yCount, (unsigned long long)idx.bucketCount, genSeconds, sortSeconds, peakRssKB());
    checkRssBudget(rssBudgetKB, "bgate-build-end");

    uint64_t A19mask = 0; for (int d : br.A19) A19mask |= bit(d);
    u128 Bexp = powmod_u128((u128)B, (u128)(12 + NY), Lc);

    u64 verifiedOk = 0, verifiedBad = 0;
    bool foundKnown = false;
    u64 xCounted = 0;
    BucketStats stats;

    forEachPermutation(br.A19, NX, [&](const std::vector<int> &xdigits) {
        xCounted++;
        u128 x_val = 0, Bpow = 1;
        for (int i = 0; i < NX; i++) { x_val += (u128)xdigits[i] * Bpow; Bpow *= (u128)B; }
        u128 term = mulmod_u128(Bexp, x_val % Lc, Lc);
        u128 c_x = (br.T_target + Lc - (term % Lc)) % Lc;
        uint64_t xmask = 0; for (int d : xdigits) xmask |= bit(d);
        bool thisIsKnownX = true;
        for (int i = 0; i < NX; i++) if (xdigits[i] != knownX[i]) { thisIsKnownX = false; break; }

        bucketSearchX(idx, c_x, Lc, c.lifts, xmask, A19mask, K, stats,
            [&](const BucketRecord &r, u128 leaf, uint64_t decodedMask) {
                // Reconstruct full free-digit vector (positions 1..19): decode
                // the 12 leaf digits LSD-first (position i+1 = digit i), then
                // the y-block from r's actual ordered digits is NOT stored
                // (only its mask) -- but for THIS split's direct verification
                // we only need candidate-20's own known completion to appear
                // among survivors; the y-block's *specific* order only matters
                // for reconstructing an arbitrary survivor's full 47-digit
                // value. Since gcd(49,L_c)=1 and y < 49^NY < L_c, r.u
                // uniquely determines the ordered y digits; recover them by
                // inverting u_y = B^12 * y mod L_c.
                u128 B12modLc = powmod_u128((u128)B, 12, Lc);
                u128 B12inv = modinv_u128(B12modLc, Lc);
                u128 y_val = mulmod_u128(r.u, B12inv, Lc);
                std::vector<int> ydigitsOrdered(NY);
                u128 yv = y_val;
                for (int i = 0; i < NY; i++) { ydigitsOrdered[i] = (int)(yv % (u128)B); yv /= (u128)B; }

                std::vector<int> freePos1to19(19);
                u128 lv = leaf;
                for (int i = 0; i < 12; i++) { freePos1to19[i] = (int)(lv % (u128)B); lv /= (u128)B; }
                for (int i = 0; i < NY; i++) freePos1to19[12 + i] = ydigitsOrdered[i];
                for (int i = 0; i < NX; i++) freePos1to19[12 + NY + i] = xdigits[i];
                bool ok = verifySurvivorDirect(c, br, c.prefix, 20, freePos1to19);
                if (ok) verifiedOk++; else verifiedBad++;

                if (r.yMask == knownYmask && xmask == knownXmask &&
                    decodedMask == knownLeafmask && thisIsKnownX) {
                    foundKnown = true;
                }
            });
    });

    fprintf(stderr, "[bgate] x permutations=%llu, |Y|=%llu, roots=%llu rootsRejected=%llu lookups=%llu scans=%llu "
                    "maskPasses=%llu survivors=%llu peakRSS=%ldKB\n",
            (unsigned long long)xCounted, (unsigned long long)yCount,
            (unsigned long long)stats.roots, (unsigned long long)stats.rootsRejected,
            (unsigned long long)stats.lookups, (unsigned long long)stats.scans,
            (unsigned long long)stats.maskPasses, (unsigned long long)stats.survivors, peakRssKB());
    fprintf(stderr, "[bgate] direct-arithmetic verification of survivors: %llu OK, %llu FAILED\n",
            (unsigned long long)verifiedOk, (unsigned long long)verifiedBad);
    fprintf(stderr, "[bgate] known completion found among survivors: %s\n", foundKnown ? "YES" : "NO");
    bool pass = foundKnown && (verifiedBad == 0) && (stats.survivors == verifiedOk) && (stats.survivors == 1);
    fprintf(stderr, "[bgate] SOUNDNESS GATE: %s\n", pass ? "PASS" : "FAIL");
    checkRssBudget(rssBudgetKB, "bgate-walk-end");
}

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
    u64 rootsTotal = 0, rootsRejected = 0;
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

        // Root envelope sieve (LIFT-ENVELOPE-SIEVE.md §3/§4), same test as the
        // classic path, applied before ever entering the compact trie.
        LeafEnvelope env{};
        if (SIEVE_LEVEL & 1) env = leafEnvelope(allowedMinusX, 12);

        WalkStats stats;
        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            if (SIEVE_LEVEL & 1) {
                rootsTotal++;
                if (!liftCouldContainLeaf(W, Lc, env)) { rootsRejected++; continue; }
            }
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
    if (SIEVE_LEVEL & 1) {
        double pct = rootsTotal ? 100.0 * (double)rootsRejected / (double)rootsTotal : 0.0;
        fprintf(stderr, "[patfull][sieve] roots total=%llu rejected=%llu (%.1f%%)\n",
                (unsigned long long)rootsTotal, (unsigned long long)rootsRejected, pct);
    }
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
    u64 rootsTotal = 0, rootsRejected = 0;
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

        LeafEnvelope env{};
        if (SIEVE_LEVEL & 1) env = leafEnvelope(allowedMinusX, 12);

        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            if (SIEVE_LEVEL & 1) {
                rootsTotal++;
                if (!liftCouldContainLeaf(W, Lc, env)) { rootsRejected++; continue; }
            }
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
    if (SIEVE_LEVEL & 1) {
        double pct = rootsTotal ? 100.0 * (double)rootsRejected / (double)rootsTotal : 0.0;
        fprintf(stderr, "[pgate][sieve] roots total=%llu rejected=%llu (%.1f%%)\n",
                (unsigned long long)rootsTotal, (unsigned long long)rootsRejected, pct);
    }
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
    if (SIEVE_LEVEL & 1) {
        double pct = rr.rootsTotal ? 100.0 * (double)rr.rootsRejected / (double)rr.rootsTotal : 0.0;
        fprintf(stderr, "[sieve] roots total=%llu rejected=%llu (%.1f%%)\n",
                (unsigned long long)rr.rootsTotal, (unsigned long long)rr.rootsRejected, pct);
    }
}

// ================= Base-48 auto-configured bucket join =================
//
// A fourth, fully independent A/B arm: the same shallow-bucket-join method
// validated above at base 49, retargeted at base 48's own certificate. Every
// numeric parameter is DERIVED from base parameters at runtime -- nothing is
// copied from NILPOTENT-PEELING.md except the branch decomposition itself
// (fixed descending prefix; candidate 21 = known-NO branch, candidate 20 =
// known-YES branch), per the task brief. In particular the suffix-digit
// congruence condition, the forced-reservation of digit 24, and the 34/35
// admissible-suffix-triple counts are all DISCOVERED here by direct
// computation/search, not asserted from the doc -- they are only compared
// against the doc's stated 34/35 as an external cross-check after the fact.
namespace b48 {

static const int B48 = 48;

struct Constants48 {
    std::vector<int> D;         // {1..47} \ {16,32,46}
    u128 L;
    u128 Lnil;                  // 216, given as a base parameter (2^3 * 3^3)
    u128 Lc;
    int T = 0;                  // suffix length: minimal T with B48^T % Lnil == 0
    int Pc = 0;                 // leaf horizon: minimal P with B48^P >= Lc
    int lifts = 0;
    std::vector<int> prefix;    // fixed descending prefix, MSB..LSB (high to low)
};

static Constants48 deriveConstants48() {
    Constants48 c;
    for (int d = 1; d <= 47; d++) if (d != 16 && d != 32 && d != 46) c.D.push_back(d);
    if (c.D.size() != 44) { fprintf(stderr, "FATAL b48: |D|=%zu != 44\n", c.D.size()); exit(1); }

    u128 L = 1;
    for (int d : c.D) L = lcm_u128(L, (u128)d);
    c.L = L;
    // Guard-rail cross-check against the doc's independently stated value.
    {
        const char *lit = "110680160865928453800";
        u128 v = 0; for (const char *p = lit; *p; p++) v = v * 10 + (u128)(*p - '0');
        if (c.L != v) { fprintf(stderr, "FATAL b48: L mismatch: computed=%s expected=%s\n",
                                 u128_to_string(c.L).c_str(), u128_to_string(v).c_str()); exit(1); }
    }
    fprintf(stderr, "[b48 guard] L = %s (assert OK)\n", u128_to_string(c.L).c_str());

    c.Lnil = 216; // given base parameter: Lambda_nil = 2^3 * 3^3
    if (c.L % c.Lnil != 0) { fprintf(stderr, "FATAL b48: Lnil does not divide L\n"); exit(1); }
    c.Lc = c.L / c.Lnil;
    {
        const char *lit = "512408152157076175";
        u128 v = 0; for (const char *p = lit; *p; p++) v = v * 10 + (u128)(*p - '0');
        if (c.Lc != v) { fprintf(stderr, "FATAL b48: Lc mismatch: computed=%s expected=%s\n",
                                  u128_to_string(c.Lc).c_str(), u128_to_string(v).c_str()); exit(1); }
    }
    fprintf(stderr, "[b48 guard] Lc = %s (assert OK)\n", u128_to_string(c.Lc).c_str());

    // gcd(Lnil, Lc) must be 1 for the CRT split (mod Lnil, mod Lc) to be
    // equivalent to (mod L); derive and check, don't assume.
    if (gcd_u128(c.Lnil, c.Lc) != 1) { fprintf(stderr, "FATAL b48: gcd(Lnil,Lc) != 1\n"); exit(1); }
    fprintf(stderr, "[b48 guard] gcd(Lnil, Lc) = 1 (assert OK)\n");

    // gcd(Lc mod B48, B48) == 1, needed for B48 to be invertible mod Lc.
    u64 g = std::gcd((u64)(c.Lc % (u128)B48), (u64)B48);
    if (g != 1) { fprintf(stderr, "FATAL b48: gcd(Lc mod B48, B48) = %llu != 1\n", (unsigned long long)g); exit(1); }
    fprintf(stderr, "[b48 guard] gcd(Lc mod %d, %d) = 1 (assert OK)\n", B48, B48);

    // T: minimal T with B48^T === 0 (mod Lnil) -- beyond this many low digits,
    // higher digit positions cannot affect N mod Lnil at all.
    { u128 p = 1; int T = 0; do { T++; p = (p * (u128)B48) % c.Lnil; } while (p != 0 && T < 10);
      if (p != 0) { fprintf(stderr, "FATAL b48: B48^T never reaches 0 mod Lnil\n"); exit(1); }
      c.T = T; }
    fprintf(stderr, "[b48 guard] T (suffix length, derived) = %d\n", c.T);

    // Pc: minimal P with B48^P >= Lc.
    { u128 v = 1; int P = 0; while (v < c.Lc) { v *= (u128)B48; P++; }
      c.Pc = P;
      u128 lifts128 = (v + c.Lc - 1) / c.Lc; // ceil(B48^Pc / Lc)
      c.lifts = (int)lifts128; }
    fprintf(stderr, "[b48 guard] Pc (leaf horizon, derived) = %d, lifts = %d\n", c.Pc, c.lifts);

    return c;
}

// Adaptively determine the fixed descending prefix for a given candidate
// digit. Starts from the naive maximal descending run (47 downto 22, skipping
// D's own exclusions), then -- if that leaves ZERO admissible suffix triples
// -- searches for which single prefix digit must be reserved (held out of the
// prefix, added back to the pool) to make the suffix congruence solvable,
// trying candidates closest to the boundary first (mirrors the doc's own
// language: "the maximal descending branch that consumes it is immediately
// impossible"). This *discovers* the digit-24 reservation rather than
// assuming it.
static int countAdmissibleSuffixTriples(const std::vector<int> &pool, int T, u128 Lnil,
                                         std::vector<std::array<int,3>> *outTriples = nullptr) {
    if (T != 3) { fprintf(stderr, "FATAL b48: suffix-triple search hardcodes T==3\n"); exit(1); }
    int cnt = 0;
    for (int d0 : pool) {
        u128 p0 = (u128)d0;
        for (int d1 : pool) {
            if (d1 == d0) continue;
            u128 p1 = p0 + (u128)d1 * (u128)B48;
            for (int d2 : pool) {
                if (d2 == d0 || d2 == d1) continue;
                u128 val = p1 + (u128)d2 * (u128)B48 * (u128)B48;
                if (val % Lnil == 0) {
                    cnt++;
                    if (outTriples) outTriples->push_back({d0, d1, d2});
                }
            }
        }
    }
    return cnt;
}

static std::vector<int> findPrefix(const Constants48 &c, int candidate) {
    std::vector<int> naivePrefix;
    for (int d = 47; d >= 22; d--) {
        if (std::find(c.D.begin(), c.D.end(), d) == c.D.end()) continue; // D-excluded
        naivePrefix.push_back(d);
    }
    auto poolFor = [&](const std::vector<int> &prefix) {
        std::vector<int> pool;
        for (int d : c.D) {
            if (d == candidate) continue;
            if (std::find(prefix.begin(), prefix.end(), d) != prefix.end()) continue;
            pool.push_back(d);
        }
        return pool;
    };

    int naiveCount = countAdmissibleSuffixTriples(poolFor(naivePrefix), c.T, c.Lnil);
    if (naiveCount > 0) {
        fprintf(stderr, "[b48] candidate=%d: naive maximal descending prefix already admits %d suffix triples\n",
                candidate, naiveCount);
        return naivePrefix;
    }

    fprintf(stderr, "[b48] candidate=%d: naive prefix admits 0 suffix triples; searching for a digit to reserve...\n",
            candidate);
    // Try reserving each prefix digit, closest to the candidate (low end)
    // first, since that's structurally where the doc's own reasoning places
    // the obstruction.
    for (auto it = naivePrefix.rbegin(); it != naivePrefix.rend(); ++it) {
        int reserve = *it;
        std::vector<int> trial;
        for (int d : naivePrefix) if (d != reserve) trial.push_back(d);
        int cnt = countAdmissibleSuffixTriples(poolFor(trial), c.T, c.Lnil);
        if (cnt > 0) {
            fprintf(stderr, "[b48] candidate=%d: reserving digit %d from the prefix admits %d suffix triples\n",
                    candidate, reserve, cnt);
            return trial;
        }
    }
    fprintf(stderr, "[b48] FATAL: no single-digit prefix reservation makes the suffix congruence solvable\n");
    exit(1);
}

// ---- Shallow-bucket machinery, reimplemented locally for B48/Pc so the
// base-49 arms above are untouched. Structurally identical to the validated
// base-49 bucket join (SHALLOW-RADIX-BUCKET-JOIN.md Phase A): flat
// (u_y, y_mask) records, counting-sort into B48^K buckets, root envelope
// sieve, exact direct leaf decode. ----

static std::array<uint8_t, 16> digitsLSDof48(u128 n, int ndig) {
    std::array<uint8_t, 16> out{};
    for (int i = 0; i < ndig; i++) { out[i] = (uint8_t)(n % (u128)B48); n /= (u128)B48; }
    return out;
}

struct LeafEnvelope48 { u128 lo, hi; };
static LeafEnvelope48 leafEnvelope48(uint64_t available, int count) {
    int digs[20]; int nd = 0;
    uint64_t av = available;
    while (av) { digs[nd++] = __builtin_ctzll(av); av &= av - 1; }
    LeafEnvelope48 out{0, 0};
    for (int i = 0; i < count; i++) out.lo = out.lo * (u128)B48 + (u128)digs[i];
    for (int i = 0; i < count; i++) out.hi = out.hi * (u128)B48 + (u128)digs[nd - 1 - i];
    return out;
}
static inline bool envelopeIntersects48(u128 W, u128 leaf_lo, u128 leaf_hi, u128 domLo, u128 domHi) {
    if (W < leaf_lo) return false;
    u128 hiU = W - leaf_lo;
    u128 loU = (W > leaf_hi) ? (W - leaf_hi) : (u128)0;
    if (hiU < domLo) return false;
    if (loU > domHi) return false;
    return true;
}

struct BucketRecord48 { u128 u; uint64_t yMask; };
struct BucketIndex48 {
    std::vector<uint32_t> offsets;
    std::vector<BucketRecord48> records;
    int K = 3;
    u64 bucketCount = 0;
};

// One admissible suffix triple's induced branch: fixed prefix+candidate+
// suffix, leaving `pool17` (freeblock digits) for the coprime-window search.
struct SuffixBranch {
    std::array<int,3> suffix; // (d0,d1,d2), positions 0,1,2
    std::vector<int> freeDigits; // pool minus suffix, size = Pc + (NX+NY)
    u128 T_target;   // target residue for the freeblock's own value, mod Lc
};

static SuffixBranch buildSuffixBranch(const Constants48 &c, const std::vector<int> &prefix,
                                       int candidate, const std::vector<int> &pool,
                                       const std::array<int,3> &suffix) {
    SuffixBranch sb;
    sb.suffix = suffix;
    for (int d : pool) {
        if (d == suffix[0] || d == suffix[1] || d == suffix[2]) continue;
        sb.freeDigits.push_back(d);
    }
    u128 Lc = c.Lc;
    u128 C = 0;
    // prefix digits occupy positions 43 downto 21 (MSB-first in `prefix`)
    int pos = 20 + (int)prefix.size();
    for (int dd : prefix) {
        u128 term = mulmod_u128((u128)dd, powmod_u128((u128)B48, (u128)pos, Lc), Lc);
        C = (C + term) % Lc;
        pos--;
    }
    C = (C + mulmod_u128((u128)candidate, powmod_u128((u128)B48, 20, Lc), Lc)) % Lc;
    u128 S = (u128)suffix[0] + (u128)suffix[1] * (u128)B48 + (u128)suffix[2] * (u128)B48 * (u128)B48;
    C = (C + S) % Lc;

    u128 BTinv = modinv_u128(powmod_u128((u128)B48, (u128)c.T, Lc), Lc);
    u128 negC = (Lc - (C % Lc)) % Lc;
    sb.T_target = mulmod_u128(negC, BTinv, Lc);
    return sb;
}

// Builds the bucket index for one suffix branch's coprime-window search,
// with Pc/NY/K all explicit.
static BucketIndex48 buildBucketIndex48Real(const std::vector<int> &A, int NY, int K, int Pc, u128 Lc,
                                             u64 &yCount, double &genSeconds, double &sortSeconds,
                                             long rssBudgetKB = -1) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    u128 BPcModLc = powmod_u128((u128)B48, (u128)Pc, Lc);
    u64 bucketCount = 1;
    for (int i = 0; i < K; i++) bucketCount *= (u64)B48;
    u128 bucketMod = (u128)bucketCount;

    std::vector<BucketRecord48> raw;
    std::vector<uint32_t> key;
    std::vector<uint32_t> counts(bucketCount + 1, 0);

    forEachPermutation(A, NY, [&](const std::vector<int> &ydigits) {
        u128 y_val = 0, Bpow = 1;
        for (int i = 0; i < NY; i++) { y_val += (u128)ydigits[i] * Bpow; Bpow *= (u128)B48; }
        u128 u_y = mulmod_u128(BPcModLc, y_val % Lc, Lc);
        uint64_t m = 0; for (int d : ydigits) m |= bit(d);
        raw.push_back({u_y, m});
        uint32_t k = (uint32_t)(u_y % bucketMod);
        key.push_back(k);
        counts[k + 1]++;
    });
    yCount = raw.size();
    auto t1 = clock::now();
    genSeconds = std::chrono::duration<double>(t1 - t0).count();
    checkRssBudget(rssBudgetKB, "b48-bucket-post-generation");

    for (u64 i = 0; i < bucketCount; i++) counts[i + 1] += counts[i];
    BucketIndex48 idx;
    idx.K = K; idx.bucketCount = bucketCount;
    idx.offsets = counts;
    idx.records.resize(raw.size());
    std::vector<uint32_t> cursor(idx.offsets.begin(), idx.offsets.end());
    for (size_t i = 0; i < raw.size(); i++) {
        uint32_t k = key[i];
        idx.records[cursor[k]++] = raw[i];
    }
    auto t2 = clock::now();
    sortSeconds = std::chrono::duration<double>(t2 - t1).count();
    checkRssBudget(rssBudgetKB, "b48-bucket-post-build");
    return idx;
}

template <typename CB>
static void enumLeafPrefix48(int depth, int K, int borrow, uint64_t used, uint64_t key, uint64_t keyMul,
                              const std::array<uint8_t,16> &Wdig, uint64_t allowed, CB &&cb) {
    if (depth == K) { cb(key, used); return; }
    uint64_t rem = allowed & ~used;
    while (rem) {
        int l = __builtin_ctzll(rem);
        rem &= rem - 1;
        int raw = (int)Wdig[depth] - borrow - l;
        int u_i, nb;
        if (raw < 0) { u_i = raw + B48; nb = 1; } else { u_i = raw; nb = 0; }
        enumLeafPrefix48(depth + 1, K, nb, used | bit(l), key + (uint64_t)u_i * keyMul, keyMul * (uint64_t)B48,
                         Wdig, allowed, cb);
    }
}

static inline bool decodeDistinctLeaf48(u128 leaf, uint64_t allowed, int Pc, uint64_t &outMask) {
    uint64_t mask = 0;
    for (int i = 0; i < Pc; i++) {
        int d = (int)(leaf % (u128)B48);
        leaf /= (u128)B48;
        if (d == 0) return false;
        uint64_t db = bit(d);
        if (!(allowed & db)) return false;
        if (mask & db) return false;
        mask |= db;
    }
    outMask = mask;
    return true;
}

struct BucketStats48 {
    u64 roots = 0, rootsRejected = 0, lookups = 0, scans = 0, maskPasses = 0, survivors = 0;
};

template <typename OnSurvivor>
static void bucketSearchX48(const BucketIndex48 &idx, u128 c_x, u128 Lc, int lifts, int Pc,
                             uint64_t xmask, uint64_t Amask, int K, BucketStats48 &stats,
                             const std::array<u128,16> &Bpow, OnSurvivor &&onSurvivor) {
    uint64_t allowed = Amask & ~xmask;
    LeafEnvelope48 env{};
    if (SIEVE_LEVEL & 1) env = leafEnvelope48(allowed, Pc);

    for (int j = 0; j < lifts; j++) {
        u128 W = c_x + (u128)j * Lc;
        if (SIEVE_LEVEL & 1) {
            stats.roots++;
            if (W < env.lo) { stats.rootsRejected++; continue; }
            u128 minReq = (W > env.hi) ? (W - env.hi) : (u128)0;
            if (minReq >= Lc) { stats.rootsRejected++; continue; }
        } else {
            stats.roots++;
        }
        auto Wdig = digitsLSDof48(W, K > Pc ? K : Pc);
        enumLeafPrefix48(0, K, 0, 0, 0, 1, Wdig, allowed,
            [&](uint64_t key, uint64_t lowLeafMask) {
                stats.lookups++;
                uint32_t lo = idx.offsets[key], hi = idx.offsets[key + 1];
                for (uint32_t p = lo; p < hi; p++) {
                    stats.scans++;
                    const BucketRecord48 &r = idx.records[p];
                    if (r.yMask & (xmask | lowLeafMask)) continue;
                    if (W < r.u) continue;
                    u128 leaf = W - r.u;
                    if (leaf >= Bpow[Pc]) continue;
                    stats.maskPasses++;
                    uint64_t decodedMask;
                    if (!decodeDistinctLeaf48(leaf, allowed, Pc, decodedMask)) continue;
                    uint64_t required = Amask & ~xmask & ~r.yMask;
                    if (decodedMask != required) continue;
                    stats.survivors++;
                    onSurvivor(r, leaf, decodedMask);
                }
            });
    }
}

// Direct verification: reconstruct the full 44-digit number and check
// N mod L == 0, independent of the bucket-join machinery (mirrors
// verifySurvivorDirect for base 49).
static bool verifySurvivorDirect48(const Constants48 &c, const std::vector<int> &prefix, int candidate,
                                    const std::array<int,3> &suffix,
                                    const std::vector<int> &freeDigitsMSBfirst /* upper..leaf, MSD..LSD */) {
    std::vector<int> digitsMSBfirst;
    for (int dd : prefix) digitsMSBfirst.push_back(dd);
    digitsMSBfirst.push_back(candidate);
    for (int d : freeDigitsMSBfirst) digitsMSBfirst.push_back(d);
    digitsMSBfirst.push_back(suffix[2]);
    digitsMSBfirst.push_back(suffix[1]);
    digitsMSBfirst.push_back(suffix[0]);
    if ((int)digitsMSBfirst.size() != (int)c.D.size()) {
        fprintf(stderr, "FATAL b48 verify: digit count %zu != %zu\n", digitsMSBfirst.size(), c.D.size());
        exit(1);
    }
    u128 acc = 0;
    for (int d : digitsMSBfirst) {
        acc = mulmod_u128(acc, (u128)B48, c.L);
        acc = (acc + (u128)d) % c.L;
    }
    return acc == 0;
}

// a(48) is the ordinary INTEGER whose base-48 positional value is the digit
// sequence (sum d_i * 48^i), same convention as verifySurvivorDirect48's
// "mod L" check and as base49's known reconstructed value in
// NILPOTENT-PEELING.md -- NOT a concatenation of decimal digit labels (44
// digits, mixed 1- and 2-digit labels, would give 79 decimal characters;
// the true base-48 value gives 74, matching the target). Minimal base-1e9
// bignum: Horner's method, multiply-by-48-add-digit, MSD to LSD.
static std::string reconstructDecimal48(const std::vector<int> &prefix, int candidate,
                                         const std::array<int,3> &suffix,
                                         const std::vector<int> &freeDigitsMSBfirst) {
    std::vector<int> digitsMSBfirst;
    for (int d : prefix) digitsMSBfirst.push_back(d);
    digitsMSBfirst.push_back(candidate);
    for (int d : freeDigitsMSBfirst) digitsMSBfirst.push_back(d);
    digitsMSBfirst.push_back(suffix[2]);
    digitsMSBfirst.push_back(suffix[1]);
    digitsMSBfirst.push_back(suffix[0]);

    // Base-1,000,000,000 limbs, little-endian (limb 0 = least significant).
    std::vector<uint32_t> limbs(1, 0);
    const uint64_t BASE = 1000000000ULL;
    for (int d : digitsMSBfirst) {
        uint64_t carry = (uint64_t)d;
        for (size_t i = 0; i < limbs.size(); i++) {
            uint64_t v = (uint64_t)limbs[i] * 48ULL + carry;
            limbs[i] = (uint32_t)(v % BASE);
            carry = v / BASE;
        }
        while (carry) { limbs.push_back((uint32_t)(carry % BASE)); carry /= BASE; }
    }
    // Strip leading (most-significant) zero limbs.
    while (limbs.size() > 1 && limbs.back() == 0) limbs.pop_back();
    std::string s = std::to_string(limbs.back());
    for (int i = (int)limbs.size() - 2; i >= 0; i--) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%09u", limbs[i]);
        s += buf;
    }
    return s;
}

static void runBranch(const Constants48 &c, int candidate, int NX, int NY, int K, long rssBudgetKB,
                       const char *expectedDecimalOrNull) {
    using clock = std::chrono::steady_clock;
    auto tBranch0 = clock::now();

    std::vector<int> prefix = findPrefix(c, candidate);
    std::vector<int> pool;
    for (int d : c.D) {
        if (d == candidate) continue;
        if (std::find(prefix.begin(), prefix.end(), d) != prefix.end()) continue;
        pool.push_back(d);
    }
    std::vector<std::array<int,3>> triples;
    int nTriples = countAdmissibleSuffixTriples(pool, c.T, c.Lnil, &triples);
    fprintf(stderr, "[b48] candidate=%d: prefix len=%zu, pool size=%zu, admissible suffix triples=%d "
                    "(doc cross-check: expect ~34/35)\n",
            candidate, prefix.size(), pool.size(), nTriples);

    std::array<u128,16> Bpow{}; { u128 p = 1; for (int i = 0; i < 16; i++) { Bpow[i] = p; p *= (u128)B48; } }

    u64 totalSurvivors = 0, totalRoots = 0, totalRootsRejected = 0, totalLookups = 0, totalScans = 0;
    u64 verifiedOk = 0, verifiedBad = 0;
    std::string foundDecimal;
    double totalGen = 0, totalSort = 0, totalSearch = 0;

    for (size_t ti = 0; ti < triples.size(); ti++) {
        const auto &suffix = triples[ti];
        SuffixBranch sb = buildSuffixBranch(c, prefix, candidate, pool, suffix);
        u64 freeCount = sb.freeDigits.size();
        if ((int)freeCount != c.Pc + NX + NY) {
            fprintf(stderr, "FATAL b48: freeDigits size %llu != Pc+NX+NY=%d\n",
                    (unsigned long long)freeCount, c.Pc + NX + NY);
            exit(1);
        }
        uint64_t Amask = 0; for (int d : sb.freeDigits) Amask |= bit(d);

        auto t0 = clock::now();
        u64 yCount = 0; double genS = 0, sortS = 0;
        BucketIndex48 idx = buildBucketIndex48Real(sb.freeDigits, NY, K, c.Pc, c.Lc, yCount, genS, sortS, rssBudgetKB);
        totalGen += genS; totalSort += sortS;

        u128 Bexp = powmod_u128((u128)B48, (u128)(c.Pc + NY), c.Lc);
        BucketStats48 stats;
        auto tsearch0 = clock::now();
        forEachPermutation(sb.freeDigits, NX, [&](const std::vector<int> &xdigits) {
            u128 x_val = 0, Bpow_local = 1;
            for (int i = 0; i < NX; i++) { x_val += (u128)xdigits[i] * Bpow_local; Bpow_local *= (u128)B48; }
            u128 term = mulmod_u128(Bexp, x_val % c.Lc, c.Lc);
            u128 c_x = (sb.T_target + c.Lc - (term % c.Lc)) % c.Lc;
            uint64_t xmask = 0; for (int d : xdigits) xmask |= bit(d);

            bucketSearchX48(idx, c_x, c.Lc, c.lifts, c.Pc, xmask, Amask, K, stats, Bpow,
                [&](const BucketRecord48 &r, u128 leaf, uint64_t decodedMask) {
                    // Reconstruct: leaf (Pc digits, LSD-first) + x (NX digits) +
                    // y (NY digits, recovered from r.u via modular inverse) +
                    // suffix + candidate + prefix, direct-verify mod ORIGINAL L.
                    std::vector<int> leafDigits(c.Pc);
                    u128 lv = leaf;
                    for (int i = 0; i < c.Pc; i++) { leafDigits[i] = (int)(lv % (u128)B48); lv /= (u128)B48; }
                    u128 BPcModLc = powmod_u128((u128)B48, (u128)c.Pc, c.Lc);
                    u128 BPcInv = modinv_u128(BPcModLc, c.Lc);
                    u128 y_val = mulmod_u128(r.u, BPcInv, c.Lc);
                    std::vector<int> ydigits(NY);
                    u128 yv = y_val;
                    for (int i = 0; i < NY; i++) { ydigits[i] = (int)(yv % (u128)B48); yv /= (u128)B48; }

                    // freeDigitsMSBfirst = [x MSD..LSD][y MSD..LSD][leaf MSD..LSD]
                    std::vector<int> freeMSB;
                    for (int i = NX - 1; i >= 0; i--) freeMSB.push_back(xdigits[i]);
                    for (int i = NY - 1; i >= 0; i--) freeMSB.push_back(ydigits[i]);
                    for (int i = c.Pc - 1; i >= 0; i--) freeMSB.push_back(leafDigits[i]);

                    bool ok = verifySurvivorDirect48(c, prefix, candidate, suffix, freeMSB);
                    if (ok) verifiedOk++; else verifiedBad++;
                    if (ok) {
                        // The branch's own leading digits (fixed descending
                        // prefix, same across every suffix triple) make every
                        // valid completion the same decimal-digit LENGTH, so
                        // plain string comparison is a safe/correct maximum.
                        std::string dec = reconstructDecimal48(prefix, candidate, suffix, freeMSB);
                        if (foundDecimal.empty() || dec.size() > foundDecimal.size() ||
                            (dec.size() == foundDecimal.size() && dec > foundDecimal)) {
                            foundDecimal = dec;
                        }
                    }
                });
        });
        auto tsearch1 = clock::now();
        totalSearch += std::chrono::duration<double>(tsearch1 - tsearch0).count();

        totalSurvivors += stats.survivors;
        totalRoots += stats.roots; totalRootsRejected += stats.rootsRejected;
        totalLookups += stats.lookups; totalScans += stats.scans;

        checkRssBudget(rssBudgetKB, "b48-per-suffix-triple");
    }

    auto tBranch1 = clock::now();
    double branchWall = std::chrono::duration<double>(tBranch1 - tBranch0).count();

    fprintf(stderr, "[b48branch] candidate=%d: %d suffix triples, total roots=%llu rejected=%llu (%.1f%%) "
                    "lookups=%llu scans=%llu survivors=%llu\n",
            candidate, nTriples, (unsigned long long)totalRoots, (unsigned long long)totalRootsRejected,
            totalRoots ? 100.0 * totalRootsRejected / totalRoots : 0.0,
            (unsigned long long)totalLookups, (unsigned long long)totalScans, (unsigned long long)totalSurvivors);
    fprintf(stderr, "[b48branch] candidate=%d: gen=%.3fs sort=%.3fs search=%.3fs total wall=%.3fs peakRSS=%ldKB\n",
            candidate, totalGen, totalSort, totalSearch, branchWall, peakRssKB());
    fprintf(stderr, "[b48branch] candidate=%d: direct-arithmetic verification: %llu OK, %llu FAILED\n",
            candidate, (unsigned long long)verifiedOk, (unsigned long long)verifiedBad);

    if (candidate == 21) {
        bool pass = (totalSurvivors == 0) && (verifiedBad == 0);
        fprintf(stderr, "[b48branch] candidate=21 SOUNDNESS GATE (expect 0 survivors): %s\n", pass ? "PASS" : "FAIL");
    } else {
        // The branch can (and did) contain multiple independently-valid
        // completions across its 35 suffix triples -- every one of them is a
        // real solution to the peeled congruence, direct-verified against the
        // ORIGINAL L. The hard gate is that the MAXIMUM among them (a(48) is
        // defined as the largest such number) matches the lead's stated known
        // target exactly, not that the branch is uniquely single-valued.
        bool foundExact = expectedDecimalOrNull && foundDecimal == expectedDecimalOrNull;
        fprintf(stderr, "[b48branch] candidate=20: %llu total survivors across all suffix triples "
                        "(all %llu direct-verified valid, i.e. genuine solutions of the peeled congruence)\n",
                (unsigned long long)totalSurvivors, (unsigned long long)verifiedOk);
        fprintf(stderr, "[b48branch] candidate=20: MAXIMUM reconstructed decimal (%zu digits): %s\n",
                foundDecimal.size(), foundDecimal.empty() ? "(none found)" : foundDecimal.c_str());
        bool pass = (totalSurvivors >= 1) && (verifiedBad == 0) && foundExact;
        fprintf(stderr, "[b48branch] candidate=20 SOUNDNESS GATE (max survivor decimal == known target): %s\n",
                pass ? "PASS" : "FAIL");
    }
}

} // namespace b48

// ================= Generalized certificate driver (mode "cert <base>") =================
//
// Fully autonomous generalization of the b48 driver above: no branch or
// divergence hints for ANY base. Two genuinely-derived pieces make this
// possible:
//
//  1. Lnil/Lc split needs no hint: Lnil := product over primes p|B of
//     p^(v_p(L)) (the full "B-adic part" of L=lcm(D)); Lc := L/Lnil is then
//     coprime to B by construction (proven, not assumed -- guarded at
//     runtime). T and Pc follow exactly as in the b48 derivation.
//  2. Subset selection (which digits belong in D) and the prefix/window
//     divergence point are both discovered by the SAME mechanism: a
//     wrong-turn refutation search. At the position immediately below the
//     trusted-descending prefix, try the largest available digit; run a
//     full bucket-join search across every admissible suffix family for
//     that choice; zero survivors REFUTES it (that digit can never occupy
//     that position in ANY valid completion) and the search drops to the
//     next-largest digit. If every digit at that position is refuted, the
//     window itself must be widened (equivalent to releasing another
//     prefix digit into the pool) or -- as a last resort -- a digit is
//     dropped from D entirely and the whole search restarts one subset
//     size down, mirroring a113028_v13's own k=nD,nD-1,... descent.
//
// CAVEAT (stated plainly, not just in the final report): the subset-search
// here is a best-effort reproduction of "existing subset machinery," not a
// byte-for-byte port of a113028_v13.c's subset_filters()/build_pps(). It is
// validated against three KNOWN targets (48, 49, 40) before being trusted on
// anything new. Every reported value is direct-u128-verified against the
// original L regardless.
namespace certdrv {

struct ConstantsGen {
    int B = 0;
    std::vector<int> D;
    u128 L = 0, Lnil = 0, Lc = 0;
    int T = 0, Pc = 0, lifts = 0;
    bool ok = false;
};

static std::vector<int> primeFactorsOf(int n) {
    std::vector<int> fs;
    for (int p = 2; (long long)p * p <= n; p++) {
        if (n % p == 0) { fs.push_back(p); while (n % p == 0) n /= p; }
    }
    if (n > 1) fs.push_back(n);
    return fs;
}

static ConstantsGen deriveConstantsGen(int B, const std::vector<int> &D) {
    ConstantsGen c;
    c.B = B; c.D = D;
    u128 L = 1;
    for (int d : D) L = lcm_u128(L, (u128)d);
    c.L = L;

    u128 Lnil = 1;
    for (int p : primeFactorsOf(B)) {
        u128 Lp = L; u128 pk = 1;
        while (Lp % (u128)p == 0) { Lp /= (u128)p; pk *= (u128)p; }
        Lnil *= pk;
    }
    c.Lnil = Lnil;
    if (c.L % c.Lnil != 0) return c; // shouldn't happen; leave ok=false
    c.Lc = c.L / c.Lnil;

    // gcd(Lc mod B, B) == 1 is REQUIRED for the peeling to work (B must be
    // invertible mod Lc); if it fails for this D, this subset is rejected.
    u64 g = std::gcd((u64)(c.Lc % (u128)B), (u64)B);
    if (g != 1) return c; // ok=false

    // T: minimal T with B^T === 0 (mod Lnil).
    { u128 p = 1; int T = 0; bool found = false;
      for (T = 1; T <= 12; T++) { p = (p * (u128)B) % c.Lnil; if (p == 0) { found = true; break; } }
      if (!found) return c; // ok=false
      c.T = T; }

    // Pc: minimal P with B^P >= Lc.
    { u128 v = 1; int P = 0; while (v < c.Lc) { v *= (u128)B; P++; } c.Pc = P; }
    { u128 v = 1; for (int i = 0; i < c.Pc; i++) v *= (u128)B;
      u128 lifts128 = (v + c.Lc - 1) / c.Lc; c.lifts = (int)lifts128; }

    c.ok = true;
    return c;
}

// General ordered T-tuple admissibility count/collection: distinct digits
// from `pool`, weighted sum (positions 0..T-1, LSD-first) === 0 (mod Lnil).
static int countAdmissibleSuffixTuplesGen(const std::vector<int> &pool, int T, int B, u128 Lnil,
                                           std::vector<std::vector<int>> *outTuples = nullptr) {
    std::function<void(int, u128, u128, std::vector<int>&, std::vector<bool>&, int&)> rec =
        [&](int depth, u128 partial, u128 weight, std::vector<int> &cur, std::vector<bool> &used, int &count) {
        if (depth == T) {
            if (partial % Lnil == 0) { count++; if (outTuples) outTuples->push_back(cur); }
            return;
        }
        for (size_t i = 0; i < pool.size(); i++) {
            if (used[i]) continue;
            used[i] = true; cur.push_back(pool[i]);
            rec(depth + 1, partial + (u128)pool[i] * weight, weight * (u128)B, cur, used, count);
            cur.pop_back(); used[i] = false;
        }
    };
    int count = 0;
    std::vector<int> cur; std::vector<bool> used(pool.size(), false);
    rec(0, 0, 1, cur, used, count);
    return count;
}

// One admissible suffix tuple's induced branch (generalizes b48::SuffixBranch
// to arbitrary T).
struct SuffixBranchGen {
    std::vector<int> suffix; // (d0..d_{T-1}), LSD-first
    std::vector<int> freeDigits;
    u128 T_target;
};

static SuffixBranchGen buildSuffixBranchGen(const ConstantsGen &c, const std::vector<int> &prefix,
                                             int candidate, const std::vector<int> &pool,
                                             const std::vector<int> &suffix) {
    SuffixBranchGen sb;
    sb.suffix = suffix;
    for (int d : pool) {
        if (std::find(suffix.begin(), suffix.end(), d) != suffix.end()) continue;
        sb.freeDigits.push_back(d);
    }
    u128 Lc = c.Lc;
    u128 C = 0;
    int pos = 20 + (int)prefix.size(); // candidate fixed at position 20, same convention as b48
    for (int dd : prefix) {
        u128 term = mulmod_u128((u128)dd, powmod_u128((u128)c.B, (u128)pos, Lc), Lc);
        C = (C + term) % Lc;
        pos--;
    }
    C = (C + mulmod_u128((u128)candidate, powmod_u128((u128)c.B, 20, Lc), Lc)) % Lc;
    u128 S = 0, w = 1;
    for (int d : suffix) { S = (S + (u128)d * w) % Lc; w *= (u128)c.B; } // Lc-reduced is fine, S<Lc always since S<B^T<=Lc*B typically; safe mod
    C = (C + S) % Lc;

    u128 BTinv = modinv_u128(powmod_u128((u128)c.B, (u128)c.T, Lc), Lc);
    u128 negC = (Lc - (C % Lc)) % Lc;
    sb.T_target = mulmod_u128(negC, BTinv, Lc);
    return sb;
}

// Runtime-B versions of the bucket-join primitives (B is a runtime field of
// ConstantsGen, not a compile-time constant -- kept fully separate from both
// the base-49 arms and the b48 namespace).
static std::array<uint8_t, 16> digitsLSDofGen(u128 n, int B, int ndig) {
    std::array<uint8_t, 16> out{};
    for (int i = 0; i < ndig; i++) { out[i] = (uint8_t)(n % (u128)B); n /= (u128)B; }
    return out;
}
struct LeafEnvelopeGen { u128 lo, hi; };
static LeafEnvelopeGen leafEnvelopeGen(uint64_t available, int B, int count) {
    int digs[24]; int nd = 0;
    uint64_t av = available;
    while (av) { digs[nd++] = __builtin_ctzll(av); av &= av - 1; }
    LeafEnvelopeGen out{0, 0};
    for (int i = 0; i < count; i++) out.lo = out.lo * (u128)B + (u128)digs[i];
    for (int i = 0; i < count; i++) out.hi = out.hi * (u128)B + (u128)digs[nd - 1 - i];
    return out;
}
struct BucketRecordGen { u128 u; uint64_t yMask; };
struct BucketIndexGen {
    std::vector<uint32_t> offsets;
    std::vector<BucketRecordGen> records;
    int K = 3;
    u64 bucketCount = 0;
};

// ---------- Checked u128 arithmetic (RADIX-BUCKET-ADMISSION-CONTROL.md Sec 3) ----------
// Overflow is a policy signal here ("this configuration cannot be sized"),
// never a silently-wrapped number. All return std::nullopt on overflow.
static std::optional<u128> checkedMulU128(u128 a, u128 b) {
    if (a == 0 || b == 0) return (u128)0;
    static const u128 kU128Max = ~(u128)0;
    if (a > kU128Max / b) return std::nullopt;
    return a * b;
}
// a P n  (falling factorial: a * (a-1) * ... * (a-n+1)), the exact y-arrangement count.
static std::optional<u128> checkedFallingFactorial(int a, int n) {
    if (a < 0 || n < 0 || n > a) return std::nullopt;
    u128 r = 1;
    for (int i = 0; i < n; i++) {
        auto next = checkedMulU128(r, (u128)(a - i));
        if (!next) return std::nullopt;
        r = *next;
    }
    return r;
}
// B^K, the directory bucket count.
static std::optional<u128> checkedPower(u128 base, int exp) {
    if (exp < 0) return std::nullopt;
    u128 r = 1;
    for (int i = 0; i < exp; i++) {
        auto next = checkedMulU128(r, base);
        if (!next) return std::nullopt;
        r = *next;
    }
    return r;
}
// Smallest power of two >= n (vector growth capacity bound). nullopt only if
// that power would exceed 2^127, which cannot happen once n has already
// passed the Y <= UINT32_MAX gate below.
static std::optional<u128> nextPow2U128(u128 n) {
    if (n <= 1) return (u128)1;
    u128 p = 1;
    while (p < n) {
        u128 next = p << 1;
        if (next <= p) return std::nullopt; // overflowed 128 bits
        p = next;
    }
    return p;
}
static std::string formatGiB(u128 bytes) {
    long double gib = (long double)bytes / (1024.0L * 1024.0L * 1024.0L);
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2LfGiB", gib);
    return std::string(buf);
}

// Preflight admission gate for the current grow-and-scatter builder below
// (RADIX-BUCKET-ADMISSION-CONTROL.md Sec 2/3, Sec 12 Phase A). Active only
// when rssBudgetKB > 0; manual/debug call sites that pass -1 keep exactly
// their current behavior. A decline here is resource POLICY, not
// mathematics (Sec 9): it must never be mistaken for a refutation, so it
// terminates the process immediately (exit 3 = BUCKET_DECLINED) with a
// reproducible trace, rather than returning a value the caller (or the
// wrong-turn search above it) could interpret as "no survivors."
static void bucketAdmissionGate(int a, int NY, int K, int B, long rssBudgetKB) {
    if (rssBudgetKB <= 0) return;

    auto yOpt = checkedFallingFactorial(a, NY);
    auto qOpt = checkedPower((u128)B, K);

    long currentRssKB = peakRssKB();
    u128 currentRssBytes = currentRssKB > 0 ? (u128)currentRssKB * 1024 : (u128)0;
    const u128 kReserveBytes = (u128)256 * 1024 * 1024;
    u128 budgetBytes = (u128)rssBudgetKB * 1024;
    u128 available = (budgetBytes > currentRssBytes + kReserveBytes)
                          ? (budgetBytes - currentRssBytes - kReserveBytes)
                          : (u128)0;

    bool declined = false;
    bool haveProjected = false;
    const char *reason = "";
    u128 Y = 0, Q = 0, projected = 0;

    if (!yOpt) { declined = true; reason = "Y_overflow"; }
    else if (!qOpt) { declined = true; reason = "Q_overflow"; }
    else {
        Y = *yOpt; Q = *qOpt;
        if (Y > (u128)UINT32_MAX) { declined = true; reason = "Y_exceeds_uint32"; }
        else {
            auto cOpt = nextPow2U128(Y);
            u128 C = cOpt ? *cOpt : Y;
            u128 recBytes = (u128)sizeof(BucketRecordGen);
            u128 idxBytes = (u128)sizeof(uint32_t);
            // doc Sec 3: C(R+4) + YR + 12(Q+1), using real sizeof rather than
            // the copied constants in the doc's worked example.
            projected = C * (recBytes + idxBytes) + Y * recBytes + (u128)3 * (Q + 1) * idxBytes;
            haveProjected = true;
            u128 need = projected + (projected * 15) / 100; // 1.15x projected
            if (need > available) { declined = true; reason = "insufficient_budget"; }
        }
    }

    std::string yStr = yOpt ? u128_to_string(Y) : std::string("OVERFLOW");
    std::string qStr = qOpt ? u128_to_string(Q) : std::string("OVERFLOW");
    std::string projStr = haveProjected ? formatGiB(projected) : std::string("n/a");
    fprintf(stderr, "[bucket-plan] B=%d a=%d NY=%d K=%d Y=%s Q=%s projectedPeak=%s availableForIndex=%s\n",
            B, a, NY, K, yStr.c_str(), qStr.c_str(), projStr.c_str(), formatGiB(available).c_str());

    if (declined) {
        fprintf(stderr, "[bucket-plan] decision=DECLINE_MEMORY reason=%s fallback=scan (exit 3 = BUCKET_DECLINED)\n", reason);
        exit(3);
    }

    fprintf(stderr, "[bucket-plan] decision=ADMIT\n");
}

static BucketIndexGen buildBucketIndexGen(const std::vector<int> &A, int NY, int K, int B, int Pc, u128 Lc,
                                           u64 &yCount, long rssBudgetKB = -1) {
    bucketAdmissionGate((int)A.size(), NY, K, B, rssBudgetKB);
    u128 BPcModLc = powmod_u128((u128)B, (u128)Pc, Lc);
    u64 bucketCount = 1;
    for (int i = 0; i < K; i++) bucketCount *= (u64)B;
    u128 bucketMod = (u128)bucketCount;

    std::vector<BucketRecordGen> raw;
    std::vector<uint32_t> key;
    std::vector<uint32_t> counts(bucketCount + 1, 0);
    forEachPermutation(A, NY, [&](const std::vector<int> &ydigits) {
        u128 y_val = 0, Bpow = 1;
        for (int i = 0; i < NY; i++) { y_val += (u128)ydigits[i] * Bpow; Bpow *= (u128)B; }
        u128 u_y = mulmod_u128(BPcModLc, y_val % Lc, Lc);
        uint64_t m = 0; for (int d : ydigits) m |= bit(d);
        raw.push_back({u_y, m});
        uint32_t k = (uint32_t)(u_y % bucketMod);
        key.push_back(k);
        counts[k + 1]++;
    });
    yCount = raw.size();
    checkRssBudget(rssBudgetKB, "certdrv-bucket-post-generation");

    for (u64 i = 0; i < bucketCount; i++) counts[i + 1] += counts[i];
    BucketIndexGen idx;
    idx.K = K; idx.bucketCount = bucketCount;
    idx.offsets = counts;
    idx.records.resize(raw.size());
    std::vector<uint32_t> cursor(idx.offsets.begin(), idx.offsets.end());
    for (size_t i = 0; i < raw.size(); i++) { uint32_t k = key[i]; idx.records[cursor[k]++] = raw[i]; }
    checkRssBudget(rssBudgetKB, "certdrv-bucket-post-build");
    return idx;
}

template <typename CB>
static void enumLeafPrefixGen(int depth, int K, int B, int borrow, uint64_t used, uint64_t key, uint64_t keyMul,
                               const std::array<uint8_t,16> &Wdig, uint64_t allowed, CB &&cb) {
    if (depth == K) { cb(key, used); return; }
    uint64_t rem = allowed & ~used;
    while (rem) {
        int l = __builtin_ctzll(rem);
        rem &= rem - 1;
        int raw = (int)Wdig[depth] - borrow - l;
        int u_i, nb;
        if (raw < 0) { u_i = raw + B; nb = 1; } else { u_i = raw; nb = 0; }
        enumLeafPrefixGen(depth + 1, K, B, nb, used | bit(l), key + (uint64_t)u_i * keyMul, keyMul * (uint64_t)B,
                          Wdig, allowed, cb);
    }
}
static inline bool decodeDistinctLeafGen(u128 leaf, uint64_t allowed, int B, int Pc, uint64_t &outMask) {
    uint64_t mask = 0;
    for (int i = 0; i < Pc; i++) {
        int d = (int)(leaf % (u128)B);
        leaf /= (u128)B;
        if (d == 0) return false;
        uint64_t db = bit(d);
        if (!(allowed & db)) return false;
        if (mask & db) return false;
        mask |= db;
    }
    outMask = mask;
    return true;
}
struct BucketStatsGen { u64 roots = 0, rootsRejected = 0, lookups = 0, scans = 0, survivors = 0; };

template <typename OnSurvivor>
static void bucketSearchXGen(const BucketIndexGen &idx, u128 c_x, u128 Lc, int lifts, int B, int Pc,
                              uint64_t xmask, uint64_t Amask, int K, BucketStatsGen &stats,
                              const std::vector<u128> &Bpow, OnSurvivor &&onSurvivor) {
    uint64_t allowed = Amask & ~xmask;
    LeafEnvelopeGen env{};
    if (SIEVE_LEVEL & 1) env = leafEnvelopeGen(allowed, B, Pc);
    for (int j = 0; j < lifts; j++) {
        u128 W = c_x + (u128)j * Lc;
        if (SIEVE_LEVEL & 1) {
            stats.roots++;
            if (W < env.lo) { stats.rootsRejected++; continue; }
            u128 minReq = (W > env.hi) ? (W - env.hi) : (u128)0;
            if (minReq >= Lc) { stats.rootsRejected++; continue; }
        } else stats.roots++;
        auto Wdig = digitsLSDofGen(W, B, K > Pc ? K : Pc);
        enumLeafPrefixGen(0, K, B, 0, 0, 0, 1, Wdig, allowed,
            [&](uint64_t key, uint64_t lowLeafMask) {
                stats.lookups++;
                uint32_t lo = idx.offsets[key], hi = idx.offsets[key + 1];
                for (uint32_t p = lo; p < hi; p++) {
                    stats.scans++;
                    const BucketRecordGen &r = idx.records[p];
                    if (r.yMask & (xmask | lowLeafMask)) continue;
                    if (W < r.u) continue;
                    u128 leaf = W - r.u;
                    if (leaf >= Bpow[Pc]) continue;
                    uint64_t decodedMask;
                    if (!decodeDistinctLeafGen(leaf, allowed, B, Pc, decodedMask)) continue;
                    uint64_t required = Amask & ~xmask & ~r.yMask;
                    if (decodedMask != required) continue;
                    stats.survivors++;
                    onSurvivor(r, leaf, decodedMask);
                }
            });
    }
}

static bool verifySurvivorDirectGen(const ConstantsGen &c, const std::vector<int> &prefix, int candidate,
                                     const std::vector<int> &suffix /*LSD..MSD i.e. suffix[0]=d0*/,
                                     const std::vector<int> &freeDigitsMSBfirst) {
    std::vector<int> digitsMSBfirst;
    for (int dd : prefix) digitsMSBfirst.push_back(dd);
    digitsMSBfirst.push_back(candidate);
    for (int d : freeDigitsMSBfirst) digitsMSBfirst.push_back(d);
    for (int i = (int)suffix.size() - 1; i >= 0; i--) digitsMSBfirst.push_back(suffix[i]);
    if ((int)digitsMSBfirst.size() != (int)c.D.size()) return false;
    // Distinctness + range sanity (defense in depth, per the lead's explicit
    // "sanity-verify divisibility+distinctness in-code" instruction).
    std::vector<bool> seen(c.B, false);
    for (int d : digitsMSBfirst) {
        if (d <= 0 || d >= c.B) return false;
        if (seen[d]) return false;
        seen[d] = true;
    }
    u128 acc = 0;
    for (int d : digitsMSBfirst) { acc = mulmod_u128(acc, (u128)c.B, c.L); acc = (acc + (u128)d) % c.L; }
    return acc == 0;
}

static std::string reconstructDecimalGen(int B, const std::vector<int> &prefix, int candidate,
                                          const std::vector<int> &suffix,
                                          const std::vector<int> &freeDigitsMSBfirst) {
    std::vector<int> digitsMSBfirst;
    for (int d : prefix) digitsMSBfirst.push_back(d);
    digitsMSBfirst.push_back(candidate);
    for (int d : freeDigitsMSBfirst) digitsMSBfirst.push_back(d);
    for (int i = (int)suffix.size() - 1; i >= 0; i--) digitsMSBfirst.push_back(suffix[i]);

    std::vector<uint32_t> limbs(1, 0);
    const uint64_t BASE = 1000000000ULL;
    for (int d : digitsMSBfirst) {
        uint64_t carry = (uint64_t)d;
        for (size_t i = 0; i < limbs.size(); i++) {
            uint64_t v = (uint64_t)limbs[i] * (uint64_t)B + carry;
            limbs[i] = (uint32_t)(v % BASE); carry = v / BASE;
        }
        while (carry) { limbs.push_back((uint32_t)(carry % BASE)); carry /= BASE; }
    }
    while (limbs.size() > 1 && limbs.back() == 0) limbs.pop_back();
    std::string s = std::to_string(limbs.back());
    for (int i = (int)limbs.size() - 2; i >= 0; i--) { char buf[16]; snprintf(buf, sizeof(buf), "%09u", limbs[i]); s += buf; }
    return s;
}

// Result of attempting one specific (D, prefix-with-reservations) hypothesis.
struct AttemptResult {
    bool success = false;
    int wrongTurnsRefuted = 0;
    u64 totalSurvivors = 0;
    u64 verifiedOk = 0, verifiedBad = 0;
    std::string maxDecimal;
    int winningCandidate = -1;
    double wallSeconds = 0;
    long peakRssKBSeen = 0;
};

// The core wrong-turn-refutation search: given a fully-specified D, a
// derived ConstantsGen, and a prefix, try candidate digits at the
// window-top position in descending order. Each refuted candidate is a
// "wrong turn"; the first with survivors wins (max taken across all its
// admissible suffix families).
static AttemptResult runWrongTurnSearch(const ConstantsGen &c, const std::vector<int> &prefix,
                                         const std::vector<int> &pool, int NX, int NY, int K,
                                         long rssBudgetKB) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    AttemptResult res;

    std::vector<int> candidates = pool; // try largest-first
    std::sort(candidates.begin(), candidates.end(), std::greater<int>());

    std::vector<u128> Bpow(c.Pc + 1, 1);
    for (int i = 1; i <= c.Pc; i++) Bpow[i] = Bpow[i-1] * (u128)c.B;

    for (int candidate : candidates) {
        std::vector<int> restPool;
        for (int d : pool) if (d != candidate) restPool.push_back(d);

        std::vector<std::vector<int>> tuples;
        int nTuples = countAdmissibleSuffixTuplesGen(restPool, c.T, c.B, c.Lnil, &tuples);
        if (nTuples == 0) continue; // this candidate can't even form a suffix; not a real refutation, just infeasible bookkeeping -- skip silently to next-largest

        u64 totalSurvivors = 0, verifiedOk = 0, verifiedBad = 0;
        std::string bestDecimal;

        for (auto &suffix : tuples) {
            SuffixBranchGen sb = buildSuffixBranchGen(c, prefix, candidate, restPool, suffix);
            if ((int)sb.freeDigits.size() != c.Pc + NX + NY) continue; // shouldn't happen; skip defensively

            uint64_t Amask = 0; for (int d : sb.freeDigits) Amask |= bit(d);
            u64 yCount = 0;
            BucketIndexGen idx = buildBucketIndexGen(sb.freeDigits, NY, K, c.B, c.Pc, c.Lc, yCount, rssBudgetKB);
            u128 Bexp = powmod_u128((u128)c.B, (u128)(c.Pc + NY), c.Lc);
            BucketStatsGen stats;
            forEachPermutation(sb.freeDigits, NX, [&](const std::vector<int> &xdigits) {
                u128 x_val = 0, Bp = 1;
                for (int i = 0; i < NX; i++) { x_val += (u128)xdigits[i] * Bp; Bp *= (u128)c.B; }
                u128 term = mulmod_u128(Bexp, x_val % c.Lc, c.Lc);
                u128 c_x = (sb.T_target + c.Lc - (term % c.Lc)) % c.Lc;
                uint64_t xmask = 0; for (int d : xdigits) xmask |= bit(d);
                bucketSearchXGen(idx, c_x, c.Lc, c.lifts, c.B, c.Pc, xmask, Amask, K, stats, Bpow,
                    [&](const BucketRecordGen &r, u128 leaf, uint64_t decodedMask) {
                        std::vector<int> leafDigits(c.Pc);
                        u128 lv = leaf;
                        for (int i = 0; i < c.Pc; i++) { leafDigits[i] = (int)(lv % (u128)c.B); lv /= (u128)c.B; }
                        u128 BPcModLc = powmod_u128((u128)c.B, (u128)c.Pc, c.Lc);
                        u128 BPcInv = modinv_u128(BPcModLc, c.Lc);
                        u128 y_val = mulmod_u128(r.u, BPcInv, c.Lc);
                        std::vector<int> ydigits(NY);
                        u128 yv = y_val;
                        for (int i = 0; i < NY; i++) { ydigits[i] = (int)(yv % (u128)c.B); yv /= (u128)c.B; }

                        std::vector<int> freeMSB;
                        for (int i = NX - 1; i >= 0; i--) freeMSB.push_back(xdigits[i]);
                        for (int i = NY - 1; i >= 0; i--) freeMSB.push_back(ydigits[i]);
                        for (int i = c.Pc - 1; i >= 0; i--) freeMSB.push_back(leafDigits[i]);

                        bool ok = verifySurvivorDirectGen(c, prefix, candidate, suffix, freeMSB);
                        if (ok) verifiedOk++; else verifiedBad++;
                        if (ok) {
                            std::string dec = reconstructDecimalGen(c.B, prefix, candidate, suffix, freeMSB);
                            if (bestDecimal.empty() || dec.size() > bestDecimal.size() ||
                                (dec.size() == bestDecimal.size() && dec > bestDecimal))
                                bestDecimal = dec;
                        }
                    });
            });
            totalSurvivors += stats.survivors;
            checkRssBudget(rssBudgetKB, "certdrv-per-suffix-tuple");
            long kb = peakRssKB(); if (kb > res.peakRssKBSeen) res.peakRssKBSeen = kb;
        }

        if (totalSurvivors == 0) {
            res.wrongTurnsRefuted++;
            continue; // refuted: this digit can never occupy the window-top position
        }

        // Survivors found: this candidate wins. Take the max, verify, done.
        res.success = true;
        res.totalSurvivors = totalSurvivors;
        res.verifiedOk = verifiedOk; res.verifiedBad = verifiedBad;
        res.maxDecimal = bestDecimal;
        res.winningCandidate = candidate;
        break;
    }

    auto t1 = clock::now();
    res.wallSeconds = std::chrono::duration<double>(t1 - t0).count();
    return res;
}

// Adaptive prefix construction for a candidate D (mirrors b48::findPrefix,
// generalized): start from the maximal descending run down to a window
// boundary sized for Pc+targetWY+1(candidate), release digits one at a time
// (closest to the boundary first) until the resulting pool admits at least
// one admissible suffix tuple AT ALL (a cheap necessary pre-check before the
// expensive wrong-turn search).
static bool buildFeasiblePrefix(const ConstantsGen &c, int targetWY, int maxReleases,
                                 std::vector<int> &outPrefix, std::vector<int> &outPool) {
    int nD = (int)c.D.size();
    int windowSize = c.T + c.Pc + targetWY + 1; // +1 for the candidate position
    if (windowSize >= nD) return false;
    std::vector<int> sortedDesc = c.D;
    std::sort(sortedDesc.begin(), sortedDesc.end(), std::greater<int>());
    int naivePrefixLen = nD - windowSize;
    std::vector<int> naivePrefix(sortedDesc.begin(), sortedDesc.begin() + naivePrefixLen);
    // naivePool sorted descending too -- its FRONT is the boundary-adjacent
    // (largest) pool digits, used below as the natural promotion candidates.
    std::vector<int> naivePool(sortedDesc.begin() + naivePrefixLen, sortedDesc.end());

    auto poolFor = [&](const std::vector<int> &prefix) {
        std::vector<int> pool;
        std::vector<bool> inPrefix(c.B, false);
        for (int d : prefix) inPrefix[d] = true;
        for (int d : c.D) if (!inPrefix[d]) pool.push_back(d);
        return pool;
    };

    if (countAdmissibleSuffixTuplesGen(poolFor(naivePrefix), c.T, c.B, c.Lnil) > 0) {
        outPrefix = naivePrefix; outPool = poolFor(naivePrefix); return true;
    }
    // Naive (no reservation) prefix is infeasible. Try releasing INDIVIDUAL
    // prefix digits (not necessarily contiguous/trailing) back into the
    // pool -- but pool.size() MUST stay exactly windowSize (runWrongTurnSearch's
    // buildSuffixBranchGen hard-requires freeDigits.size()==Pc+NX+NY, i.e.
    // pool.size()==windowSize, via its fixed pos=20-style convention). So
    // every release of r prefix digits is paired with a compensating
    // PROMOTION of the r boundary-adjacent (largest) pool digits into the
    // prefix, keeping both sizes exactly fixed -- a swap, not a plain
    // shrink. Search order over which r prefix digits to release is
    // unchanged (standard ascending-index combination enumeration; npref is
    // at most a few dozen, cheap regardless of order).
    int npref = (int)naivePrefix.size();
    for (int releases = 1; releases <= maxReleases && releases <= npref && releases <= (int)naivePool.size(); releases++) {
        std::vector<int> promoted(naivePool.begin(), naivePool.begin() + releases);
        std::vector<int> comb(releases);
        for (int i = 0; i < releases; i++) comb[i] = i;
        while (true) {
            std::vector<bool> release(npref, false);
            for (int idx : comb) release[idx] = true;
            std::vector<int> trial;
            for (int i = 0; i < npref; i++) if (!release[i]) trial.push_back(naivePrefix[i]);
            for (int d : promoted) trial.push_back(d);
            auto pool = poolFor(trial);
            if (countAdmissibleSuffixTuplesGen(pool, c.T, c.B, c.Lnil) > 0) {
                outPrefix = trial; outPool = pool; return true;
            }
            int i = releases - 1;
            while (i >= 0 && comb[i] == npref - releases + i) i--;
            if (i < 0) break;
            comb[i]++;
            for (int j = i + 1; j < releases; j++) comb[j] = comb[j-1] + 1;
        }
    }
    return false;
}

// ---------------------------------------------------------------------
// subsetdisc: subset-DISCOVERY engine ported from a113028_v13.c's
// solve_base architecture (descending-lex combination enumeration over
// digit subsets, gated by build_pps' ten-rule + subset_filters' digit-sum
// q1 / D1-existence / e2 / order-e(3..6) CRT partition-feasibility DPs).
// This REPLACES the old brute-force drop-k discovery in runCert: instead
// of trying drops=0,1,2,3 digit-drop combinations in a heuristic
// prime-power-weighted order, subsets of {1..B-1} are enumerated in
// EXACTLY the order solve_base uses -- descending k (subset size), then
// within a fixed k, descending-lex over which k digits are kept (S[i] =
// B-1-c[i] with c[] the standard ascending combination of indices). Any
// subset surviving the cheap filters below is handed to the EXISTING,
// UNCHANGED certdrv wrong-turn-refutation search (runWrongTurnSearch);
// the first subset (in this order) whose search proves a certified
// completion is the answer. This reproduces solve_base's validated
// (47-base) behavior: within a fixed k the enumeration visits subsets in
// non-increasing "best possible arrangement" order, so the first success
// at a given k is provably that k's maximum, and k itself is tried
// largest-first, so the very first success found anywhere is the global
// maximum.
namespace subsetdisc {

constexpr int MAXB2 = 64;
constexpr int OE_MAXSTATES2 = 2000000;

struct PPsub { u64 q; int is_nil; int t; int e; u64 q1, q2; u64 qe[7]; };

// Mirrors v13's file-scope subset state: single-threaded, one subset
// evaluated at a time, exactly as solve_base uses it.
static int    SB_B = 0;
static int    SB_S[MAXB2];
static int    SB_k = 0;
static u128   SB_LCM = 1;
static u64    SB_setmask = 0;
static u64    SB_D1 = 1;
static PPsub  SB_pps[32];
static int    SB_npps = 0;
static const u128 SB_TARGET_ = 0; // subset-level filters always target 0

static uint8_t SB_oe_cur[OE_MAXSTATES2], SB_oe_nxt[OE_MAXSTATES2];

static int SB_vp(int n, int p) { int v = 0; while (n % p == 0) { v++; n /= p; } return v; }

// Ten-rule ported verbatim from build_pps(): rejects the subset iff its
// digit-set LCM is divisible by B (LCM % B == 0).
static int SB_build_pps() {
    SB_npps = 0; SB_D1 = 1; SB_LCM = 1;
    for (int p = 2; p < SB_B; p++) {
        int isp = 1; for (int d = 2; d * d <= p; d++) if (p % d == 0) { isp = 0; break; }
        if (!isp) continue;
        int a = 0;
        for (int i = 0; i < SB_k; i++) { int v = SB_vp(SB_S[i], p); if (v > a) a = v; }
        if (a == 0) continue;
        u64 q = 1; for (int i = 0; i < a; i++) q *= (u64)p;
        u128 nl = SB_LCM * (u128)q;
        if (nl / q != SB_LCM) return 0; // overflow guard; not expected at these bases
        SB_LCM = nl;
        PPsub *pp = &SB_pps[SB_npps++]; pp->q = q;
        int vB = SB_vp(SB_B, p);
        if (vB > 0) { pp->is_nil = 1; pp->t = (a + vB - 1) / vB; if (pp->t == 1) SB_D1 *= q; }
        else {
            pp->is_nil = 0;
            u64 r = (u64)SB_B % q; int e = 1;
            while (r != 1 && e <= 2) { r = (r * ((u64)SB_B % q)) % q; e++; }
            pp->e = (r == 1) ? e : 3;
            u64 q1 = 1, q2 = 1, pb = 1;
            u64 qe3 = 1, qe4 = 1, qe5 = 1, qe6 = 1;
            for (int b = 1; b <= a; b++) {
                pb *= (u64)p;
                if (((u64)SB_B - 1) % pb == 0) q1 = pb;
                u64 Bm = (u64)SB_B % pb;
                u64 p2 = (Bm * Bm) % pb;
                if (p2 == 1 % pb) q2 = pb;
                u64 p3 = (p2 * Bm) % pb;
                if (p3 == 1 % pb) qe3 = pb;
                u64 p4 = (p2 * p2) % pb;
                if (p4 == 1 % pb) qe4 = pb;
                u64 p5 = (p4 * Bm) % pb;
                if (p5 == 1 % pb) qe5 = pb;
                u64 p6 = (p3 * p3) % pb;
                if (p6 == 1 % pb) qe6 = pb;
            }
            pp->q1 = q1; pp->q2 = q2;
            pp->qe[3] = qe3; pp->qe[4] = qe4; pp->qe[5] = qe5; pp->qe[6] = qe6;
        }
    }
    if ((u64)(SB_LCM % (unsigned)SB_B) == 0) return 0; // ten rule
    return 1;
}

// Order-2 CRT partition-feasibility DP, ported verbatim from e2_check().
static int SB_e2_check(u64 q, u64 avail, int m, u64 r) {
    int co = m / 2;
    u64 Bq = (u64)SB_B % q;
    u64 bm = (m & 1) ? Bq : 1;
    u64 target = ((u64)(SB_TARGET_ % q) + q - (r * bm) % q) % q;
    u64 R = 0;
    for (int d = 1; d < SB_B; d++) if (avail >> d & 1) R = (R + d) % q;
    u64 Bm1 = ((u64)SB_B - 1) % q;
    u64 need = (target + q - R) % q;
    u64 dp[MAXB2 / 2 + 2]; memset(dp, 0, sizeof(u64) * (co + 2));
    dp[0] = 1;
    u64 full = (q >= 64) ? ~0ULL : ((1ULL << q) - 1);
    int seen = 0;
    for (int d = SB_B - 1; d >= 1; d--) {
        if (!(avail >> d & 1)) continue;
        seen++;
        int dd = d % (int)q;
        int hi = (seen < co) ? seen : co;
        for (int c = hi; c >= 1; c--) {
            u64 mm = dp[c - 1];
            dp[c] |= dd ? (((mm << dd) | (mm >> (q - dd))) & full) : mm;
        }
    }
    u64 mask = dp[co];
    while (mask) {
        int so = __builtin_ctzll(mask); mask &= mask - 1;
        if ((Bm1 * (u64)so) % q == need) return 1;
    }
    return 0;
}

// Order-e (e=3..6) CRT partition-feasibility DP, ported verbatim from
// order_e_check_m().
static int SB_order_e_check_m(u64 q, int e, u64 avail, int m, u64 target) {
    if (q <= 1) return 1;
    int c[8]; for (int j = 0; j < e; j++) c[j] = 0;
    for (int i = 0; i < m; i++) c[i % e]++;
    u64 w[8]; { u64 wv = 1 % q; u64 Bq = (u64)SB_B % q; for (int j = 0; j < e; j++) { w[j] = wv; wv = (wv * Bq) % q; } }
    u64 dims[8]; for (int j = 0; j < e - 1; j++) dims[j] = (u64)(c[j] + 1);
    u64 stride[8]; u64 M = 1;
    for (int j = 0; j < e - 1; j++) { stride[j] = M; M *= dims[j]; }
    u64 total = M * q;
    if (total == 0 || total > OE_MAXSTATES2) return 1;
    memset(SB_oe_cur, 0, (size_t)total);
    SB_oe_cur[0] = 1;
    uint8_t *cur = SB_oe_cur, *nxt = SB_oe_nxt;
    int processed = 0;
    for (int d = 1; d < SB_B; d++) {
        if (!(avail >> d & 1ULL)) continue;
        memset(nxt, 0, (size_t)total);
        u64 dm = (u64)d % q;
        for (u64 idx = 0; idx < total; idx++) {
            if (!cur[idx]) continue;
            u64 s = idx / M, rem = idx % M;
            int nv[8], sumtracked = 0;
            for (int j = 0; j < e - 1; j++) { nv[j] = (int)((rem / stride[j]) % dims[j]); sumtracked += nv[j]; }
            for (int j = 0; j < e - 1; j++) {
                if (nv[j] < c[j]) {
                    u64 nrem = rem + stride[j];
                    u64 ns = (s + w[j] * dm) % q;
                    nxt[ns * M + nrem] = 1;
                }
            }
            int implied = processed - sumtracked;
            if (implied < c[e - 1]) {
                u64 ns = (s + w[e - 1] * dm) % q;
                nxt[ns * M + rem] = 1;
            }
        }
        uint8_t *t = cur; cur = nxt; nxt = t;
        processed++;
    }
    u64 tt = target % q;
    return cur[tt * M + (M - 1)] ? 1 : 0;
}

static int SB_order_e_check(u64 q, int e) {
    return SB_order_e_check_m(q, e, SB_setmask, SB_k, 0);
}

// Ported verbatim from subset_filters().
static int SB_subset_filters() {
    u64 T = 0; for (int i = 0; i < SB_k; i++) T += SB_S[i];
    for (int j = 0; j < SB_npps; j++) {
        if (SB_pps[j].is_nil) continue;
        if (SB_pps[j].q1 > 1 && (T % SB_pps[j].q1)) return 0;
    }
    if (SB_D1 > 1) {
        int ok = 0; for (int i = 0; i < SB_k; i++) if ((u64)SB_S[i] % SB_D1 == 0) { ok = 1; break; }
        if (!ok) return 0;
    }
    for (int j = 0; j < SB_npps; j++)
        if (!SB_pps[j].is_nil && SB_pps[j].q2 > SB_pps[j].q1)
            if (!SB_e2_check(SB_pps[j].q2, SB_setmask, SB_k, 0)) return 0;
    for (int j = 0; j < SB_npps; j++) {
        if (SB_pps[j].is_nil) continue;
        u64 q1 = SB_pps[j].q1, q2 = SB_pps[j].q2;
        u64 qe3 = SB_pps[j].qe[3], qe4 = SB_pps[j].qe[4], qe5 = SB_pps[j].qe[5], qe6 = SB_pps[j].qe[6];
        if (qe3 > q1)
            if (!SB_order_e_check(qe3, 3)) return 0;
        if (qe4 > q1 && qe4 > q2)
            if (!SB_order_e_check(qe4, 4)) return 0;
        if (qe5 > q1)
            if (!SB_order_e_check(qe5, 5)) return 0;
        if (qe6 > q1 && qe6 > q2 && qe6 > qe3)
            if (!SB_order_e_check(qe6, 6)) return 0;
    }
    return 1;
}

} // namespace subsetdisc

// Top-level: find a working (D, prefix, WY) hypothesis and run the
// wrong-turn search. Subset DISCOVERY is delegated to subsetdisc's
// descending-lex enumeration (ported from a113028_v13.c's solve_base),
// gated by subsetdisc's ten-rule + subset_filters; certification for each
// surviving subset remains the original, unchanged wrong-turn-refutation
// search below.
static void runCert(int B, const char *expectedDecimalOrNull, long rssBudgetKB, double capSeconds1800, double capSeconds5400) {
    using clock = std::chrono::steady_clock;
    auto tAll0 = clock::now();
    int n = B - 1;
    fprintf(stderr, "[cert] base=%d, n=%d, autonomous subset+divergence discovery starting "
                     "(subsetdisc: solve_base-style descending-lex enumeration)\n", B, n);

    // Reasonable NY/K heuristics, matching what won tonight's head-to-head:
    // aim for B^NY in the low millions, K=3.
    // buildSuffixBranchGen hardcodes the candidate at ABSOLUTE digit-position
    // 20 (pos = 20 + prefix.size(); powmod_u128(B, 20, Lc) for the candidate
    // term) -- i.e. it hard-requires exactly 20 digit-positions below the
    // candidate: T (suffix) + Pc (leaf window) + NX + NY (free digits) == 20,
    // always, for every subset. The old "aim for B^NY in the low millions"
    // heuristic chose NY independently of T/Pc and only coincidentally
    // matched this invariant for some subsets (e.g. base 48's T=3,Pc=11);
    // for others (e.g. base 49's T=1,Pc=12) it did not, silently producing
    // wrong position arithmetic and false refutations. NY is now DERIVED
    // from the invariant directly, with NX shrunk first if T+Pc alone
    // already leaves little room.
    const int CANDIDATE_POS = 20;
    auto chooseWY = [&](const ConstantsGen &c) -> std::pair<int,int> {
        int NX = 2;
        int NY = CANDIDATE_POS - c.T - c.Pc - NX;
        if (NY < 1) { NX = 1; NY = CANDIDATE_POS - c.T - c.Pc - NX; }
        if (NY < 1) { NX = 0; NY = CANDIDATE_POS - c.T - c.Pc - NX; }
        return std::make_pair(NX, NY);
    };

    subsetdisc::SB_B = B;
    long long subsetsChecked = 0, subsetsScanned = 0;

    // solve_base's exact enumeration: k = subset size, descending from n
    // (full alphabet) down to 1; for each k, combinations c[0..k-1] of
    // indices in {0..n-1} in standard ascending-lex order, mapped to
    // digit-subset S[i] = (B-1) - c[i] (so S starts as the top-k digits
    // and, as c[] advances, trends toward smaller digits -- descending-lex
    // over kept-digit subsets). First subset surviving the cheap filters
    // whose wrong-turn search yields a certified completion wins; k is
    // tried largest-first so the first success anywhere is the global max.
    for (int k = n; k >= 1; k--) {
        subsetdisc::SB_k = k;
        std::vector<int> comb(k);
        for (int i = 0; i < k; i++) comb[i] = i;
        bool done = false;
        while (!done) {
            for (int i = 0; i < k; i++) subsetdisc::SB_S[i] = (B - 1) - comb[i];
            subsetsChecked++;
            subsetdisc::SB_setmask = 0;
            for (int i = 0; i < k; i++) subsetdisc::SB_setmask |= 1ULL << subsetdisc::SB_S[i];

            if (subsetdisc::SB_build_pps() && subsetdisc::SB_subset_filters()) {
                subsetsScanned++;
                std::vector<int> D(subsetdisc::SB_S, subsetdisc::SB_S + k);

                if (getenv("DEBUG_SUBSETDISC")) {
                    std::vector<bool> present(B, false);
                    for (int d : D) present[d] = true;
                    fprintf(stderr, "[dbg] k=%d dropped=", k);
                    for (int d = 1; d < B; d++) if (!present[d]) fprintf(stderr, "%d,", d);
                    fprintf(stderr, "\n");
                }

                ConstantsGen c = deriveConstantsGen(B, D);
                if (c.ok && c.T + c.Pc < CANDIDATE_POS) {
                    auto [NX, NY] = chooseWY(c);
                    std::vector<int> prefix, pool;
                    bool feas = buildFeasiblePrefix(c, NX + NY, 3, prefix, pool);
                    if (getenv("DEBUG_SUBSETDISC") && !feas) fprintf(stderr, "[dbg]   buildFeasiblePrefix FAILED (T=%d Pc=%d NX=%d NY=%d)\n", c.T, c.Pc, NX, NY);
                    if (feas) {
                        AttemptResult ar = runWrongTurnSearch(c, prefix, pool, NX, NY, 3, rssBudgetKB);
                        if (getenv("DEBUG_SUBSETDISC")) fprintf(stderr, "[dbg]   success=%d winCand=%d refuted=%d survivors=%llu verifiedOk=%llu verifiedBad=%llu prefixLen=%zu poolLen=%zu T=%d Pc=%d NX=%d NY=%d\n",
                            ar.success, ar.winningCandidate, ar.wrongTurnsRefuted, (unsigned long long)ar.totalSurvivors,
                            (unsigned long long)ar.verifiedOk, (unsigned long long)ar.verifiedBad, prefix.size(), pool.size(), c.T, c.Pc, NX, NY);

                        double elapsed = std::chrono::duration<double>(clock::now() - tAll0).count();
                        if (elapsed > capSeconds1800 && elapsed < capSeconds1800 + 5) {
                            fprintf(stderr, "[cert] checkpoint at %.0fs: still searching (k=%d, subsetsChecked=%lld, "
                                            "subsetsScanned=%lld)\n", elapsed, k, subsetsChecked, subsetsScanned);
                        }
                        if (elapsed > capSeconds5400) {
                            fprintf(stderr, "[cert] FATAL: exceeded %.0fs cap, aborting search\n", capSeconds5400);
                            return;
                        }

                        // A "certified completion" (per the task's own phrasing) means
                        // ar.success (survivors found) AND at least one of them
                        // direct-arithmetic-verified true, with none verified false.
                        // ar.success alone is NOT sufficient: runWrongTurnSearch sets
                        // it as soon as totalSurvivors>0, even if every survivor is
                        // later refuted by direct verification (a spurious/false-
                        // positive residue match) -- that is NOT a certified
                        // completion, so treat it the same as a refutation and fall
                        // through to the next surviving subset instead of reporting a
                        // bogus (empty-decimal) FOUND.
                        bool certified = ar.success && ar.verifiedOk >= 1 && ar.verifiedBad == 0;
                        if (ar.success && !certified && getenv("DEBUG_SUBSETDISC")) {
                            fprintf(stderr, "[cert] base=%d: subset |D|=%d had %llu survivor(s) but NONE "
                                            "direct-verified true (verified %llu OK / %llu FAILED) -- not a "
                                            "certified completion, continuing\n",
                                    B, k, (unsigned long long)ar.totalSurvivors,
                                    (unsigned long long)ar.verifiedOk, (unsigned long long)ar.verifiedBad);
                        }
                        if (certified) {
                            fprintf(stderr, "[cert] base=%d: FOUND with |D|=%d (dropped %d digit(s)) prefixLen=%zu "
                                            "poolSize=%zu winningCandidate=%d wrongTurnsRefuted=%d survivors=%llu "
                                            "(verified %llu OK / %llu FAILED) wall=%.3fs peakRSS~%ldKB "
                                            "[subsetsChecked=%lld subsetsScanned=%lld]\n",
                                    B, k, n - k, prefix.size(), pool.size(), ar.winningCandidate,
                                    ar.wrongTurnsRefuted, (unsigned long long)ar.totalSurvivors,
                                    (unsigned long long)ar.verifiedOk, (unsigned long long)ar.verifiedBad,
                                    ar.wallSeconds, ar.peakRssKBSeen, subsetsChecked, subsetsScanned);
                            fprintf(stderr, "[cert] base=%d: maximum value (%zu digits): %s\n",
                                    B, ar.maxDecimal.size(), ar.maxDecimal.c_str());

                            bool exact = expectedDecimalOrNull && ar.maxDecimal == expectedDecimalOrNull;
                            if (expectedDecimalOrNull) {
                                fprintf(stderr, "[cert] base=%d CERTIFICATION (matches known target): %s\n",
                                        B, exact ? "PASS" : "FAIL");
                            } else {
                                fprintf(stderr, "[cert] base=%d CERTIFICATION (direct-verified, no external target "
                                                "to compare): PASS\n", B);
                            }
                            double totalWall = std::chrono::duration<double>(clock::now() - tAll0).count();
                            fprintf(stderr, "[cert] base=%d total autonomous-search wall=%.3fs\n", B, totalWall);
                            return;
                        }
                        // Certifier proved (via exhaustive wrong-turn refutation over
                        // this subset's window-top candidates, or via direct
                        // verification rejecting every survivor) that this subset has
                        // no certified completion reachable from this prefix
                        // construction: fall through to the next surviving subset.
                    }
                }
            }

            // Advance comb[] to the next k-combination (standard ascending-lex
            // next-combination), matching solve_base's c[] update exactly.
            int i = k - 1;
            while (i >= 0 && comb[i] == n - k + i) i--;
            if (i < 0) { done = true; }
            else { comb[i]++; for (int j = i + 1; j < k; j++) comb[j] = comb[j - 1] + 1; }
        }
    }
    fprintf(stderr, "[cert] base=%d: FAILED to find a working (subset, prefix) hypothesis within bounded search "
                    "[subsetsChecked=%lld subsetsScanned=%lld]\n", B, subsetsChecked, subsetsScanned);
}

} // namespace certdrv

int main(int argc, char **argv) {
    initBpow12();
    Constants c = deriveConstants();
    printGuardSummary(c);

    // SIEVE_LEVEL (LIFT-ENVELOPE-SIEVE.md) is read from the SIEVE env var so it
    // doesn't have to be threaded through every mode's positional CLI:
    //   SIEVE=0 off (default) | 1 root only | 2 root+every-node | 4 +depth3-joint
    //   (bitmask; e.g. SIEVE=7 = root + every-node + depth3-joint combined)
    if (const char *sv = getenv("SIEVE")) SIEVE_LEVEL = atoi(sv);
    fprintf(stderr, "[sieve] SIEVE_LEVEL=%d (bit0=root bit1=every-node bit2=depth3-joint)\n", SIEVE_LEVEL);

    if (argc < 2) {
        fprintf(stderr, "usage: %s smoke|gate|full|pgate|pfull|bgate|bfull <cand> <NX> <NY> [pruneLevel]\n", argv[0]);
        fprintf(stderr, "  bgate/bfull: SHALLOW-RADIX-BUCKET-JOIN.md Phase A shallow-bucket builder/searcher\n");
        fprintf(stderr, "  pgate/pfull: PATRICIA-CARRY-TRIE.md Phase A path-compressed builder/walker\n");
        fprintf(stderr, "  pruneLevel: 0=none 1=rung1(mask-union) 2=+rung2a(p1) 3=+rung2b(p2) [default 3]\n");
        fprintf(stderr, "  SIEVE env var: LIFT-ENVELOPE-SIEVE.md bitmask, see above\n");
        return 1;
    }
    std::string mode = argv[1];
    if (mode == "smoke") {
        if (argc >= 3) PRUNE_LEVEL = atoi(argv[2]);
        fprintf(stderr, "[prune] level=%d\n", PRUNE_LEVEL);
        runSmoke(c);
    } else if (mode == "gate") {
        // gate [NX] [NY] [pruneLevel]   (default 3 4, for back-compat)
        int NX = 3, NY = 4;
        if (argc >= 4) { NX = atoi(argv[2]); NY = atoi(argv[3]); }
        if (argc >= 5) PRUNE_LEVEL = atoi(argv[4]);
        fprintf(stderr, "[prune] level=%d\n", PRUNE_LEVEL);
        runGate(c, NX, NY);
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
    } else if (mode == "bgate") {
        // bgate [NX] [NY] [K] [rssBudgetKB]   (default 2 5 3, per doc's measured optimum)
        int NX = 2, NY = 5, K = 3;
        if (argc >= 4) { NX = atoi(argv[2]); NY = atoi(argv[3]); }
        if (argc >= 5) K = atoi(argv[4]);
        long rssBudgetKB = -1;
        if (argc >= 6) rssBudgetKB = atol(argv[5]);
        fprintf(stderr, "[bucket] K=%d rssBudgetKB=%ld\n", K, rssBudgetKB);
        runBucketGate(c, NX, NY, K, rssBudgetKB);
    } else if (mode == "bfull") {
        // bfull <cand> <NX> <NY> [K] [rssBudgetKB]
        if (argc < 5) { fprintf(stderr, "bfull requires <cand> <NX> <NY>\n"); return 1; }
        int cand = atoi(argv[2]);
        int NX = atoi(argv[3]);
        int NY = atoi(argv[4]);
        int K = 3;
        if (argc >= 6) K = atoi(argv[5]);
        long rssBudgetKB = -1;
        if (argc >= 7) rssBudgetKB = atol(argv[6]);
        fprintf(stderr, "[bucket] K=%d rssBudgetKB=%ld\n", K, rssBudgetKB);
        runBucketFull(c, cand, NX, NY, K, rssBudgetKB);
    } else if (mode == "b48run") {
        // b48run <cand:20|21> [NX] [NY] [K] [rssBudgetKB]
        if (argc < 3) { fprintf(stderr, "b48run requires <cand:20|21>\n"); return 1; }
        int cand = atoi(argv[2]);
        int NX = 2, NY = 4, K = 3;
        if (argc >= 5) { NX = atoi(argv[3]); NY = atoi(argv[4]); }
        if (argc >= 6) K = atoi(argv[5]);
        long rssBudgetKB = -1;
        if (argc >= 7) rssBudgetKB = atol(argv[6]);
        fprintf(stderr, "[b48] NX=%d NY=%d K=%d rssBudgetKB=%ld\n", NX, NY, K, rssBudgetKB);
        // Reference-only cross-check target for candidate 20 (the lead's
        // stated known-YES value); used ONLY post-hoc to compare the found
        // survivor's reconstructed decimal string -- never fed into the
        // search/arithmetic itself.
        static const char *B48_KNOWN_20_DECIMAL =
            "94237804886307950779486130179671488973571078333724597158459950718126090200";
        b48::Constants48 bc = b48::deriveConstants48();
        b48::runBranch(bc, cand, NX, NY, K, rssBudgetKB, cand == 20 ? B48_KNOWN_20_DECIMAL : nullptr);
    } else if (mode == "cert") {
        // cert <base> [expectedDecimal] [rssBudgetKB]
        if (argc < 3) { fprintf(stderr, "cert requires <base>\n"); return 1; }
        int base = atoi(argv[2]);
        const char *expected = (argc >= 4 && std::string(argv[3]) != "-") ? argv[3] : nullptr;
        long rssBudgetKB = -1;
        if (argc >= 5) rssBudgetKB = atol(argv[4]);
        fprintf(stderr, "[cert] base=%d rssBudgetKB=%ld expected=%s\n", base, rssBudgetKB, expected ? expected : "(none)");
        certdrv::runCert(base, expected, rssBudgetKB, 1800.0, 5400.0);
    } else {
        fprintf(stderr, "unknown mode %s\n", mode.c_str());
        return 1;
    }
    return 0;
}
