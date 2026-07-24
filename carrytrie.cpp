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
#include <ctime>
#include <cerrno>
#include <cassert>
#include <unistd.h>
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

// C1 (Fable review, MASK-WIDENING-128BIT.md): overflow-CHECKED lcm
// accumulation. A plain lcm_u128 wrap is a silent mod-2^128 value that can
// land ANYWHERE in [1,2^128) -- post-hoc range inspection of the result
// cannot detect it. This checks for overflow AT the multiplication step
// (the same (a/g)*b lcm_u128 performs), before it happens, and never lets a
// wrapped value survive to be inspected.
static bool lcmStepOverflows_u128(u128 L, u128 d, u128 *outNewL) {
    u128 g = gcd_u128(L, d);
    u128 a = L / g;
    static const u128 kU128Max = ~(u128)0;
    if (d != 0 && a > kU128Max / d) return true;
    *outNewL = a * d;
    return false;
}

// Startup guard (review C1): compute checked lcm(1..maxDigit) ONCE. Since
// every subset D subsetdisc ever searches is a SUBSET of {1..B-1}, and
// lcm(subset) always divides (hence is <=) lcm(full range), this single
// full-range check upper-bounds every subsequent per-subset lcm_u128 call
// -- no per-subset re-checking is needed. Fatal-exits with a clear message
// on overflow (bases ~89+); NEVER returns a wrapped value.
static u128 checkedLcmUpToOrDie(int maxDigit) {
    u128 L = 1;
    for (int d = 1; d <= maxDigit; d++) {
        u128 next;
        if (lcmStepOverflows_u128(L, (u128)d, &next)) {
            fprintf(stderr,
                "FATAL: arithmetic ceiling exceeded -- bases >=~89 need the u256 epic "
                "(checked lcm(1..%d) overflows unsigned __int128 at d=%d; L so far=%s)\n",
                maxDigit, d, u128_to_string(L).c_str());
            exit(1);
        }
        L = next;
    }
    return L;
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

// Widened to unsigned __int128 (MASK-WIDENING-128BIT.md Sec 2/3): a single
// shared bit(d) is called from every arm (fixed-B49, fixed-B48, patricia,
// and the runtime-B Gen/FM certauto arms). The fixed-B arms only ever call
// this with d<49/48 (dead region above bit 63 for them; their local mask
// variables stay uint64_t and the implicit narrowing on assignment is a
// no-op there -- see MASK-WIDENING-128BIT.md report for the audit). The
// Gen/FM arms (certauto, arbitrary B) are the ones that actually need
// d up to 127; assert(d<128) catches any B>128 caller before UB.
static inline unsigned __int128 bit(int d) {
    assert(d >= 0 && d < 128);
    return (unsigned __int128)1 << d;
}

// Two-limb ctz for unsigned __int128, needed wherever a digit-set mask can
// now have bits set at position >=64 (the Gen/FM arms' `allowed`/`used`/
// `available` masks) -- __builtin_ctzll silently truncates a u128 argument
// to its low 64 bits, which is exactly the C1-style silent-bug risk this
// epic is about. Caller must ensure x != 0 (matches __builtin_ctzll's own
// UB-on-zero contract).
static inline int ctz128(unsigned __int128 x) {
    uint64_t lo = (uint64_t)x;
    if (lo) return __builtin_ctzll(lo);
    uint64_t hi = (uint64_t)(x >> 64);
    return 64 + __builtin_ctzll(hi);
}

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

// CHANGE 2 (window-position widening knob, b50 audit): every hard-coded "20"
// below was the fixed digit-position (below the top/candidate digit) that
// buildSuffixBranchGen, runWrongTurnSearchFM, and the peeled/full-modulus
// planner's window derivations (chooseWY/chooseWYFM, enumeratePeeledConfigs,
// enumerateFMConfigs, buildFeasiblePrefix's callers) all assumed in lockstep.
// certPos() reads CERTPOS once (default 20, matching every prior run
// byte-for-byte), validates it against [20,24], and caches it -- every one
// of those sites now derives from this single value instead of repeating
// the literal, so widening the window is a one-env-var change instead of a
// silent-mismatch risk.
// MASK-WIDENING-128BIT.md Sec 4 (formula only, per review C3 minor note --
// the per-range table is documentation, not a second mechanism):
// configureCertPosForBase() must be called once, by each of
// runCert/runCertFM/runCertAuto, BEFORE certPos() is first invoked for that
// run (certPos()'s default/validation-range are latched on first call and
// never re-read). B<=64 leaves g_certPosDefault/-Max at their unconfigured
// sentinel (-1), so certPos() falls back to the exact literal 20/[20,24]
// every prior run used -- byte-identical for the whole regression suite.
static int g_certPosDefaultOverride = -1;
static int g_certPosMaxOverride = -1;
static void configureCertPosForBase(int B, int P_full) {
    int def, maxV;
    if (B <= 64) { def = 20; maxV = 24; }
    else { def = std::min(28, P_full + 8); maxV = std::max(24, P_full + 10); }
    g_certPosDefaultOverride = def;
    g_certPosMaxOverride = maxV;
    fprintf(stderr, "[setup] base=%d P_full=%d CERTPOS_default=%d CERTPOS_validrange=[20,%d]\n",
            B, P_full, def, maxV);
}

static int certPos() {
    static int pos = [] {
        int lo = 20;
        int hi = (g_certPosMaxOverride > 0) ? g_certPosMaxOverride : 24;
        int def = (g_certPosDefaultOverride > 0) ? g_certPosDefaultOverride : 20;
        const char *env = getenv("CERTPOS");
        int p = def;
        if (env && *env) {
            char *end = nullptr;
            long v = strtol(env, &end, 10);
            if (end == env || *end != '\0' || v < lo || v > hi) {
                fprintf(stderr, "[certdrv] FATAL: CERTPOS=\"%s\" invalid; must be an integer in [%d,%d]\n", env, lo, hi);
                exit(1);
            }
            p = (int)v;
        }
        return p;
    }();
    return pos;
}

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

// Exact admissible-ordered-suffix-tuple COUNT via digit DP (BASE-50-FULL-
// MODULUS-BUCKET.md Sec 6/9 Phase B): state = (occupied suffix-position mask
// in 0..2^T-1, weighted partial sum mod Lnil), processed incrementally over
// the pool's digits (each digit is either skipped, or placed into exactly
// one currently-unoccupied suffix position). Reaching mask==full with
// residue 0 after processing every pool digit corresponds to exactly one
// ordered assignment of T distinct pool digits to positions 0..T-1 weighted
// by B^position -- precisely the same set counted by
// countAdmissibleSuffixTuplesGen's depth-first permutation enumeration
// above, but in O(n * 2^T * T * Lnil) instead of O(n P T) time and without
// ever materializing a tuple. For base 50 (T=5, Lnil=160) that is a few
// thousand DP transitions per candidate digit instead of enumerating up to
// tens of millions of suffix permutations (doc Sec 6 "do not materialize
// tuples for counting").
static uint64_t countAdmissibleSuffixTuplesDP(const std::vector<int> &pool, int T, int B, u128 Lnil) {
    if (T <= 0) return 1; // no suffix positions: the empty tuple is trivially admissible
    if (Lnil == 0 || Lnil > (u128)200000) return UINT64_MAX; // sentinel: caller falls back / declines
    uint64_t Ln = (uint64_t)Lnil;
    int nMasks = 1 << T;
    std::vector<u128> Bpow(T, 1);
    for (int i = 1; i < T; i++) Bpow[i] = (Bpow[i - 1] * (u128)B) % Lnil;

    std::vector<uint64_t> dp((size_t)nMasks * Ln, 0);
    dp[0] = 1;
    std::vector<uint64_t> next(dp.size());
    for (int d : pool) {
        next = dp; // "skip this digit" case carried forward unchanged
        for (int mask = 0; mask < nMasks; mask++) {
            if (mask == nMasks - 1) continue; // no empty suffix position left to place d
            const uint64_t *srcRow = &dp[(size_t)mask * Ln];
            for (int j = 0; j < T; j++) {
                if (mask & (1 << j)) continue;
                uint64_t add = (uint64_t)((u128)d * Bpow[j] % Lnil);
                uint64_t *dstRow = &next[(size_t)(mask | (1 << j)) * Ln];
                for (uint64_t r = 0; r < Ln; r++) {
                    if (!srcRow[r]) continue;
                    dstRow[(r + add) % Ln] += srcRow[r];
                }
            }
        }
        dp.swap(next);
    }
    return dp[(size_t)(nMasks - 1) * Ln + 0];
}

// Cross-checks countAdmissibleSuffixTuplesDP against the brute-force
// countAdmissibleSuffixTuplesGen (task requirement: "must agree exactly"),
// plus an exact reproduction of BASE-50-FULL-MODULUS-BUCKET.md Sec 3.2's
// worked example (pool {1..20}, T=5, B=50, Lnil=160 -> 11,552 admissible
// ordered suffixes). Aborts the process (exit 1) on any mismatch -- a
// disagreement here means the DP is unsound and nothing downstream that
// relies on it (the planner's suffix multiplicity) can be trusted.
static void selfTestSuffixDP() {
    struct Case { std::vector<int> pool; int T; int B; u128 Lnil; uint64_t expect; const char *label; };
    std::vector<int> p20; for (int i = 1; i <= 20; i++) p20.push_back(i);
    std::vector<int> p15; for (int i = 1; i <= 15; i++) p15.push_back(i);
    std::vector<int> p12; for (int i = 1; i <= 12; i++) p12.push_back(i);
    std::vector<Case> cases = {
        {p20, 5, 50, 160, 11552, "doc#23-Sec3.2-b50-cand21"},
        {p15, 3, 48, 216, 0,     "b48-shape-T3-Lnil216"},
        {p12, 1, 49, 7,   0,     "b49-shape-T1-Lnil7"},
        {p15, 2, 48, 24,  0,     "misc-T2"},
    };
    bool allOk = true;
    for (auto &tc : cases) {
        uint64_t dpCount = countAdmissibleSuffixTuplesDP(tc.pool, tc.T, tc.B, tc.Lnil);
        int bruteCount = countAdmissibleSuffixTuplesGen(tc.pool, tc.T, tc.B, tc.Lnil);
        bool ok = (dpCount == (uint64_t)bruteCount) && (tc.expect == 0 || dpCount == tc.expect);
        char expectBuf[64] = "";
        if (tc.expect) snprintf(expectBuf, sizeof(expectBuf), " expect=%llu", (unsigned long long)tc.expect);
        fprintf(stderr, "[selftest-suffixdp] %s: DP=%llu brute=%d%s -> %s\n",
                tc.label, (unsigned long long)dpCount, bruteCount, expectBuf, ok ? "OK" : "MISMATCH");
        if (!ok) allOk = false;
    }
    if (!allOk) {
        fprintf(stderr, "[selftest-suffixdp] FATAL: suffix-tuple DP disagrees with brute enumeration\n");
        exit(1);
    }
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
    int pos = certPos() + (int)prefix.size(); // candidate fixed at position CERTPOS, same convention as b48
    for (int dd : prefix) {
        u128 term = mulmod_u128((u128)dd, powmod_u128((u128)c.B, (u128)pos, Lc), Lc);
        C = (C + term) % Lc;
        pos--;
    }
    C = (C + mulmod_u128((u128)candidate, powmod_u128((u128)c.B, (u128)certPos(), Lc), Lc)) % Lc;
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
// `available` is a digit-SET mask (widened per MASK-WIDENING-128BIT.md);
// digs[] must hold every set bit, i.e. up to the free-window size, which
// scales with dynamic CERTPOS (Sec 4, max 28) -- 32 keeps headroom above
// that without materially growing the stack frame. Uses ctz128 (not
// __builtin_ctzll) so digits >=64 are found instead of silently truncated.
static LeafEnvelopeGen leafEnvelopeGen(unsigned __int128 available, int B, int count) {
    int digs[32]; int nd = 0;
    unsigned __int128 av = available;
    while (av) { assert(nd < 32); digs[nd++] = ctz128(av); av &= av - 1; }
    LeafEnvelopeGen out{0, 0};
    for (int i = 0; i < count; i++) out.lo = out.lo * (u128)B + (u128)digs[i];
    for (int i = 0; i < count; i++) out.hi = out.hi * (u128)B + (u128)digs[nd - 1 - i];
    return out;
}
struct BucketRecordGen { u128 u; unsigned __int128 yMask; };
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

// Current (not peak) RSS of this process, in KB (Linux /proc/self/status
// VmRSS). Distinct from peakRssKB()'s VmHWM, which is monotonic non-
// decreasing for the whole process lifetime. Every EXISTING call site
// (cert/certfm, via bucketAdmissionGate's default useCurrentRssForAvailable
// =false below) keeps using peakRssKB() UNCHANGED, so their output stays
// byte-identical. certauto opts in (useCurrentRssForAvailable=true) both
// for its own planner's preflight queries AND for the actual builds its
// execution phase runs, because runWrongTurnSearch/FM loop over MULTIPLE
// candidates and/or suffix tuples, each building and then FREEING its own
// index -- charging a later candidate's admission check against the
// process's historical peak double-charges it for memory that an earlier,
// already-destructed index no longer holds (BASE-50-FULL-MODULUS-BUCKET.md
// Sec 9 Phase B, task item 5's "multi-candidate double-charge"). VmRSS
// reflects what is genuinely resident right now.
static long currentRssKB() {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[256];
    long kb = -1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) { sscanf(line + 6, "%ld", &kb); break; }
    }
    fclose(f);
    return kb;
}

// The same "C(R+4) + YR + 3(Q+1)*4" projected-peak formula bucketAdmissionGate
// uses inline below (RADIX-BUCKET-ADMISSION-CONTROL.md Sec 3), factored out
// so the certauto planner can reuse it exactly (task item: "use the Phase-A
// gate's projected-peak formula with the right record sizes") without
// duplicating -- and risking drifting out of sync with -- the real gate's
// arithmetic. bucketAdmissionGate's own body is left untouched below so its
// existing exit(3) call sites keep byte-identical behavior.
static std::optional<u128> projectedPeakBytesFor(u128 Y, u128 Q, size_t recBytes) {
    auto cOpt = nextPow2U128(Y);
    if (!cOpt) return std::nullopt;
    u128 C = *cOpt;
    u128 recBytesU = (u128)recBytes;
    u128 idxBytes = (u128)sizeof(uint32_t);
    return C * (recBytesU + idxBytes) + Y * recBytesU + (u128)3 * (Q + 1) * idxBytes;
}

// Pure QUERY form of the memory gate (RADIX-BUCKET-ADMISSION-CONTROL.md
// Sec 9 "Sound fallback semantics" / Sec 10 planner sketch): computes the
// admit/decline decision and returns it as data, WITHOUT printing a trace
// or calling exit(3) -- the certauto planner below evaluates many
// (NX,NY,K) configurations and must be able to discard infeasible ones
// silently before committing to a trace for the CHOSEN plan. Existing
// call sites keep calling bucketAdmissionGate() (unchanged, still prints
// + exit(3)s); this is new additive plumbing, not a replacement.
struct BucketAdmissionResult {
    bool admitted = false;
    std::string reason;
    u128 Y = 0, Q = 0;
    bool yOverflow = false, qOverflow = false; // which checked-arithmetic call (if any) actually overflowed
    u128 projectedPeakBytes = 0;
    bool haveProjected = false;
    u128 availableBytes = 0;
};
static BucketAdmissionResult bucketAdmissionQuery(int a, int NY, int K, int B, long rssBudgetKB,
                                                    size_t recBytes, long rssForAvailableKB) {
    BucketAdmissionResult out;
    if (rssBudgetKB <= 0) { out.admitted = true; return out; } // budget disabled: same convention as bucketAdmissionGate

    auto yOpt = checkedFallingFactorial(a, NY);
    auto qOpt = checkedPower((u128)B, K);

    u128 currentRssBytes = rssForAvailableKB > 0 ? (u128)rssForAvailableKB * 1024 : (u128)0;
    const u128 kReserveBytes = (u128)256 * 1024 * 1024;
    u128 budgetBytes = (u128)rssBudgetKB * 1024;
    out.availableBytes = (budgetBytes > currentRssBytes + kReserveBytes)
                              ? (budgetBytes - currentRssBytes - kReserveBytes)
                              : (u128)0;

    if (!yOpt) { out.reason = "Y_overflow"; out.yOverflow = true; return out; }
    if (!qOpt) { out.reason = "Q_overflow"; out.qOverflow = true; return out; }
    out.Y = *yOpt; out.Q = *qOpt;
    if (out.Y > (u128)UINT32_MAX) { out.reason = "Y_exceeds_uint32"; return out; }

    auto bytesOpt = projectedPeakBytesFor(out.Y, out.Q, recBytes);
    if (!bytesOpt) { out.reason = "peak_bytes_overflow"; return out; }
    out.projectedPeakBytes = *bytesOpt;
    out.haveProjected = true;
    u128 need = out.projectedPeakBytes + (out.projectedPeakBytes * 15) / 100; // 1.15x, same margin as bucketAdmissionGate
    if (need > out.availableBytes) { out.reason = "insufficient_budget"; return out; }

    out.admitted = true;
    return out;
}

// Preflight admission gate for the current grow-and-scatter builder below
// (RADIX-BUCKET-ADMISSION-CONTROL.md Sec 2/3, Sec 12 Phase A). Active only
// when rssBudgetKB > 0; manual/debug call sites that pass -1 keep exactly
// their current behavior. A decline here is resource POLICY, not
// mathematics (Sec 9): it must never be mistaken for a refutation, so it
// terminates the process immediately (exit 3 = BUCKET_DECLINED) with a
// reproducible trace, rather than returning a value the caller (or the
// wrong-turn search above it) could interpret as "no survivors."
//
// Thin wrapper over bucketAdmissionQuery: with useCurrentRssForAvailable
// left at its default (false), this reproduces the ORIGINAL inline
// computation's output EXACTLY -- same formula, same order of checks, same
// (intentionally preserved) quirk that Y prints as 0 rather than the true
// falling-factorial value on the rare Q-overflow-only path, since that
// quirk was already unreachable in practice (Q=B^K never overflows for the
// K<=4 values this driver uses) and preserving cert/certfm's byte-for-byte
// output takes priority over quietly fixing it here. Passing
// useCurrentRssForAvailable=true (certauto's execution-phase call sites)
// is the only behavioral change this wrapper can introduce, and only for
// callers that explicitly opt in.
static void bucketAdmissionGate(int a, int NY, int K, int B, long rssBudgetKB,
                                 size_t recBytes = sizeof(BucketRecordGen),
                                 bool useCurrentRssForAvailable = false) {
    if (rssBudgetKB <= 0) return;

    long rssKB = useCurrentRssForAvailable ? currentRssKB() : peakRssKB();
    BucketAdmissionResult r = bucketAdmissionQuery(a, NY, K, B, rssBudgetKB, recBytes, rssKB);

    std::string yStr = r.yOverflow ? std::string("OVERFLOW") : u128_to_string(r.Y);
    std::string qStr = r.qOverflow ? std::string("OVERFLOW") : u128_to_string(r.Q);
    std::string projStr = r.haveProjected ? formatGiB(r.projectedPeakBytes) : std::string("n/a");
    fprintf(stderr, "[bucket-plan] B=%d a=%d NY=%d K=%d Y=%s Q=%s projectedPeak=%s availableForIndex=%s\n",
            B, a, NY, K, yStr.c_str(), qStr.c_str(), projStr.c_str(), formatGiB(r.availableBytes).c_str());

    if (!r.admitted) {
        fprintf(stderr, "[bucket-plan] decision=DECLINE_MEMORY reason=%s fallback=scan (exit 3 = BUCKET_DECLINED)\n", r.reason.c_str());
        exit(3);
    }

    fprintf(stderr, "[bucket-plan] decision=ADMIT\n");
}

static BucketIndexGen buildBucketIndexGen(const std::vector<int> &A, int NY, int K, int B, int Pc, u128 Lc,
                                           u64 &yCount, long rssBudgetKB = -1, bool useCurrentRssForAvailable = false) {
    bucketAdmissionGate((int)A.size(), NY, K, B, rssBudgetKB, sizeof(BucketRecordGen), useCurrentRssForAvailable);
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
        unsigned __int128 m = 0; for (int d : ydigits) m |= bit(d);
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
static void enumLeafPrefixGen(int depth, int K, int B, int borrow, unsigned __int128 used, uint64_t key, uint64_t keyMul,
                               const std::array<uint8_t,16> &Wdig, unsigned __int128 allowed, CB &&cb) {
    if (depth == K) { cb(key, used); return; }
    unsigned __int128 rem = allowed & ~used;
    while (rem) {
        int l = ctz128(rem);
        rem &= rem - 1;
        int raw = (int)Wdig[depth] - borrow - l;
        int u_i, nb;
        if (raw < 0) { u_i = raw + B; nb = 1; } else { u_i = raw; nb = 0; }
        enumLeafPrefixGen(depth + 1, K, B, nb, used | bit(l), key + (uint64_t)u_i * keyMul, keyMul * (uint64_t)B,
                          Wdig, allowed, cb);
    }
}
static inline bool decodeDistinctLeafGen(u128 leaf, unsigned __int128 allowed, int B, int Pc, unsigned __int128 &outMask) {
    unsigned __int128 mask = 0;
    for (int i = 0; i < Pc; i++) {
        int d = (int)(leaf % (u128)B);
        leaf /= (u128)B;
        if (d == 0) return false;
        unsigned __int128 db = bit(d);
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
                              unsigned __int128 xmask, unsigned __int128 Amask, int K, BucketStatsGen &stats,
                              const std::vector<u128> &Bpow, OnSurvivor &&onSurvivor) {
    unsigned __int128 allowed = Amask & ~xmask;
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
            [&](uint64_t key, unsigned __int128 lowLeafMask) {
                stats.lookups++;
                uint32_t lo = idx.offsets[key], hi = idx.offsets[key + 1];
                for (uint32_t p = lo; p < hi; p++) {
                    stats.scans++;
                    const BucketRecordGen &r = idx.records[p];
                    if (r.yMask & (xmask | lowLeafMask)) continue;
                    if (W < r.u) continue;
                    u128 leaf = W - r.u;
                    if (leaf >= Bpow[Pc]) continue;
                    unsigned __int128 decodedMask;
                    if (!decodeDistinctLeafGen(leaf, allowed, B, Pc, decodedMask)) continue;
                    unsigned __int128 required = Amask & ~xmask & ~r.yMask;
                    if (decodedMask != required) continue;
                    stats.survivors++;
                    onSurvivor(r, leaf, decodedMask);
                }
            });
    }
}

// ============================================================================
// Full-modulus bucket engine (BASE-50-FULL-MODULUS-BUCKET.md Sec 4/9 Phase A).
//
// Same shallow-radix join machinery as the peeled path above, but run
// directly modulo the original L instead of modulo L_c after nilpotent
// suffix-peeling: M=L, leaf width P=P_full=ceil(log_B L), no suffix
// enumeration (T plays no role -- the nilpotent condition is absorbed by
// working mod the full L). Records additionally carry PACKED ORDERED y
// digits so no modular inverse of B mod L is needed to recover y from u_y
// (doc Sec 4 "No inverse of 50 modulo L is needed", Sec 9 "Remove the
// modular-inverse reconstruction requirement").
//
// doc Sec 7 CRITICAL warning: never deduplicate records sharing (u, mask) --
// different digit orders of the same y multiset are distinct records with
// distinct reconstructions. Packing the exact digit order (not just the
// residue/mask) sidesteps this trap: every permutation gets its own record.
// ============================================================================

// NY <= 10 digits, each < 64, packed 6 bits/digit LSD-first into a uint64_t
// (up to 60 bits used of 64). Digit 0 is never a valid selected digit here
// (all digits are 1..B-1), so 0 is safe as a non-conflicting pad value, but
// we never rely on that -- unpack always uses the caller-supplied NY count.
//
// SOUNDNESS FIX (b50 width-20 false-negative autopsy, 2026-07-23): this was
// a uint32_t (30 bits = 5 digits max) despite the planner legally choosing
// NY up to 6+ for the full-modulus family. With NY=6, the 6th (most
// significant) digit's 6-bit field landed at bit offset 30, so only its
// low 2 bits survived a 32-bit shift and its high bits were silently
// dropped -- corrupting the reconstructed y digit (e.g. 12 -> 0) and
// causing a genuine completion to fail verifySurvivorDirectGen's
// distinctness/range check as a false negative, even though the bucket
// join itself had correctly found it. Widening to uint64_t removes the
// truncation for any NY this driver's planner can select.
static inline uint64_t packDigitsLSD(const std::vector<int> &digits) {
    uint64_t packed = 0;
    for (size_t i = 0; i < digits.size(); i++) packed |= (uint64_t)(digits[i] & 0x3F) << (6 * i);
    return packed;
}
static inline void unpackDigitsLSD(uint64_t packed, int count, std::vector<int> &out) {
    out.resize(count);
    for (int i = 0; i < count; i++) out[i] = (int)((packed >> (6 * i)) & 0x3F);
}

// yDigitsPacked stays uint64_t (packed ORDERED digits, 6 bits each -- not a
// digit-SET mask; see MASK-WIDENING-128BIT.md report, "leave alone").
struct BucketRecordFM { u128 u; unsigned __int128 yMask; uint64_t yDigitsPacked; };
struct BucketIndexFM {
    std::vector<uint32_t> offsets;
    std::vector<BucketRecordFM> records;
    int K = 3;
    u64 bucketCount = 0;
};

// Mirrors buildBucketIndexGen exactly, parameterized by the general modulus
// M and leaf width P instead of the peeled path's fixed L_c/Pc, and storing
// packed y digits in each record. Reuses the SAME admission gate
// (bucketAdmissionGate), passing the FM record's larger sizeof so the memory
// projection stays accurate for this record shape.
static BucketIndexFM buildBucketIndexFM(const std::vector<int> &A, int NY, int K, int B, int P, u128 M,
                                         u64 &yCount, long rssBudgetKB = -1, bool useCurrentRssForAvailable = false) {
    bucketAdmissionGate((int)A.size(), NY, K, B, rssBudgetKB, sizeof(BucketRecordFM), useCurrentRssForAvailable);
    u128 BPmodM = powmod_u128((u128)B, (u128)P, M);
    u64 bucketCount = 1;
    for (int i = 0; i < K; i++) bucketCount *= (u64)B;
    u128 bucketMod = (u128)bucketCount;

    std::vector<BucketRecordFM> raw;
    std::vector<uint32_t> key;
    std::vector<uint32_t> counts(bucketCount + 1, 0);
    forEachPermutation(A, NY, [&](const std::vector<int> &ydigits) {
        u128 y_val = 0, Bpow = 1;
        for (int i = 0; i < NY; i++) { y_val += (u128)ydigits[i] * Bpow; Bpow *= (u128)B; }
        u128 u_y = mulmod_u128(BPmodM, y_val % M, M);
        unsigned __int128 m = 0; for (int d : ydigits) m |= bit(d);
        uint64_t packed = packDigitsLSD(ydigits);
        raw.push_back({u_y, m, packed});
        uint32_t k = (uint32_t)(u_y % bucketMod);
        key.push_back(k);
        counts[k + 1]++;
    });
    yCount = raw.size();
    checkRssBudget(rssBudgetKB, "certdrv-fm-bucket-post-generation");

    for (u64 i = 0; i < bucketCount; i++) counts[i + 1] += counts[i];
    BucketIndexFM idx;
    idx.K = K; idx.bucketCount = bucketCount;
    idx.offsets = counts;
    idx.records.resize(raw.size());
    std::vector<uint32_t> cursor(idx.offsets.begin(), idx.offsets.end());
    for (size_t i = 0; i < raw.size(); i++) { uint32_t k = key[i]; idx.records[cursor[k]++] = raw[i]; }
    checkRssBudget(rssBudgetKB, "certdrv-fm-bucket-post-build");
    return idx;
}

// Mirrors bucketSearchXGen, but takes an explicit `liftsFull` (the caller
// computes the CONSERVATIVE-INCLUSIVE lift bound per doc Sec 4 "Lift
// enumeration": j = 0 .. ceil(B^P / M) inclusive -- NOT the peeled path's
// ceil(B^Pc / Lc) convention without the +1) and searches BucketRecordFM
// records instead of BucketRecordGen. The per-record acceptance tests
// (mask disjointness, W >= u, leaf < B^P, exact decoded-mask match) are
// IDENTICAL to the peeled path and constitute the doc Sec 8 correctness
// argument steps 4-6; W >= r.u / leaf < Bpow[P] here is exactly the
// "retain a pair only when W_j >= u_y AND W_j - u_y < B^P" condition from
// doc Sec 4, applied per-record across all lifts rather than assuming a
// fixed count.
template <typename OnSurvivor>
static void bucketSearchXFM(const BucketIndexFM &idx, u128 c_x, u128 M, int liftsFull, int B, int P,
                             unsigned __int128 xmask, unsigned __int128 Amask, int K, BucketStatsGen &stats,
                             const std::vector<u128> &Bpow, OnSurvivor &&onSurvivor) {
    unsigned __int128 allowed = Amask & ~xmask;
    LeafEnvelopeGen env{};
    if (SIEVE_LEVEL & 1) env = leafEnvelopeGen(allowed, B, P);
    for (int j = 0; j < liftsFull; j++) {
        u128 W = c_x + (u128)j * M;
        if (SIEVE_LEVEL & 1) {
            stats.roots++;
            if (W < env.lo) { stats.rootsRejected++; continue; }
            u128 minReq = (W > env.hi) ? (W - env.hi) : (u128)0;
            if (minReq >= M) { stats.rootsRejected++; continue; }
        } else stats.roots++;
        auto Wdig = digitsLSDofGen(W, B, K > P ? K : P);
        enumLeafPrefixGen(0, K, B, 0, 0, 0, 1, Wdig, allowed,
            [&](uint64_t key, unsigned __int128 lowLeafMask) {
                stats.lookups++;
                uint32_t lo = idx.offsets[key], hi = idx.offsets[key + 1];
                for (uint32_t p = lo; p < hi; p++) {
                    stats.scans++;
                    const BucketRecordFM &r = idx.records[p];
                    if (r.yMask & (xmask | lowLeafMask)) continue;
                    if (W < r.u) continue;
                    u128 leaf = W - r.u;
                    if (leaf >= Bpow[P]) continue;
                    unsigned __int128 decodedMask;
                    if (!decodeDistinctLeafGen(leaf, allowed, B, P, decodedMask)) continue;
                    unsigned __int128 required = Amask & ~xmask & ~r.yMask;
                    if (decodedMask != required) continue;
                    stats.survivors++;
                    onSurvivor(r, leaf, decodedMask);
                }
            });
    }
}

static bool verifySurvivorDirectGen(const ConstantsGen &c, const std::vector<int> &prefix, int candidate,
                                     const std::vector<int> &suffix /*LSD..MSD i.e. suffix[0]=d0*/,
                                     const std::vector<int> &freeDigitsMSBfirst,
                                     std::vector<int> *outDigitsMSBfirst = nullptr) {
    std::vector<int> digitsMSBfirst;
    for (int dd : prefix) digitsMSBfirst.push_back(dd);
    digitsMSBfirst.push_back(candidate);
    for (int d : freeDigitsMSBfirst) digitsMSBfirst.push_back(d);
    for (int i = (int)suffix.size() - 1; i >= 0; i--) digitsMSBfirst.push_back(suffix[i]);
    if (outDigitsMSBfirst) *outDigitsMSBfirst = digitsMSBfirst;
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
    // Cumulative BucketStatsGen totals across every candidate/suffix branch
    // this call actually walked (wrong turns AND the eventual winner, if
    // any) -- used by certauto to print predicted-vs-actual counters
    // (task item 4) against the planner's single-representative-candidate
    // estimate. Not populated by cert/certfm's call sites' callers (they
    // never read these fields), so this is purely additive.
    u64 totalRoots = 0, totalLookups = 0, totalScans = 0;
    // HIGHER-BASE-CERTIFICATION-STRATEGY.md Sec3+8 outer-DFS support
    // (obligation I1): the winning survivor's full digit array, MSD-first,
    // fixed length == |D| always -- the ONLY thing certbb's incumbent
    // comparisons touch (never maxDecimal, never any integer). Populated
    // alongside maxDecimal wherever runWrongTurnSearch updates bestDecimal.
    std::vector<int> maxDigitsMSBfirst;
    // Set when a caller-supplied deadline (obligation I4: any terminal that
    // times out is UNFINISHED, never a refutation) was reached before the
    // candidate loop exhausted every pool digit. When true, `success` is
    // guaranteed false (the loop breaks before evaluating the timed-out
    // candidate), so callers must check timedOut BEFORE treating
    // !success as a refutation.
    bool timedOut = false;
};

// ---------------------------------------------------------------------
// BAND-DEPTH-CERTIFICATION.md Phase 2 (GroupedCyclotomicDP), Site 2 forward
// declarations. The real definitions live in `namespace subsetdisc` below
// (after this point in the file, since subsetdisc's per-subset PPsub state
// -- SB_pps/SB_npps/SB_LCM -- is what the grouped moduli are derived from),
// but runWrongTurnSearch/runWrongTurnSearchFM (defined here, above
// subsetdisc) need to call into it for the Site-2 candidate-window
// precheck. Forward-declaring into the SAME already-open `certdrv`
// namespace's `subsetdisc` sub-namespace is standard C++ (multiple
// namespace-opening blocks referring to the same namespace); the
// definitions below must match these signatures exactly.
namespace subsetdisc {
    // shadow|active|off, cached from GROUPDP env var after first call.
    int SB_groupDPMode();
    // True once SB_buildGroupedGroups() has populated SB_gdpGroups for the
    // CURRENT subset (set by SB_subset_filters(), which always runs before
    // any search call for that subset -- see SB_checkAndLogOrder call sites).
    bool SB_gdpGroupsAvailable();
    // Mirrors buildSuffixBranchGen's / runWrongTurnSearchFM's own C
    // computation EXACTLY (prefix + candidate contribution at position
    // certPos(), reduced mod `modulus`) -- never re-derived independently,
    // per C2-iv. Callers pass their own c.Lc (peeled) or c.L (FM) as
    // `modulus`; the returned C is later reduced further mod each Q_e | Lc
    // | L by SB_site2GroupedFeasible itself (valid by CRT since Q_e divides
    // `modulus`).
    u128 SB_computePrefixCandidateC(int B, const std::vector<int> &prefix, int candidate, u128 modulus);
    // Necessary-condition precheck over the CURRENT subset's grouped Q_e's
    // (built once per subset, reused here -- see C2-iv "highest risk line"):
    // tests whether ANY assignment of `restAvailMask`'s digits to the
    // certPosVal positions 0..certPosVal-1 (below the fixed prefix+
    // candidate) can satisfy total===0 (mod L) for EVERY grouped Q_e | L.
    // Returns 1 (feasible / declined-to-decide) or 0 (PROVEN infeasible).
    // Logs every INFEASIBLE verdict to stderr tagged by `tag` ("peeled" /
    // "fm") and `candidate`. Does NOT itself decide shadow-vs-active
    // behavior -- callers do that (shadow: log + continue; active: skip +
    // continue, treating the candidate as a wrong turn).
    int SB_site2GroupedFeasible(u128 modulus, u128 Cfixed, unsigned __int128 restAvailMask,
                                 int certPosVal, const char *tag, int candidate);
    // C3 contradiction-oracle counters, incremented here, read/reported by
    // the certPos()-window search call sites and by runCert/FM/Auto's final
    // summary prints.
    void SB_gdpSite2NoteResult(bool shadowSaidInfeasible, u64 totalSurvivorsForCandidate,
                                const char *tag, int candidate);
}

// The core wrong-turn-refutation search: given a fully-specified D, a
// derived ConstantsGen, and a prefix, try candidate digits at the
// window-top position in descending order. Each refuted candidate is a
// "wrong turn"; the first with survivors wins (max taken across all its
// admissible suffix families).
static AttemptResult runWrongTurnSearch(const ConstantsGen &c, const std::vector<int> &prefix,
                                         const std::vector<int> &pool, int NX, int NY, int K,
                                         long rssBudgetKB, bool useCurrentRssForAvailable = false,
                                         std::chrono::steady_clock::time_point deadline =
                                             std::chrono::steady_clock::time_point::max()) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    AttemptResult res;

    // MASK-WIDENING-128BIT.md Sec 4: NX+NY<2 means the join has degenerated
    // to nothing (CERTPOS starvation risk at high bases) -- a resource-limit
    // decline, never a refutation. exit(3) matches certauto's own
    // resource-policy-decline convention (see the planner-declined exit(3)
    // just above runCertAuto's search dispatch).
    if (NX + NY < 2) {
        fprintf(stderr, "[certdrv] FATAL: NX+NY=%d < 2 before join search (base=%d) -- resource-policy "
                        "decline (CERTPOS window starved), not a refutation.\n", NX + NY, c.B);
        exit(3);
    }

    std::vector<int> candidates = pool; // try largest-first
    std::sort(candidates.begin(), candidates.end(), std::greater<int>());

    std::vector<u128> Bpow(c.Pc + 1, 1);
    for (int i = 1; i <= c.Pc; i++) Bpow[i] = Bpow[i-1] * (u128)c.B;

    for (int candidate : candidates) {
        // HIGHER-BASE-CERTIFICATION-STRATEGY.md Sec8/I4: a caller-supplied
        // deadline (certbb's per-terminal budget) reached before every pool
        // digit has been tried is a RESOURCE_DECLINED terminal, never a
        // refutation -- stop trying further candidates and report timedOut
        // with whatever wrongTurnsRefuted/totalSurvivors were accumulated so
        // far (success is guaranteed false here: had a survivor already been
        // found the loop would already have broken above via `break`).
        if (clock::now() >= deadline) { res.timedOut = true; break; }
        std::vector<int> restPool;
        for (int d : pool) if (d != candidate) restPool.push_back(d);

        // BAND-DEPTH-CERTIFICATION.md Phase 2, Site 2 (C2-iv "highest risk
        // line"): grouped-DP precheck over restPool's certPos() positions
        // (T suffix + Pc+NX+NY free -- restPool covers ALL of them jointly;
        // the suffix/free split hasn't happened yet at this point, which is
        // fine, this is a coarser-but-still-sound necessary condition), mod
        // Lc (this is the peeled path -- Lc excludes B-nilpotent primes,
        // exactly the primes SB_pps groups). Modulus/positions/candidate
        // residue are all derived the SAME way buildSuffixBranchGen derives
        // its own C below (mirrored via SB_computePrefixCandidateC, never
        // independently re-derived).
        int gdpMode = subsetdisc::SB_groupDPMode();
        bool site2ShadowInfeasible = false;
        if (gdpMode != 0 && subsetdisc::SB_gdpGroupsAvailable()) {
            unsigned __int128 restAvailMask = 0; for (int d : restPool) restAvailMask |= bit(d);
            u128 Cfixed = subsetdisc::SB_computePrefixCandidateC(c.B, prefix, candidate, c.Lc);
            int feas = subsetdisc::SB_site2GroupedFeasible(c.Lc, Cfixed, restAvailMask,
                                                             (int)restPool.size(), "peeled", candidate);
            if (!feas) {
                if (gdpMode == 2) { // active: sound skip, never build the expensive suffix/index machinery
                    res.wrongTurnsRefuted++;
                    subsetdisc::SB_gdpSite2NoteResult(true, 0, "peeled-active-skip", candidate);
                    continue;
                }
                site2ShadowInfeasible = true; // shadow: fall through, run the real search, compare after
            }
        }

        std::vector<std::vector<int>> tuples;
        int nTuples = countAdmissibleSuffixTuplesGen(restPool, c.T, c.B, c.Lnil, &tuples);
        if (nTuples == 0) continue; // this candidate can't even form a suffix; not a real refutation, just infeasible bookkeeping -- skip silently to next-largest

        u64 totalSurvivors = 0, verifiedOk = 0, verifiedBad = 0;
        std::string bestDecimal;
        std::vector<int> bestDigits; // MSD-first, mirrors bestDecimal (I1 support)

        for (auto &suffix : tuples) {
            SuffixBranchGen sb = buildSuffixBranchGen(c, prefix, candidate, restPool, suffix);
            if ((int)sb.freeDigits.size() != c.Pc + NX + NY) continue; // shouldn't happen; skip defensively

            unsigned __int128 Amask = 0; for (int d : sb.freeDigits) Amask |= bit(d);
            u64 yCount = 0;
            BucketIndexGen idx = buildBucketIndexGen(sb.freeDigits, NY, K, c.B, c.Pc, c.Lc, yCount, rssBudgetKB, useCurrentRssForAvailable);
            u128 Bexp = powmod_u128((u128)c.B, (u128)(c.Pc + NY), c.Lc);
            BucketStatsGen stats;
            forEachPermutation(sb.freeDigits, NX, [&](const std::vector<int> &xdigits) {
                u128 x_val = 0, Bp = 1;
                for (int i = 0; i < NX; i++) { x_val += (u128)xdigits[i] * Bp; Bp *= (u128)c.B; }
                u128 term = mulmod_u128(Bexp, x_val % c.Lc, c.Lc);
                u128 c_x = (sb.T_target + c.Lc - (term % c.Lc)) % c.Lc;
                unsigned __int128 xmask = 0; for (int d : xdigits) xmask |= bit(d);
                bucketSearchXGen(idx, c_x, c.Lc, c.lifts, c.B, c.Pc, xmask, Amask, K, stats, Bpow,
                    [&](const BucketRecordGen &r, u128 leaf, unsigned __int128 decodedMask) {
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

                        std::vector<int> fullDigits;
                        bool ok = verifySurvivorDirectGen(c, prefix, candidate, suffix, freeMSB, &fullDigits);
                        if (ok) verifiedOk++; else verifiedBad++;
                        if (ok) {
                            std::string dec = reconstructDecimalGen(c.B, prefix, candidate, suffix, freeMSB);
                            if (bestDecimal.empty() || dec.size() > bestDecimal.size() ||
                                (dec.size() == bestDecimal.size() && dec > bestDecimal))
                                bestDecimal = dec;
                            // I1: bestDigits tracked by its OWN fixed-length
                            // array lex comparison, independent of the
                            // decimal-string logic just above (kept for
                            // byte-identical legacy behavior/output).
                            if (bestDigits.empty()) bestDigits = fullDigits;
                            else {
                                size_t n = std::min(bestDigits.size(), fullDigits.size());
                                bool greater = false;
                                for (size_t i = 0; i < n; i++) {
                                    if (fullDigits[i] != bestDigits[i]) { greater = fullDigits[i] > bestDigits[i]; break; }
                                }
                                if (fullDigits.size() != bestDigits.size() && !greater) {
                                    // shouldn't happen (both always length |D|), but stay defensive/honest
                                    greater = fullDigits.size() > bestDigits.size();
                                }
                                if (greater) bestDigits = fullDigits;
                            }
                        }
                    });
            });
            totalSurvivors += stats.survivors;
            res.totalRoots += stats.roots; res.totalLookups += stats.lookups; res.totalScans += stats.scans;
            checkRssBudget(rssBudgetKB, "certdrv-per-suffix-tuple");
            long kb = peakRssKB(); if (kb > res.peakRssKBSeen) res.peakRssKBSeen = kb;
        }

        // C3 contradiction oracle (mandatory, must never be silent): the
        // shadow precheck said this candidate's window is infeasible mod
        // some grouped Q_e | Lc; if the EXACT search just found a real
        // survivor anyway, the DP (or its wiring) is unsound -- abort
        // loudly rather than let a false-negative feasibility gate ever
        // reach active mode undetected.
        subsetdisc::SB_gdpSite2NoteResult(site2ShadowInfeasible, totalSurvivors, "peeled", candidate);
        if (site2ShadowInfeasible && totalSurvivors > 0) {
            fprintf(stderr, "[GROUPDP-CONTRADICTION] site=2 path=peeled base=B%d candidate=%d "
                            "restPoolSize=%zu certPos=%d totalSurvivors=%llu -- grouped-DP precheck said "
                            "INFEASIBLE but the exact search found survivor(s). This is a PROVEN "
                            "unsoundness in the grouped-DP precheck (or its position/modulus wiring). "
                            "Aborting.\n",
                    c.B, candidate, restPool.size(), certPos(), (unsigned long long)totalSurvivors);
            fflush(stderr);
            abort();
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
        res.maxDigitsMSBfirst = bestDigits;
        res.winningCandidate = candidate;
        break;
    }

    auto t1 = clock::now();
    res.wallSeconds = std::chrono::duration<double>(t1 - t0).count();
    return res;
}

// Full-modulus analogue of runWrongTurnSearch (BASE-50-FULL-MODULUS-BUCKET.md
// Sec 4/8/9 Phase A): replaces the per-suffix peeled join (modulus L_c, T
// suffix digits consumed first, one bucket search per admissible ordered
// suffix) with ONE join directly modulo the original L. There is no suffix
// loop at all -- T plays no role in this decomposition -- so `pool` minus
// the tried candidate IS the free window directly (size P+NX+NY), unlike
// the peeled path where restPool still had to be split into a suffix tuple
// plus SuffixBranchGen's freeDigits. The CANDIDATE_POS=certPos() invariant
// becomes P + NY + NX == certPos() (asserted below rather than assumed).
static AttemptResult runWrongTurnSearchFM(const ConstantsGen &c, const std::vector<int> &prefix,
                                           const std::vector<int> &pool, int P, int NX, int NY, int K,
                                           long rssBudgetKB, bool useCurrentRssForAvailable = false) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    AttemptResult res;

    if (P + NX + NY != certPos()) {
        fprintf(stderr, "[certfm] FATAL: CANDIDATE_POS invariant violated: P=%d NX=%d NY=%d (sum=%d != %d)\n",
                P, NX, NY, P + NX + NY, certPos());
        exit(1);
    }
    if (NX + NY < 2) {
        fprintf(stderr, "[certdrv] FATAL: NX+NY=%d < 2 before join search (base=%d) -- resource-policy "
                        "decline (CERTPOS window starved), not a refutation.\n", NX + NY, c.B);
        exit(3);
    }

    std::vector<int> candidates = pool; // try largest-first
    std::sort(candidates.begin(), candidates.end(), std::greater<int>());

    std::vector<u128> Bpow(P + 1, 1);
    for (int i = 1; i <= P; i++) Bpow[i] = Bpow[i-1] * (u128)c.B;

    // Conservative-inclusive lift count (doc Sec 4 "Lift enumeration"):
    // j = 0 .. ceil(B^P / L) inclusive -- NOT the peeled path's fixed-count
    // convention. This is what covers the u_y > c_x wrap case soundly.
    u128 jmax = (Bpow[P] + c.L - 1) / c.L;
    int liftsFull = (int)jmax + 1;

    const std::vector<int> emptySuffix; // no suffix in the full-modulus join

    for (int candidate : candidates) {
        std::vector<int> restPool;
        for (int d : pool) if (d != candidate) restPool.push_back(d);
        if ((int)restPool.size() != P + NX + NY) continue; // shouldn't happen; defensive

        unsigned __int128 Amask = 0; for (int d : restPool) Amask |= bit(d);

        // C: contribution of the fixed prefix and this candidate mod L
        // (mirrors buildSuffixBranchGen's C computation, but with NO suffix
        // term and NO B^{-T} shift -- there is no suffix to fold in here).
        // N == C + leaf + B^P y + B^(P+NY) x (mod L), so
        // leaf + B^P y + B^(P+NY) x == target := -C (mod L).
        u128 C = 0;
        int pos = certPos() + (int)prefix.size();
        for (int dd : prefix) {
            u128 term = mulmod_u128((u128)dd, powmod_u128((u128)c.B, (u128)pos, c.L), c.L);
            C = (C + term) % c.L;
            pos--;
        }
        C = (C + mulmod_u128((u128)candidate, powmod_u128((u128)c.B, (u128)certPos(), c.L), c.L)) % c.L;
        u128 target = (c.L - (C % c.L)) % c.L;

        // BAND-DEPTH-CERTIFICATION.md Phase 2, Site 2 (C2-iv "highest risk
        // line"), FM analogue of the peeled-path precheck above: SAME `C`
        // (computed two lines up, not re-derived independently) and SAME
        // restPool/Amask this function already built for the real join,
        // reduced mod L instead of Lc (no suffix to exclude here).
        int gdpModeFM = subsetdisc::SB_groupDPMode();
        bool site2ShadowInfeasibleFM = false;
        if (gdpModeFM != 0 && subsetdisc::SB_gdpGroupsAvailable()) {
            int feas = subsetdisc::SB_site2GroupedFeasible(c.L, C, Amask, (int)restPool.size(), "fm", candidate);
            if (!feas) {
                if (gdpModeFM == 2) {
                    res.wrongTurnsRefuted++;
                    subsetdisc::SB_gdpSite2NoteResult(true, 0, "fm-active-skip", candidate);
                    continue;
                }
                site2ShadowInfeasibleFM = true;
            }
        }

        u64 yCount = 0;
        BucketIndexFM idx = buildBucketIndexFM(restPool, NY, K, c.B, P, c.L, yCount, rssBudgetKB, useCurrentRssForAvailable);
        u128 Bexp = powmod_u128((u128)c.B, (u128)(P + NY), c.L);
        BucketStatsGen stats;
        u64 totalSurvivors = 0, verifiedOk = 0, verifiedBad = 0;
        std::string bestDecimal;

        forEachPermutation(restPool, NX, [&](const std::vector<int> &xdigits) {
            u128 x_val = 0, Bp = 1;
            for (int i = 0; i < NX; i++) { x_val += (u128)xdigits[i] * Bp; Bp *= (u128)c.B; }
            u128 term = mulmod_u128(Bexp, x_val % c.L, c.L);
            u128 c_x = (target + c.L - (term % c.L)) % c.L;
            unsigned __int128 xmask = 0; for (int d : xdigits) xmask |= bit(d);
            bucketSearchXFM(idx, c_x, c.L, liftsFull, c.B, P, xmask, Amask, K, stats, Bpow,
                [&](const BucketRecordFM &r, u128 leaf, unsigned __int128 decodedMask) {
                    std::vector<int> leafDigits(P);
                    u128 lv = leaf;
                    for (int i = 0; i < P; i++) { leafDigits[i] = (int)(lv % (u128)c.B); lv /= (u128)c.B; }
                    // Packed ordered y digits recovered directly -- NO
                    // modular inverse of B mod L (doc Sec 4/9).
                    std::vector<int> ydigits;
                    unpackDigitsLSD(r.yDigitsPacked, NY, ydigits);

                    std::vector<int> freeMSB;
                    for (int i = NX - 1; i >= 0; i--) freeMSB.push_back(xdigits[i]);
                    for (int i = NY - 1; i >= 0; i--) freeMSB.push_back(ydigits[i]);
                    for (int i = P - 1; i >= 0; i--) freeMSB.push_back(leafDigits[i]);

                    // doc Sec 4 step 9 / Sec 8 step 7: direct reconstruction
                    // and reduction mod the ORIGINAL L, independent of the
                    // join arithmetic above.
                    bool ok = verifySurvivorDirectGen(c, prefix, candidate, emptySuffix, freeMSB);
                    if (ok) verifiedOk++; else verifiedBad++;
                    if (ok) {
                        std::string dec = reconstructDecimalGen(c.B, prefix, candidate, emptySuffix, freeMSB);
                        if (bestDecimal.empty() || dec.size() > bestDecimal.size() ||
                            (dec.size() == bestDecimal.size() && dec > bestDecimal))
                            bestDecimal = dec;
                    }
                });
        });
        totalSurvivors += stats.survivors;
        res.totalRoots += stats.roots; res.totalLookups += stats.lookups; res.totalScans += stats.scans;
        checkRssBudget(rssBudgetKB, "certdrv-fm-per-candidate");
        long kb = peakRssKB(); if (kb > res.peakRssKBSeen) res.peakRssKBSeen = kb;

        // C3 contradiction oracle (mandatory): see peeled-path twin above.
        subsetdisc::SB_gdpSite2NoteResult(site2ShadowInfeasibleFM, totalSurvivors, "fm", candidate);
        if (site2ShadowInfeasibleFM && totalSurvivors > 0) {
            fprintf(stderr, "[GROUPDP-CONTRADICTION] site=2 path=fm base=B%d candidate=%d "
                            "restPoolSize=%zu certPos=%d totalSurvivors=%llu -- grouped-DP precheck said "
                            "INFEASIBLE but the exact search found survivor(s). This is a PROVEN "
                            "unsoundness in the grouped-DP precheck (or its position/modulus wiring). "
                            "Aborting.\n",
                    c.B, candidate, restPool.size(), certPos(), (unsigned long long)totalSurvivors);
            fflush(stderr);
            abort();
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
    // pool.size()==windowSize, via its fixed pos=certPos()-style convention). So
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

// ============================================================================
// Phase B planner (BASE-50-FULL-MODULUS-BUCKET.md Sec 6/9 Phase B;
// RADIX-BUCKET-ADMISSION-CONTROL.md Sec 4/5/8-10): enumerate BOTH plan
// families (peeled-suffix bucket, full-modulus bucket) across legal
// (NX,NY,K) splits, cost each with the doc's formulas, discard configs that
// violate the memory budget (bucketAdmissionQuery), and select the minimum-
// score feasible config. If nothing is feasible, the caller must treat this
// as AttemptStatus::Declined (Sec 9): print the trace and exit(3), never
// silently fall through as though the subset/candidate were refuted.
// ============================================================================

enum class BucketFamily { Peeled, FullModulus };
static const char *familyName(BucketFamily f) { return f == BucketFamily::Peeled ? "peeled" : "full-modulus"; }

struct PlanConfig {
    BucketFamily family = BucketFamily::Peeled;
    int NX = 0, NY = 0, K = 0;
    int P = 0;                 // leaf width: c.Pc for Peeled, P_full for FullModulus
    int lifts = 0;
    uint64_t suffixMult = 1;   // exact admissible-suffix-tuple count (Peeled); always 1 for FullModulus
    u128 Y = 0, Q = 0, roots = 0;
    long double lookups = 0, meanBucket = 0, recordChecks = 0;
    u128 projectedPeakBytes = 0;
    long double score = 0;
};

// CALIBRATED coefficients (BAND-DEPTH-CERTIFICATION.md Phase 3, fit
// 2026-07-23; REVISED 2026-07-23 same day after postcal regression showed a
// cross-K ranking regression -- see revision note below). Provenance:
// fit_planner.py parsed 402 (chosen-config, predicted-vs-actual) pairs out
// of planner_telemetry.csv's Phase-1 baseline plus every existing
// "[bucket-plan] admit ..".."[certauto] .. predicted-vs-actual" line pair
// already present in b*_certauto*.log / b*_certpos21.log / b50_certified.log
// / b60_certified.log (bases 50-64, both peeled and full-modulus families).
// Method: per-term actual/predicted RATIO MEDIANS (task-sanctioned
// alternative to full least-squares) -- robust to the handful of
// catastrophic outliers (8 of 30 unique (B,family,NX,NY,K) configs are
// >100x) that would dominate an OLS fit and wash out the well-behaved
// majority's calibration.
//
// The actual/predicted ratio for the LOOKUP and SCAN terms is STRONGLY
// K-dependent, not a single global constant (median ratio, n = number of
// (config) telemetry rows in that K bucket):
//   K=2: 115100x (n=12)     K=3: 21x (n=379)     K=4: 14x (n=11)
// A K=2 leaf's B^K modulus is too small for the mean-bucket (Y/Q)
// approximation to hold -- actual bucket occupancy runs orders of magnitude
// higher than the falling-factorial estimate. This is the root cause of the
// b58 W=21 pathological pick this task was told to fix: peeled NX=0/NY=4/
// K=2, predLookups=2160 vs actualLookups=329,253,120 (152,400x
// underestimate), wall=2394s.
//
// REVISION: the first cut applied ALL THREE per-K medians (K=2/K=3/K=4) as
// independent multipliers. Postcal regression caught the defect: K=3's n=379
// and K=4's n=11 samples aren't estimating the same thing a K=2-vs-others
// correction needs -- they're two DIFFERENT populations' typical residual,
// and dividing K=3's score by 21x while K=4's is only divided by 14x
// artificially tilted cross-K RANKING (not just each K's own accuracy)
// toward K=4, flipping b52's picked plan (NX=2/NY=4/K=3, 18s -> NX=1/NY=5/
// K=3... wait: -> a K=3 plan is unaffected in family, the actual regression
// was K=4 winning where K=3 legacy used to: b60 flipped 12s peeled NX=1/
// NY=5/K=3 -> 229s/1GB NX=0/NY=6/K=4). A per-K scale fit from small,
// disjoint samples is not sound evidence for RANKING plans against each
// other across K; it is only sound for the ONE comparison it was actually
// built from evidence for: "K=2 configs are catastrophically undercosted
// relative to everything else" (unambiguous direction, large effect, the
// specific defect this phase was tasked to fix). So: K=3 and K=4 are
// reverted to the untouched legacy constants (6.0 / 1.5, no ratio, no
// safety-margin rescaling -- byte-identical to the pre-Phase-3 code path)
// since legacy K=3/K=4 constants have empirically produced every good pick
// to date (b50/51/52/56/60's precal plans, all char-exact, all fast). Only
// K=2 keeps its fitted correction. Revisit K=3-vs-K=4 relative weighting
// only with a fit that directly models cross-K RANKING (e.g. paired
// same-subset K=3-vs-K=4 wall-time comparisons), not independent per-K
// scaling of an already-relative score.
//
// BUILD/CLEAR (Y, Q) terms have no directly-measured actual counterpart in
// the existing predicted-vs-actual telemetry (index build/clear wall-time
// isn't logged separately from lookup/scan time) -- kept at their prior
// RELATIVE weights, scaled only by the legacy 1.5x safety margin below;
// inventing an independent correction for them from no data would be
// speculation, not a fit. (These were NOT implicated in the cross-K
// regression -- they don't vary by K -- so they are unchanged by this
// revision.)
static constexpr long double LEGACY_C_BUILD  = 3.0L;
static constexpr long double LEGACY_C_CLEAR  = 0.3L;
static constexpr long double LEGACY_C_LOOKUP = 6.0L;
static constexpr long double LEGACY_C_SCAN   = 1.5L;
static constexpr long double CAL_SAFETY_MARGIN = 1.5L;
static constexpr long double CAL_RATIO_K2 = 115100.0L; // median actual/predicted, n=12 -- KEPT (unambiguous, disqualifies K=2 only)

static constexpr long double PLAN_C_BUILD = LEGACY_C_BUILD * CAL_SAFETY_MARGIN; // 4.5
static constexpr long double PLAN_C_CLEAR = LEGACY_C_CLEAR * CAL_SAFETY_MARGIN; // 0.45

// K=2's LOOKUP/SCAN coefficients carry the fitted correction (ratio x legacy
// x safety margin); K=3/K=4 are the untouched legacy constants (REVISION
// above) -- byte-identical to the pre-Phase-3 scoring for any K != 2 plan.
// K only ever takes 2, 3, or 4 (enumeratePeeledConfigs / enumerateFMConfigs
// both loop K = 2..4); any other value defensively falls back to legacy
// (uncalibrated, matches pre-Phase-3 behavior for the whole score).
static long double planCLookup(int K) {
    if (K == 2) return LEGACY_C_LOOKUP * CAL_RATIO_K2 * CAL_SAFETY_MARGIN; // 1.036e6
    return LEGACY_C_LOOKUP; // K=3, K=4, and defensive fallback: unchanged
}
static long double planCScan(int K) {
    if (K == 2) return LEGACY_C_SCAN * CAL_RATIO_K2 * CAL_SAFETY_MARGIN; // 2.5895e5
    return LEGACY_C_SCAN; // K=3, K=4, and defensive fallback: unchanged
}

static void scoreConfig(PlanConfig &pc) {
    long double mult = (long double)pc.suffixMult; // index_build_count for Peeled (rebuilt per suffix); 1 for FullModulus
    long double buildRecords = (long double)pc.Y * mult;
    long double clearWork = (long double)pc.Q * mult;
    long double cLookup = planCLookup(pc.K);
    long double cScan   = planCScan(pc.K);
    pc.score = PLAN_C_BUILD * buildRecords + PLAN_C_CLEAR * clearWork
             + cLookup * pc.lookups * mult + cScan * pc.recordChecks * mult;
}

// Derives P_full = ceil(log_B L) exactly as runCertFM's chooseWYFM does.
static int deriveP_full(const ConstantsGen &c) {
    u128 v = 1; int P = 0;
    while (v < c.L) { v *= (u128)c.B; P++; }
    return P;
}

// Peeled family (doc #23 Sec 6a): modulus L_c, suffix depth T, one bucket
// build per admissible suffix tuple -- NX = 0..W-1 (NY = W-NX >= 1), K = 2..4
// (task-specified sweep range). a = Pc + W = certPos() - T is the free-pool
// size BOTH X and Y are drawn from (buildSuffixBranchGen's freeDigits), fixed
// by the CANDIDATE_POS=certPos() invariant regardless of split.
static void enumeratePeeledConfigs(const ConstantsGen &c, uint64_t suffixMult, long rssBudgetKB,
                                    std::vector<PlanConfig> &out) {
    int W = certPos() - c.T - c.Pc;
    if (W < 1 || suffixMult == 0 || suffixMult == UINT64_MAX) return;
    int a = c.Pc + W;
    for (int NX = 0; NX < W; NX++) {
        int NY = W - NX;
        if (NY < 1) continue;
        for (int K = 2; K <= 4; K++) {
            auto yOpt = checkedFallingFactorial(a, NY);
            auto qOpt = checkedPower((u128)c.B, K);
            if (!yOpt || !qOpt) {
                fprintf(stderr, "[bucket-plan] reject peeled NX=%d NY=%d K=%d: Y_or_Q_overflow\n", NX, NY, K);
                continue;
            }
            BucketAdmissionResult adm = bucketAdmissionQuery(a, NY, K, c.B, rssBudgetKB,
                                                               sizeof(BucketRecordGen), currentRssKB());
            if (!adm.admitted) {
                fprintf(stderr, "[bucket-plan] reject peeled NX=%d NY=%d K=%d: Y=%s Q=%s reason=%s\n",
                        NX, NY, K, u128_to_string(*yOpt).c_str(), u128_to_string(*qOpt).c_str(), adm.reason.c_str());
                continue;
            }
            auto xOpt = checkedFallingFactorial(a, NX);
            auto leafOpt = checkedFallingFactorial(a - NX, K);
            if (!xOpt || !leafOpt) {
                fprintf(stderr, "[bucket-plan] reject peeled NX=%d NY=%d K=%d: work_overflow\n", NX, NY, K);
                continue;
            }
            PlanConfig pc;
            pc.family = BucketFamily::Peeled;
            pc.NX = NX; pc.NY = NY; pc.K = K; pc.P = c.Pc; pc.lifts = c.lifts;
            pc.suffixMult = suffixMult;
            pc.Y = *yOpt; pc.Q = *qOpt;
            pc.roots = (*xOpt) * (u128)c.lifts;
            pc.lookups = (long double)pc.roots * (long double)(*leafOpt);
            pc.meanBucket = pc.Q > 0 ? (long double)pc.Y / (long double)pc.Q : 0.0L;
            pc.recordChecks = pc.lookups * pc.meanBucket;
            pc.projectedPeakBytes = adm.projectedPeakBytes;
            scoreConfig(pc);
            fprintf(stderr, "[bucket-plan] admit peeled NX=%d NY=%d K=%d: Y=%s Q=%s suffixMult=%llu "
                            "projectedPeak=%s lookups=%.4Lg scans=%.4Lg score=%.4Lg\n",
                    NX, NY, K, u128_to_string(pc.Y).c_str(), u128_to_string(pc.Q).c_str(),
                    (unsigned long long)suffixMult, formatGiB(pc.projectedPeakBytes).c_str(),
                    pc.lookups, pc.recordChecks, pc.score);
            out.push_back(pc);
        }
    }
}

// Full-modulus family (doc #23 Sec 6b / Sec 4/9 Phase A): modulus L, no
// suffix loop, P + NX + NY == certPos() always. a = certPos() fixed
// regardless of split.
static void enumerateFMConfigs(const ConstantsGen &c, int P_full, long rssBudgetKB,
                                std::vector<PlanConfig> &out) {
    int W = certPos() - P_full;
    if (W < 1) return;
    auto BPowOpt = checkedPower((u128)c.B, P_full);
    if (!BPowOpt) { fprintf(stderr, "[bucket-plan] reject full-modulus: B^P_full overflow\n"); return; }
    u128 jmax = (*BPowOpt + c.L - 1) / c.L;
    int liftsFull = (int)jmax + 1;
    const int a = certPos();
    for (int NX = 0; NX < W; NX++) {
        int NY = W - NX;
        if (NY < 1) continue;
        for (int K = 2; K <= 4; K++) {
            auto yOpt = checkedFallingFactorial(a, NY);
            auto qOpt = checkedPower((u128)c.B, K);
            if (!yOpt || !qOpt) {
                fprintf(stderr, "[bucket-plan] reject full-modulus NX=%d NY=%d K=%d: Y_or_Q_overflow\n", NX, NY, K);
                continue;
            }
            BucketAdmissionResult adm = bucketAdmissionQuery(a, NY, K, c.B, rssBudgetKB,
                                                               sizeof(BucketRecordFM), currentRssKB());
            if (!adm.admitted) {
                fprintf(stderr, "[bucket-plan] reject full-modulus NX=%d NY=%d K=%d: Y=%s Q=%s reason=%s\n",
                        NX, NY, K, u128_to_string(*yOpt).c_str(), u128_to_string(*qOpt).c_str(), adm.reason.c_str());
                continue;
            }
            auto xOpt = checkedFallingFactorial(a, NX);
            auto leafOpt = checkedFallingFactorial(a - NX, K);
            if (!xOpt || !leafOpt) {
                fprintf(stderr, "[bucket-plan] reject full-modulus NX=%d NY=%d K=%d: work_overflow\n", NX, NY, K);
                continue;
            }
            PlanConfig pc;
            pc.family = BucketFamily::FullModulus;
            pc.NX = NX; pc.NY = NY; pc.K = K; pc.P = P_full; pc.lifts = liftsFull;
            pc.suffixMult = 1;
            pc.Y = *yOpt; pc.Q = *qOpt;
            pc.roots = (*xOpt) * (u128)liftsFull;
            pc.lookups = (long double)pc.roots * (long double)(*leafOpt);
            pc.meanBucket = pc.Q > 0 ? (long double)pc.Y / (long double)pc.Q : 0.0L;
            pc.recordChecks = pc.lookups * pc.meanBucket;
            pc.projectedPeakBytes = adm.projectedPeakBytes;
            scoreConfig(pc);
            fprintf(stderr, "[bucket-plan] admit full-modulus NX=%d NY=%d K=%d: Y=%s Q=%s "
                            "projectedPeak=%s lookups=%.4Lg scans=%.4Lg score=%.4Lg\n",
                    NX, NY, K, u128_to_string(pc.Y).c_str(), u128_to_string(pc.Q).c_str(),
                    formatGiB(pc.projectedPeakBytes).c_str(), pc.lookups, pc.recordChecks, pc.score);
            out.push_back(pc);
        }
    }
}

struct PlanResult {
    bool admitted = false;
    PlanConfig chosen;
};

// Prints the doc Sec 11-style trace and returns the minimum-score feasible
// config across both families, or a Declined result if neither family has
// any config that fits the memory budget.
static PlanResult planBucket(int B, int T, int Pc, int lifts, int P_full, uint64_t suffixMult,
                              long rssBudgetKB,
                              const ConstantsGen *cPeeled, const ConstantsGen *cFM) {
    char suffixMultBuf[32];
    if (suffixMult == UINT64_MAX) snprintf(suffixMultBuf, sizeof(suffixMultBuf), "DP_DECLINED");
    else snprintf(suffixMultBuf, sizeof(suffixMultBuf), "%llu", (unsigned long long)suffixMult);
    fprintf(stderr, "[bucket-plan] B=%d T=%d Pc=%d lifts=%d P_full=%d suffixMult=%s budget=%ldKB\n",
            B, T, Pc, lifts, P_full, suffixMultBuf, rssBudgetKB);
    std::vector<PlanConfig> feasible;
    if (cPeeled) enumeratePeeledConfigs(*cPeeled, suffixMult, rssBudgetKB, feasible);
    if (cFM) enumerateFMConfigs(*cFM, P_full, rssBudgetKB, feasible);

    PlanResult res;
    if (feasible.empty()) {
        fprintf(stderr, "[bucket-plan] decision=DECLINE fallback=scan (exit 3 = BUCKET_DECLINED)\n");
        return res;
    }
    auto best = std::min_element(feasible.begin(), feasible.end(),
                                  [](const PlanConfig &x, const PlanConfig &y) { return x.score < y.score; });
    PlanConfig chosen = *best;

    // Robustness rule (RADIX-BUCKET-ADMISSION-CONTROL.md Sec 7 "require a
    // clear expected win"): the uncalibrated cost model can steer the
    // minimum-score search toward a plan that is, in practice, far slower
    // than cert/certfm's long-validated legacy default (peeled, NX=2,
    // NY=W-2, K=3) -- an uncalibrated coefficient or an underweighted term
    // (e.g. suffix multiplicity, B^K clear cost) can make a bad plan LOOK
    // cheapest. Guard against that by always locating the legacy default
    // among the feasible configs (if it is itself feasible) and only
    // accepting a DIFFERENT chosen plan when its score beats the legacy
    // default's by at least this margin. This bounds certauto's worst case
    // at "roughly the legacy default," never worse than cert by more than
    // planning noise -- it cannot make certauto slower than the known-good
    // baseline by relying on an uncalibrated model.
    constexpr long double LEGACY_DEFAULT_BEAT_MARGIN = 3.0L;
    auto legacyIt = std::find_if(feasible.begin(), feasible.end(), [](const PlanConfig &p) {
        return p.family == BucketFamily::Peeled && p.NX == 2 && p.K == 3;
    });
    if (legacyIt != feasible.end()) {
        bool differs = !(chosen.family == legacyIt->family && chosen.NX == legacyIt->NX &&
                          chosen.NY == legacyIt->NY && chosen.K == legacyIt->K);
        if (differs && !(chosen.score * LEGACY_DEFAULT_BEAT_MARGIN <= legacyIt->score)) {
            fprintf(stderr, "[bucket-plan] challenger family=%s NX=%d NY=%d K=%d score=%.4Lg did not beat "
                            "legacy default (peeled NX=2 NY=%d K=3) score=%.4Lg by %.1Lgx -- keeping legacy default\n",
                    familyName(chosen.family), chosen.NX, chosen.NY, chosen.K, chosen.score,
                    legacyIt->NY, legacyIt->score, LEGACY_DEFAULT_BEAT_MARGIN);
            chosen = *legacyIt;
        }
    }

    res.admitted = true;
    res.chosen = chosen;
    fprintf(stderr, "[bucket-plan] decision=ADMIT family=%s NX=%d NY=%d K=%d P=%d Y=%s Q=%s suffixMult=%llu "
                    "projectedPeak=%s projectedLookups=%.4Lg projectedScans=%.4Lg score=%.4Lg\n",
            familyName(chosen.family), chosen.NX, chosen.NY, chosen.K, chosen.P,
            u128_to_string(chosen.Y).c_str(), u128_to_string(chosen.Q).c_str(),
            (unsigned long long)chosen.suffixMult, formatGiB(chosen.projectedPeakBytes).c_str(),
            chosen.lookups, chosen.recordChecks, chosen.score);
    return res;
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

// Bumped 64->128 (MASK-WIDENING-128BIT.md): SB_S holds up to k=|D| digit
// values, which can approach B-1 (up to 87 for the in-scope base-88
// ceiling); this is a pure capacity bump (array sizing only, see grep of
// MAXB2 below), harmless for any B<=64 already in the regression suite.
constexpr int MAXB2 = 128;
constexpr int OE_MAXSTATES2 = 2000000;

struct PPsub { u64 q; int is_nil; int t; int e; u64 q1, q2; u64 qe[7]; };

// Mirrors v13's file-scope subset state: single-threaded, one subset
// evaluated at a time, exactly as solve_base uses it.
static int    SB_B = 0;
static int    SB_S[MAXB2];
static int    SB_k = 0;
static u128   SB_LCM = 1;
// SB_setmask: digit-SET mask (subset membership), widened per
// MASK-WIDENING-128BIT.md Sec 1.1 -- the explicitly-flagged
// "1ULL << SB_S[i]" silent-bug-risk site.
static u128   SB_setmask = 0;
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
// C3 audit (MASK-WIDENING-128BIT.md): `avail` is a digit-SET mask (widened,
// fixes the `avail >> d` UB for d>=64 that a u64 avail would hit). `q` is a
// prime-power MODULUS, not a digit -- but this DP represents "which
// residues mod q are reachable" as a q-bit rotate-shift bitmask (dp[]/
// full/mm), which is ALSO silently bounded to 64 bits in the original code
// (the `q>=64` fallback disables the mask instead of representing it). For
// B up to the ~88 ceiling this epic targets, q (a prime power <= B-1) can
// exceed 64 (e.g. q=81=3^4 is a valid digit-derived modulus at B near 88,
// and B=65 itself makes q=64=2^6 the boundary case) -- so this residue
// bitmask is widened to u128 and the fallback threshold raised to 128 (q
// remains < B <= 88 << 128 throughout this epic's scope, so the fallback
// branch is not expected to ever trigger in-scope, but is kept as a
// defensive decline rather than removed).
static int SB_e2_check(u64 q, u128 avail, int m, u64 r) {
    int co = m / 2;
    u64 Bq = (u64)SB_B % q;
    u64 bm = (m & 1) ? Bq : 1;
    u64 target = ((u64)(SB_TARGET_ % q) + q - (r * bm) % q) % q;
    u64 R = 0;
    for (int d = 1; d < SB_B; d++) if ((avail >> d) & 1) R = (R + d) % q;
    u64 Bm1 = ((u64)SB_B - 1) % q;
    u64 need = (target + q - R) % q;
    u128 dp[MAXB2 / 2 + 2]; memset(dp, 0, sizeof(u128) * (co + 2));
    dp[0] = 1;
    u128 full = (q >= 128) ? ~(u128)0 : (((u128)1 << q) - 1);
    int seen = 0;
    for (int d = SB_B - 1; d >= 1; d--) {
        if (!((avail >> d) & 1)) continue;
        seen++;
        int dd = d % (int)q;
        int hi = (seen < co) ? seen : co;
        for (int c = hi; c >= 1; c--) {
            u128 mm = dp[c - 1];
            dp[c] |= dd ? (((mm << dd) | (mm >> (q - dd))) & full) : mm;
        }
    }
    u128 mask = dp[co];
    while (mask) {
        int so = ctz128(mask); mask &= mask - 1;
        if ((Bm1 * (u64)so) % q == need) return 1;
    }
    return 0;
}

// Order-e (e=3..6) CRT partition-feasibility DP, ported verbatim from
// order_e_check_m().
static int SB_order_e_check_m(u64 q, int e, u128 avail, int m, u64 target) {
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
        if (!((avail >> d) & 1)) continue;
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

// ---------------------------------------------------------------------
// BAND-DEPTH-CERTIFICATION.md Phase 2: GroupedCyclotomicDP.
//
// Correctness argument (also required by the doc's Sec 2.2 / Sec 3, and
// C2 of the review note):
//
// For subset D with lcm L (SB_LCM), a completion is a bijection of D's
// k=SB_k digits onto positions 0..k-1 (position 0 = least-significant),
// value == sum_i digit(i) * B^i. B is invertible mod every non-nilpotent
// prime power q | L (SB_pps[j].is_nil == 0 entries -- primes NOT dividing
// B; nilpotent primes have no multiplicative order and are excluded from
// this module entirely, same scope as the existing per-prime qe[e]
// checks). Fix a set of such prime powers {q_1..q_r} that ALL share the
// SAME exact multiplicative order e := ord_{q_i}(B) (checked individually
// per prime -- see SB_trueOrderOfPP below; C2-i: primes of DIFFERING
// order are never combined, because B^i mod (q_1*q_2) would then have
// period lcm(ord(q_1), ord(q_2)) != e, misaligning which positions belong
// to which weight-class and silently turning a sound per-class-count
// argument into an unsound one). Let Q_e := q_1*q_2*...*q_r (CRT product,
// capped -- C2-ii). Since each q_i has order EXACTLY e mod itself, and
// CRT order is the lcm of component orders, ord_{Q_e}(B) = e exactly, so
// B^i mod Q_e depends ONLY on (i mod e): positions split into e disjoint
// weight-classes, and any two positions in the same class are
// interchangeable (permuting digits within one weight-class does not
// change the total mod Q_e). Hence: total ≡ target (mod Q_e) is
// achievable by SOME bijection of D's digits onto 0..k-1 iff it is
// achievable by SOME digit-per-class *multiset* partition matching the
// class sizes {c_0..c_{e-1}} implied by k and e (c_j = #{i in 0..k-1 :
// i mod e == j}) -- exactly the state SB_order_e_check_m's existing DP
// already explores (it was written for a single prime power's own qe[e];
// nothing in its logic depends on q being a prime power, so it is reused
// UNCHANGED here with q := Q_e). Q_e | L by construction, so "no bijection
// achieves total ≡ 0 mod L" is refuted by "some bijection achieves total
// ≡ 0 mod Q_e" being provably impossible -- this is therefore a proven
// NECESSARY condition on D, sound to prune on (never a sufficient one; it
// never CERTIFIES a subset, only rejects some).
//
// Site 2 (candidate-window precheck) reuses the identical DP with a
// shifted target and restricted position range (see SB_site2GroupedFeasible
// below) -- same class-invariance argument, just applied to the k'=certPos()
// positions below a FIXED prefix+candidate contribution instead of all k
// positions of the whole subset.

struct SB_GDPGroup { u64 Qe; int e; int nPrimes; };
static std::vector<SB_GDPGroup> SB_gdpGroups;
static bool SB_gdpGroupsValid = false; // true once built for the CURRENT subset

// Q_e cap (C2-ii): keeps SB_order_e_check_m's own internal state-space
// gate (OE_MAXSTATES2) meaningful and prevents u64 product overflow across
// many same-order primes. A single prime power already exceeding the cap
// is left out of any group (not combined with anything) -- still exactly
// covered by the EXISTING per-prime qe[e] checks elsewhere in this file,
// so nothing is lost, only the (unsound-if-mixed) grouping is skipped.
static constexpr u64 SB_GDP_QE_CAP = 1ULL << 24;

// Exact multiplicative order of B mod the FULL prime power pp.q (not any
// sub-power) -- derived PROGRAMMATICALLY from pp's own q1/q2/qe[3..6]
// fields, which SB_build_pps computes fresh from SB_LCM every single call
// (never hardcoded/precomputed/cached across subsets -- the b39 wrong-
// Q=3120-vs-correct-80 incident, 2026-07-22, was exactly a hardcoded/stale-Q
// bug of this class; see C2-iii). Returns 0 (not groupable by this module)
// if the order is 1 (handled elsewhere via q1/digit-sum) or doesn't divide
// 6 (order > 6, or an order strictly between the checked divisors that
// this file's per-prime infrastructure doesn't track at all).
static int SB_trueOrderOfPP(const PPsub &pp) {
    if (pp.is_nil) return 0;
    if (pp.q1 == pp.q) return 0; // order 1: already handled by the digit-sum / q1 check, not this module
    if (pp.q2 == pp.q) return 2;
    if (pp.qe[3] == pp.q) return 3;
    if (pp.qe[4] == pp.q) return 4;
    if (pp.qe[5] == pp.q) return 5;
    if (pp.qe[6] == pp.q) return 6;
    return 0;
}

// Builds SB_gdpGroups for the CURRENT subset (SB_pps/SB_npps, populated by
// SB_build_pps -- called once per subset, always before this). Called from
// SB_subset_filters() whenever GROUPDP != off, and reused verbatim by the
// Site-2 precheck for the SAME subset's search calls (never rebuilt with a
// different/stale source -- C2-iii applies here too).
static void SB_buildGroupedGroups() {
    SB_gdpGroups.clear();
    for (int e = 2; e <= 6; e++) {
        u64 curQ = 1; int curN = 0;
        for (int j = 0; j < SB_npps; j++) {
            const PPsub &pp = SB_pps[j];
            if (SB_trueOrderOfPP(pp) != e) continue;
            if (pp.q > SB_GDP_QE_CAP) continue; // too big alone to ever combine soundly; skip (see cap note)
            if (curN > 0 && curQ > SB_GDP_QE_CAP / pp.q) {
                // Adding this prime would exceed the cap: close out the
                // current group and start a new one for the SAME order e.
                // Splitting is sound -- each resulting Q_e is still an
                // exact-CRT-product of same-order primes, still a proper
                // divisor of L, still a valid necessary-condition modulus.
                SB_gdpGroups.push_back({curQ, e, curN});
                curQ = 1; curN = 0;
            }
            curQ *= pp.q; curN++;
        }
        if (curN >= 1) SB_gdpGroups.push_back({curQ, e, curN});
    }
    SB_gdpGroupsValid = true;
}

// GROUPDP env var, cached: 0=off (default) 1=shadow 2=active.
static int SB_gdpModeCache = -1;
int SB_groupDPMode() {
    if (SB_gdpModeCache < 0) {
        const char *e = getenv("GROUPDP");
        std::string v = e ? e : "off";
        if (v == "off" || v.empty()) SB_gdpModeCache = 0;
        else if (v == "shadow") SB_gdpModeCache = 1;
        else if (v == "active") SB_gdpModeCache = 2;
        else {
            fprintf(stderr, "[groupdp] FATAL: GROUPDP=\"%s\" invalid; must be shadow|active|off\n", e);
            exit(1);
        }
    }
    return SB_gdpModeCache;
}

bool SB_gdpGroupsAvailable() { return SB_gdpGroupsValid; }

// Mirrors buildSuffixBranchGen's / runWrongTurnSearchFM's own prefix+
// candidate contribution C EXACTLY (same loop, same powmod convention,
// same certPos()+prefix.size() starting position) -- see those two call
// sites; this is intentionally a byte-for-byte copy of that arithmetic,
// not a "clever" reformulation, per C2-iv's do-not-re-derive-independently
// requirement.
u128 SB_computePrefixCandidateC(int B, const std::vector<int> &prefix, int candidate, u128 modulus) {
    u128 C = 0;
    int pos = certPos() + (int)prefix.size();
    for (int dd : prefix) {
        u128 term = mulmod_u128((u128)dd, powmod_u128((u128)B, (u128)pos, modulus), modulus);
        C = (C + term) % modulus;
        pos--;
    }
    C = (C + mulmod_u128((u128)candidate, powmod_u128((u128)B, (u128)certPos(), modulus), modulus)) % modulus;
    return C;
}

// Per-run counters (summary lines only -- per-subset/per-candidate spam
// would be enormous on bases with 10^8+ subsetsChecked, e.g. b56).
static u64 SB_gdpSite1FeasibleCount = 0, SB_gdpSite1InfeasibleCount = 0, SB_gdpSite1ActiveFiltered = 0;
static u64 SB_gdpSite2FeasibleCount = 0, SB_gdpSite2InfeasibleCount = 0, SB_gdpSite2ActiveSkipped = 0;
static u64 SB_gdpContradictionCount = 0; // stays 0 forever if sound; any nonzero value means abort() already fired

// C3: whenever site-2's shadow flagged a candidate infeasible, the caller
// reports the ACTUAL search outcome here for counting/telemetry (the
// caller itself performs the abort() on contradiction -- see the two
// [GROUPDP-CONTRADICTION] sites in runWrongTurnSearch/FM -- this function
// only updates counters, so it is safe to call unconditionally).
void SB_gdpSite2NoteResult(bool shadowSaidInfeasible, u64 totalSurvivorsForCandidate,
                            const char *tag, int candidate) {
    if (std::string(tag).find("active-skip") != std::string::npos) { SB_gdpSite2ActiveSkipped++; return; }
    if (shadowSaidInfeasible) {
        SB_gdpSite2InfeasibleCount++;
        if (totalSurvivorsForCandidate > 0) SB_gdpContradictionCount++; // caller already abort()s; belt-and-suspenders count
    } else {
        SB_gdpSite2FeasibleCount++;
    }
}

// Site 2 grouped-DP feasibility precheck (C2-iv). See the module-level
// comment above for the soundness argument; this just evaluates it: for
// EVERY grouped Q_e built for the current subset, reduce the caller's
// already-correctly-derived Cfixed (mod `modulus`, itself a multiple of
// every Q_e, so further reduction mod Q_e is valid by CRT) down to a
// per-Q_e target and run the SAME per-position-class DP
// (SB_order_e_check_m) used at Site 1, just with m=certPosVal positions
// and avail=restAvailMask instead of the whole-subset SB_k/SB_setmask.
int SB_site2GroupedFeasible(u128 modulus, u128 Cfixed, unsigned __int128 restAvailMask,
                             int certPosVal, const char *tag, int candidate) {
    (void)modulus;
    int mode = SB_groupDPMode();
    int feasible = 1;
    for (const SB_GDPGroup &g : SB_gdpGroups) {
        u64 qe = g.Qe;
        u64 cmod = (u64)(Cfixed % (u128)qe);
        u64 target = (qe - cmod) % qe;
        int f = SB_order_e_check_m(qe, g.e, restAvailMask, certPosVal, target);
        if (!f) {
            feasible = 0;
            fprintf(stderr, "[groupdp-site2-%s] mode=%s cand=%d Qe=%llu e=%d nPrimes=%d certPos=%d verdict=INFEASIBLE\n",
                    tag, mode == 1 ? "shadow" : "active", candidate, (unsigned long long)qe, g.e,
                    g.nPrimes, certPosVal);
        }
    }
    return feasible;
}

// Site 1 contradiction-oracle state (C1 + C3): set by SB_subset_filters()
// whenever shadow mode found a grouped-DP verdict of INFEASIBLE for the
// subset currently being filtered (which, in shadow mode, is NOT actually
// rejected -- augment, don't filter, task item 2). If THIS SAME subset's
// search subsequently succeeds (produces the certified/candidate winner),
// that is a proof the grouped-DP subset-level filter is unsound; the
// runCert/runCertFM/runCertAuto call sites check this flag right where
// they determine `certified` and abort() if it is set.
bool SB_lastShadowSubsetRejected = false;
u64 SB_lastShadowRejectQe = 0;
int SB_lastShadowRejectE = 0;

// G1/G2 gate reporting (task REPORT item "shadow logs showing verdict
// counts per base"): one summary line per terminal point of a cert run,
// not per-subset (which would be enormous -- e.g. b56 has 1.6e8
// subsetsChecked).
void SB_printGroupDPSummary(const char *tag, int B) {
    if (SB_groupDPMode() == 0) return; // off: nothing to report
    fprintf(stderr, "[groupdp-summary] %s base=%d mode=%s site1_feasible=%llu site1_infeasible=%llu "
                    "site1_active_filtered=%llu site2_feasible=%llu site2_infeasible=%llu "
                    "site2_active_skipped=%llu contradictions=%llu\n",
            tag, B, SB_groupDPMode() == 1 ? "shadow" : "active",
            (unsigned long long)SB_gdpSite1FeasibleCount, (unsigned long long)SB_gdpSite1InfeasibleCount,
            (unsigned long long)SB_gdpSite1ActiveFiltered, (unsigned long long)SB_gdpSite2FeasibleCount,
            (unsigned long long)SB_gdpSite2InfeasibleCount, (unsigned long long)SB_gdpSite2ActiveSkipped,
            (unsigned long long)SB_gdpContradictionCount);
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

    // BAND-DEPTH-CERTIFICATION.md Phase 2, Site 1: grouped-DP subset filter.
    // AUGMENTS the checks above (never replaces them, task item 2) -- this
    // subset has already passed every existing per-prime-power test by the
    // time we get here. See the GroupedCyclotomicDP module comment above
    // for the soundness argument.
    SB_gdpGroupsValid = false;
    SB_lastShadowSubsetRejected = false;
    int gdpMode = SB_groupDPMode();
    if (gdpMode != 0) {
        SB_buildGroupedGroups();
        for (const SB_GDPGroup &g : SB_gdpGroups) {
            if (g.nPrimes < 2) continue; // no NEW CRT coupling beyond the existing per-prime qe[e] checks above
            int feas = SB_order_e_check_m(g.Qe, g.e, SB_setmask, SB_k, 0);
            if (!feas) {
                SB_gdpSite1InfeasibleCount++;
                fprintf(stderr, "[groupdp-site1] mode=%s k=%d Qe=%llu e=%d nPrimes=%d verdict=INFEASIBLE\n",
                        gdpMode == 1 ? "shadow" : "active", SB_k, (unsigned long long)g.Qe, g.e, g.nPrimes);
                if (gdpMode == 2) { // active: sound filter, this subset never reaches search
                    SB_gdpSite1ActiveFiltered++;
                    return 0;
                }
                // shadow: augment-don't-filter -- keep going, remember for the C3 oracle
                SB_lastShadowSubsetRejected = true;
                SB_lastShadowRejectQe = g.Qe; SB_lastShadowRejectE = g.e;
            } else {
                SB_gdpSite1FeasibleCount++;
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------
// B60-DISCOVERY-CHURN-FIX.md: ten-rule family-cover fast enumerator.
//
// FACT (ten-rule <=> family cover): for D subseteq {1..B-1} nonempty,
//   B does NOT divide lcm(D)  <=>  D subseteq A_p for at least one prime
//                                    p | B,
// where, for p^beta || B (beta = v_p(B), the exact power of p dividing B),
//   A_p := { d in 1..B-1 : p^beta does NOT divide d }.
//
// Proof.
//   (<=) Suppose D subseteq A_p for some prime p | B with p^beta || B. Then
//   every d in D has v_p(d) < beta, so v_p(lcm(D)) = max_{d in D} v_p(d) <
//   beta. Since p^beta | B, if B | lcm(D) then v_p(lcm(D)) >= v_p(B) =
//   beta, contradiction. So B does not divide lcm(D).
//
//   (=>) Suppose B does not divide lcm(D). Write B = prod_i p_i^{beta_i}
//   (distinct primes p_i). If for EVERY i we had v_{p_i}(lcm(D)) >=
//   beta_i, then since the p_i^{beta_i} are pairwise coprime, their
//   product B would divide lcm(D) (CRT / standard divisibility-by-
//   coprime-factors argument) -- contradicting B does not divide lcm(D).
//   So there exists some i with v_{p_i}(lcm(D)) < beta_i, i.e. every d in D
//   has v_{p_i}(d) < beta_i, i.e. D subseteq A_{p_i}. QED.
//
// Consequence: the ten-rule-feasible subsets of {1..B-1} are EXACTLY the
// union, over the (<=3, for B<=64) primes p | B, of the subsets of A_p.
// SB_build_pps's ten-rule check (SB_LCM % B == 0 => reject) is exactly
// "reject iff D is in none of these A_p's" -- so an enumerator that only
// ever emits subsets D with D subseteq A_p for some p is provably
// ten-rule-feasible by construction, and never needs to visit the huge
// interior of ten-rule-INFEASIBLE k-subsets that stalled b60 discovery.
//
// SubsetEnumerator below implements EXACTLY this, selectable via env
// SUBSETENUM ("old" = today's raw C(n,k) descending-k / ascending-lex-comb
// loop, byte-identical order; "new" = the family-restricted fast path).
// Both modes are driven through this single class so the mode switch lives
// in one place; runCert/runCertFM/runCertAuto all just call next(k).
//
// NEW-mode ordering: for k descending from n=B-1 to 1, each prime family p
// (with |A_p| >= k) contributes its k-subsets of A_p in the SAME
// ascending-lex-over-kept-index order the OLD loop uses (standard
// next-combination over comb[], mapped to digits via S[i]=(B-1)-comb[i]);
// restricting that global lex order to indices lying entirely within A_p
// is exactly the lex order of k-subsets of A_p under the induced (rank-
// preserving) relabeling -- a standard fact about lexicographic order
// restricted to a subsequence that preserves relative order. The <=3
// per-family streams are then merged by a straightforward pointer merge:
// at each step, compare the families' CURRENT actual-index tuples
// lexicographically ascending, emit the minimum, and advance every family
// whose current tuple equals the emitted one (this is the dedup: a subset
// eliminating more than one prime power, e.g. both 2^2 and 5 for b60, is
// only emitted once, at its correct position in the merged order).
class SubsetEnumerator {
public:
    explicit SubsetEnumerator(int B) : B_(B), n_(B - 1), useNew_(computeUseNew()) {}

    // Fills SB_k/SB_S[0..k-1]/SB_setmask for the next candidate subset and
    // returns true; returns false once enumeration is exhausted.
    bool next(int &kOut) {
        for (;;) {
            if (!kActive_) {
                if (!beginK()) return false;
            }
            bool got = useNew_ ? stepNew() : stepOld();
            if (!got) { kActive_ = false; continue; }
            kOut = k_;
            return true;
        }
    }

private:
    static bool computeUseNew() {
        const char *e = getenv("SUBSETENUM");
        return e && std::string(e) == "new";
    }

    bool beginK() {
        if (!started_) { k_ = n_; started_ = true; }
        else { k_--; }
        if (k_ < 1) return false;
        SB_k = k_;
        kActive_ = true;
        if (useNew_) initNewFamilies(); else initOldComb();
        return true;
    }

    // ---- OLD MODE: byte-identical to the historical loop. ----
    std::vector<int> comb_;
    bool oldPending_ = false;
    void initOldComb() {
        comb_.assign(k_, 0);
        for (int i = 0; i < k_; i++) comb_[i] = i;
        oldPending_ = true;
    }
    bool stepOld() {
        if (!oldPending_) return false;
        for (int i = 0; i < k_; i++) SB_S[i] = (B_ - 1) - comb_[i];
        SB_setmask = 0;
        for (int i = 0; i < k_; i++) SB_setmask |= (u128)1 << SB_S[i];
        int i = k_ - 1;
        while (i >= 0 && comb_[i] == n_ - k_ + i) i--;
        if (i < 0) oldPending_ = false;
        else { comb_[i]++; for (int j = i + 1; j < k_; j++) comb_[j] = comb_[j - 1] + 1; }
        return true;
    }

    // ---- NEW MODE: ten-rule family fast path. ----
    struct FamStream {
        std::vector<int> allowed; // ascending actual indices (0..n_-1) in A_p
        std::vector<int> pos;     // combination in position-space, size k_
        bool exhausted = true;
    };
    std::vector<FamStream> fams_;
    std::vector<std::pair<int,int>> primeFactors_; // (p, beta), computed once
    bool pfComputed_ = false;

    void computePrimeFactorsOnce() {
        if (pfComputed_) return;
        pfComputed_ = true;
        int m = B_;
        for (int p = 2; p <= m; p++) {
            if (m % p != 0) continue;
            int beta = 0;
            while (m % p == 0) { m /= p; beta++; }
            primeFactors_.push_back({p, beta});
        }
    }

    void initNewFamilies() {
        computePrimeFactorsOnce();
        fams_.clear();
        for (auto &pb : primeFactors_) {
            int p = pb.first, beta = pb.second;
            long long pw = 1; for (int e = 0; e < beta; e++) pw *= p;
            std::vector<int> allowed;
            allowed.reserve(n_);
            for (int i = 0; i < n_; i++) {
                int d = (B_ - 1) - i;
                if (d % pw != 0) allowed.push_back(i); // d NOT eliminated => in A_p
            }
            if ((int)allowed.size() < k_) continue; // no k-subset of A_p
            FamStream fs;
            fs.allowed = std::move(allowed);
            fs.pos.resize(k_);
            for (int i = 0; i < k_; i++) fs.pos[i] = i;
            fs.exhausted = false;
            fams_.push_back(std::move(fs));
        }
    }

    static void curTuple(const FamStream &fs, int k, std::vector<int> &out) {
        out.resize(k);
        for (int i = 0; i < k; i++) out[i] = fs.allowed[fs.pos[i]];
    }
    void advanceFam(FamStream &fs) {
        int m = (int)fs.allowed.size();
        int i = k_ - 1;
        while (i >= 0 && fs.pos[i] == m - k_ + i) i--;
        if (i < 0) { fs.exhausted = true; return; }
        fs.pos[i]++;
        for (int j = i + 1; j < k_; j++) fs.pos[j] = fs.pos[j - 1] + 1;
    }

    bool stepNew() {
        if (fams_.empty()) return false;
        std::vector<int> tup, best;
        int bestIdx = -1;
        for (size_t f = 0; f < fams_.size(); f++) {
            if (fams_[f].exhausted) continue;
            curTuple(fams_[f], k_, tup);
            if (bestIdx < 0 || tup < best) { best = tup; bestIdx = (int)f; }
        }
        if (bestIdx < 0) return false;
        for (int i = 0; i < k_; i++) SB_S[i] = (B_ - 1) - best[i];
        SB_setmask = 0;
        for (int i = 0; i < k_; i++) SB_setmask |= (u128)1 << SB_S[i];
        // Dedup: advance every family currently AT the emitted tuple (a
        // subset can eliminate more than one prime power at once).
        for (auto &fs : fams_) {
            if (fs.exhausted) continue;
            curTuple(fs, k_, tup);
            if (tup == best) advanceFam(fs);
        }
        return true;
    }

    int B_, n_;
    bool useNew_;
    bool started_ = false;
    bool kActive_ = false;
    int k_ = 0;
};

static long long SB_orderLogLines = 0;
static bool SB_orderLogTruncated = false;

static bool SB_debugOrderEnabled() {
    static int v = -1;
    if (v < 0) v = getenv("DEBUG_SUBSET_ORDER") ? 1 : 0;
    return v != 0;
}
static bool SB_isNewEnumMode() {
    static int v = -1;
    if (v < 0) { const char *e = getenv("SUBSETENUM"); v = (e && std::string(e) == "new") ? 1 : 0; }
    return v != 0;
}

// Shared wrapper around SB_build_pps(), called from the single gate site
// that already exists identically in runCert/runCertFM/runCertAuto
// (`if (subsetdisc::SB_build_pps() && subsetdisc::SB_subset_filters())`).
// Adds:
//  (a) DEBUG_SUBSET_ORDER logging of every subset that PASSES the ten-rule
//      (one line: k + descending digit list), capped at 200000 lines then
//      a single truncation marker; works identically in both SUBSETENUM
//      modes since both route every candidate through this function.
//  (b) in SUBSETENUM=new mode only: a loud abort if SB_build_pps ever
//      rejects a subset the new enumerator emitted. By the family-cover
//      fact proved above, that can never happen for a correct
//      implementation -- a rejection here means the enumerator (not the
//      subset) is buggy, so this aborts rather than silently skipping.
static int SB_checkAndLogOrder(int k) {
    int pass = SB_build_pps();
    if (SB_debugOrderEnabled() && pass) {
        if (SB_orderLogLines < 200000) {
            fprintf(stderr, "[subsetorder] k=%d D=", k);
            for (int i = 0; i < k; i++) fprintf(stderr, "%d%s", SB_S[i], (i + 1 < k) ? "," : "");
            fprintf(stderr, "\n");
            SB_orderLogLines++;
        } else if (!SB_orderLogTruncated) {
            fprintf(stderr, "[subsetorder] TRUNCATED at 200000 lines\n");
            SB_orderLogTruncated = true;
        }
    }
    if (!pass && SB_isNewEnumMode()) {
        fprintf(stderr, "[subsetdisc] FATAL: SUBSETENUM=new emitted a subset that FAILS the ten-rule "
                        "(k=%d) -- this violates the family-cover invariant the enumerator relies on; "
                        "this is an enumerator bug, not a legitimately-infeasible subset. Aborting. D=", k);
        for (int i = 0; i < k; i++) fprintf(stderr, "%d%s", SB_S[i], (i + 1 < k) ? "," : "");
        fprintf(stderr, "\n");
        fflush(stderr);
        abort();
    }
    return pass;
}

} // namespace subsetdisc

// MASK-WIDENING-128BIT.md startup guard, shared by runCert/runCertFM/
// runCertAuto (task item 3/4/5): computes the C1 checked lcm(1..B-1) once
// (fatal-exits cleanly, never returns a wrapped value, upper-bounds every
// subset's own L for the rest of the run), derives P_full = ceil(log_B L)
// from it, configures dynamic CERTPOS (Sec 4), and prints the one-time
// record-size confirmation (task item 5).
static void certStartupGuard(int B) {
    static bool sizeofPrinted = false;
    u128 Lfull = checkedLcmUpToOrDie(B - 1);
    int P_full = 0;
    { u128 v = 1; while (v < Lfull) { v *= (u128)B; P_full++; } }
    configureCertPosForBase(B, P_full);
    if (!sizeofPrinted) {
        fprintf(stderr, "[setup] sizeof(BucketRecordGen)=%zu sizeof(BucketRecordFM)=%zu\n",
                sizeof(BucketRecordGen), sizeof(BucketRecordFM));
        assert(sizeof(BucketRecordGen) == 32);
        assert(sizeof(BucketRecordFM) == 48);
        sizeofPrinted = true;
    }
}

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
    certStartupGuard(B);
    fprintf(stderr, "[cert] base=%d, n=%d, autonomous subset+divergence discovery starting "
                     "(subsetdisc: solve_base-style descending-lex enumeration)\n", B, n);

    // Reasonable NY/K heuristics, matching what won tonight's head-to-head:
    // aim for B^NY in the low millions, K=3.
    // buildSuffixBranchGen hardcodes the candidate at ABSOLUTE digit-position
    // certPos() (default 20; pos = certPos() + prefix.size();
    // powmod_u128(B, certPos(), Lc) for the candidate term) -- i.e. it hard-
    // requires exactly certPos() digit-positions below the candidate:
    // T (suffix) + Pc (leaf window) + NX + NY (free digits) == certPos(),
    // always, for every subset. The old "aim for B^NY in the low millions"
    // heuristic chose NY independently of T/Pc and only coincidentally
    // matched this invariant for some subsets (e.g. base 48's T=3,Pc=11);
    // for others (e.g. base 49's T=1,Pc=12) it did not, silently producing
    // wrong position arithmetic and false refutations. NY is now DERIVED
    // from the invariant directly, with NX shrunk first if T+Pc alone
    // already leaves little room.
    const int CANDIDATE_POS = certPos();
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
    // subsetsChecked semantics: in SUBSETENUM=old (default) this counts
    // every raw combination visited (as before). In SUBSETENUM=new it
    // counts only ten-rule-feasible candidates the fast enumerator emits
    // (a much smaller, pre-filtered stream) -- documented relabeling per
    // B60-DISCOVERY-CHURN-FIX.md risk #5, not a regression.
    subsetdisc::SubsetEnumerator subsetEnum(B);
    int k;
    while (subsetEnum.next(k)) {
        subsetsChecked++;
        {
            if (subsetdisc::SB_checkAndLogOrder(k) && subsetdisc::SB_subset_filters()) {
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
                        // C1/C3 contradiction oracle: this subset was shadow-flagged
                        // by the Site-1 grouped-DP filter as infeasible, yet its own
                        // search just produced a certified completion -- proof the
                        // filter (or its wiring) is unsound. Never silent.
                        if (certified && subsetdisc::SB_lastShadowSubsetRejected) {
                            fprintf(stderr, "[GROUPDP-CONTRADICTION] site=1 path=cert base=%d k=%d Qe=%llu e=%d "
                                            "-- shadow-mode Site-1 grouped-DP filter said this subset was "
                                            "INFEASIBLE, but its search just produced a certified completion. "
                                            "Aborting.\n",
                                    B, k, (unsigned long long)subsetdisc::SB_lastShadowRejectQe, subsetdisc::SB_lastShadowRejectE);
                            fflush(stderr);
                            abort();
                        }
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

                            // CHANGE 1 (b50 audit, honest refute-and-descend labeling):
                            // when the winning subset was NOT the first one scanned,
                            // one or more earlier-scanned subsets were refuted only
                            // within the fixed CANDIDATE_POS-position window + a single
                            // feasibility-adjusted prefix (buildFeasiblePrefix), which
                            // does not exhaustively prove no completion exists for
                            // them -- so the maximality claim behind "CERTIFICATION" is
                            // unsupported. subsetsScanned==1 (every decade-sweep base
                            // to date) is unaffected and stays byte-identical.
                            if (subsetsScanned > 1) {
                                fprintf(stderr, "[cert] base=%d STRONG CANDIDATE (direct-verified completion; "
                                                "NOT certified-maximal: %lld earlier-scanned subset(s) refuted "
                                                "only within the fixed %d-position window -- see RESULTS.md b50 "
                                                "audit)\n", B, subsetsScanned - 1, CANDIDATE_POS + 1);
                                double totalWall = std::chrono::duration<double>(clock::now() - tAll0).count();
                                fprintf(stderr, "[cert] base=%d total autonomous-search wall=%.3fs\n", B, totalWall);
                                subsetdisc::SB_printGroupDPSummary("cert", B);
                                exit(4); // CANDIDATE_ONLY: direct-verified but not proven maximal
                            }
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
                            subsetdisc::SB_printGroupDPSummary("cert", B);
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
        }
    }
    fprintf(stderr, "[cert] base=%d: FAILED to find a working (subset, prefix) hypothesis within bounded search "
                    "[subsetsChecked=%lld subsetsScanned=%lld]\n", B, subsetsChecked, subsetsScanned);
    subsetdisc::SB_printGroupDPSummary("cert", B);
}

// Full-modulus driver (BASE-50-FULL-MODULUS-BUCKET.md Sec 9 Phase A): exact
// same subset-discovery loop, prefix-construction, and CERTIFIED/Declined-
// vs-Refuted soundness discipline as runCert, but replaces the per-suffix
// peeled join with runWrongTurnSearchFM (ONE join directly modulo the
// original L, no suffix enumeration). Selected via a separate CLI mode
// (`certfm`) so the default `cert` path stays bit-identical -- this is
// purely additive.
static void runCertFM(int B, const char *expectedDecimalOrNull, long rssBudgetKB, double capSeconds1800, double capSeconds5400) {
    using clock = std::chrono::steady_clock;
    auto tAll0 = clock::now();
    int n = B - 1;
    certStartupGuard(B);
    fprintf(stderr, "[certfm] base=%d, n=%d, autonomous subset+divergence discovery starting "
                     "(full-modulus bucket join, subsetdisc: solve_base-style descending-lex enumeration)\n", B, n);

    // Same ABSOLUTE digit-position convention as runCert's chooseWY/
    // buildSuffixBranchGen (candidate fixed at position certPos()), but with T
    // replaced by 0 (no suffix in the full-modulus decomposition) and Pc
    // replaced by P_full = ceil(log_B L): P + NY + NX == CANDIDATE_POS,
    // always. NY is derived from the invariant directly, with NX shrunk
    // first if P alone already leaves little room -- mirrors chooseWY
    // exactly, just with (0, P_full) standing in for (T, Pc).
    const int CANDIDATE_POS = certPos();
    auto chooseWYFM = [&](const ConstantsGen &c, int &outP) -> std::pair<int,int> {
        u128 v = 1; int P = 0;
        while (v < c.L) { v *= (u128)c.B; P++; }
        outP = P;
        int NX = 2;
        int NY = CANDIDATE_POS - P - NX;
        if (NY < 1) { NX = 1; NY = CANDIDATE_POS - P - NX; }
        if (NY < 1) { NX = 0; NY = CANDIDATE_POS - P - NX; }
        return std::make_pair(NX, NY);
    };

    subsetdisc::SB_B = B;
    long long subsetsChecked = 0, subsetsScanned = 0;

    subsetdisc::SubsetEnumerator subsetEnum(B);
    int k;
    while (subsetEnum.next(k)) {
        subsetsChecked++;
        {
            if (subsetdisc::SB_checkAndLogOrder(k) && subsetdisc::SB_subset_filters()) {
                subsetsScanned++;
                std::vector<int> D(subsetdisc::SB_S, subsetdisc::SB_S + k);

                if (getenv("DEBUG_SUBSETDISC")) {
                    std::vector<bool> present(B, false);
                    for (int d : D) present[d] = true;
                    fprintf(stderr, "[dbg-fm] k=%d dropped=", k);
                    for (int d = 1; d < B; d++) if (!present[d]) fprintf(stderr, "%d,", d);
                    fprintf(stderr, "\n");
                }

                ConstantsGen c = deriveConstantsGen(B, D);
                if (c.ok) {
                    int P = 0;
                    auto [NX, NY] = chooseWYFM(c, P);
                    if (P < CANDIDATE_POS) {
                        // buildFeasiblePrefix only consults c.T/c.Pc/c.Lnil (via
                        // countAdmissibleSuffixTuplesGen, which with T=0
                        // trivially always admits the empty tuple -- exactly
                        // right for "no suffix" here); reuse it unmodified via
                        // a T=0/Pc=P_full view of this subset's constants.
                        ConstantsGen cFM = c;
                        cFM.T = 0; cFM.Pc = P;
                        std::vector<int> prefix, pool;
                        bool feas = buildFeasiblePrefix(cFM, NX + NY, 3, prefix, pool);
                        if (getenv("DEBUG_SUBSETDISC") && !feas) fprintf(stderr, "[dbg-fm]   buildFeasiblePrefix FAILED (P=%d NX=%d NY=%d)\n", P, NX, NY);
                        if (feas) {
                            AttemptResult ar = runWrongTurnSearchFM(c, prefix, pool, P, NX, NY, 3, rssBudgetKB);
                            if (getenv("DEBUG_SUBSETDISC")) fprintf(stderr, "[dbg-fm]   success=%d winCand=%d refuted=%d survivors=%llu verifiedOk=%llu verifiedBad=%llu prefixLen=%zu poolLen=%zu P=%d NX=%d NY=%d\n",
                                ar.success, ar.winningCandidate, ar.wrongTurnsRefuted, (unsigned long long)ar.totalSurvivors,
                                (unsigned long long)ar.verifiedOk, (unsigned long long)ar.verifiedBad, prefix.size(), pool.size(), P, NX, NY);

                            double elapsed = std::chrono::duration<double>(clock::now() - tAll0).count();
                            if (elapsed > capSeconds1800 && elapsed < capSeconds1800 + 5) {
                                fprintf(stderr, "[certfm] checkpoint at %.0fs: still searching (k=%d, subsetsChecked=%lld, "
                                                "subsetsScanned=%lld)\n", elapsed, k, subsetsChecked, subsetsScanned);
                            }
                            if (elapsed > capSeconds5400) {
                                fprintf(stderr, "[certfm] FATAL: exceeded %.0fs cap, aborting search\n", capSeconds5400);
                                return;
                            }

                            // Same Declined-vs-Refuted-safe "certified completion"
                            // discipline as runCert (RADIX-BUCKET-ADMISSION-CONTROL.md
                            // Sec 9): success alone is not sufficient.
                            bool certified = ar.success && ar.verifiedOk >= 1 && ar.verifiedBad == 0;
                            // C1/C3 contradiction oracle: see runCert's identical comment.
                            if (certified && subsetdisc::SB_lastShadowSubsetRejected) {
                                fprintf(stderr, "[GROUPDP-CONTRADICTION] site=1 path=certfm base=%d k=%d Qe=%llu e=%d "
                                                "-- shadow-mode Site-1 grouped-DP filter said this subset was "
                                                "INFEASIBLE, but its search just produced a certified completion. "
                                                "Aborting.\n",
                                        B, k, (unsigned long long)subsetdisc::SB_lastShadowRejectQe, subsetdisc::SB_lastShadowRejectE);
                                fflush(stderr);
                                abort();
                            }
                            if (ar.success && !certified && getenv("DEBUG_SUBSETDISC")) {
                                fprintf(stderr, "[certfm] base=%d: subset |D|=%d had %llu survivor(s) but NONE "
                                                "direct-verified true (verified %llu OK / %llu FAILED) -- not a "
                                                "certified completion, continuing\n",
                                        B, k, (unsigned long long)ar.totalSurvivors,
                                        (unsigned long long)ar.verifiedOk, (unsigned long long)ar.verifiedBad);
                            }
                            if (certified) {
                                fprintf(stderr, "[certfm] base=%d: FOUND with |D|=%d (dropped %d digit(s)) prefixLen=%zu "
                                                "poolSize=%zu winningCandidate=%d wrongTurnsRefuted=%d survivors=%llu "
                                                "(verified %llu OK / %llu FAILED) wall=%.3fs peakRSS~%ldKB "
                                                "[subsetsChecked=%lld subsetsScanned=%lld]\n",
                                        B, k, n - k, prefix.size(), pool.size(), ar.winningCandidate,
                                        ar.wrongTurnsRefuted, (unsigned long long)ar.totalSurvivors,
                                        (unsigned long long)ar.verifiedOk, (unsigned long long)ar.verifiedBad,
                                        ar.wallSeconds, ar.peakRssKBSeen, subsetsChecked, subsetsScanned);
                                fprintf(stderr, "[certfm] base=%d: maximum value (%zu digits): %s\n",
                                        B, ar.maxDecimal.size(), ar.maxDecimal.c_str());

                                // CHANGE 1 (b50 audit): see runCert's identical comment.
                                if (subsetsScanned > 1) {
                                    fprintf(stderr, "[certfm] base=%d STRONG CANDIDATE (direct-verified completion; "
                                                    "NOT certified-maximal: %lld earlier-scanned subset(s) refuted "
                                                    "only within the fixed %d-position window -- see RESULTS.md b50 "
                                                    "audit)\n", B, subsetsScanned - 1, CANDIDATE_POS + 1);
                                    double totalWall = std::chrono::duration<double>(clock::now() - tAll0).count();
                                    fprintf(stderr, "[certfm] base=%d total autonomous-search wall=%.3fs\n", B, totalWall);
                                    subsetdisc::SB_printGroupDPSummary("certfm", B);
                                    exit(4); // CANDIDATE_ONLY: direct-verified but not proven maximal
                                }
                                bool exact = expectedDecimalOrNull && ar.maxDecimal == expectedDecimalOrNull;
                                if (expectedDecimalOrNull) {
                                    fprintf(stderr, "[certfm] base=%d CERTIFICATION (matches known target): %s\n",
                                            B, exact ? "PASS" : "FAIL");
                                } else {
                                    fprintf(stderr, "[certfm] base=%d CERTIFICATION (direct-verified, no external target "
                                                    "to compare): PASS\n", B);
                                }
                                double totalWall = std::chrono::duration<double>(clock::now() - tAll0).count();
                                fprintf(stderr, "[certfm] base=%d total autonomous-search wall=%.3fs\n", B, totalWall);
                                subsetdisc::SB_printGroupDPSummary("certfm", B);
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    fprintf(stderr, "[certfm] base=%d: FAILED to find a working (subset, prefix) hypothesis within bounded search "
                    "[subsetsChecked=%lld subsetsScanned=%lld]\n", B, subsetsChecked, subsetsScanned);
    subsetdisc::SB_printGroupDPSummary("certfm", B);
}

// Autonomous planner-driven driver (BASE-50-FULL-MODULUS-BUCKET.md Sec 9
// Phase B): same subset-discovery loop and Declined-vs-Refuted-safe
// certification discipline as runCert/runCertFM, but instead of a
// hardcoded chooseWY/chooseWYFM split, calls planBucket() for each
// surviving subset to pick the cheapest FEASIBLE (family, NX, NY, K) from
// BOTH the peeled and full-modulus families before running any search.
//
// Sec 9's fallback semantics are load-bearing here: if planBucket() finds
// NO feasible config for a subset, that is AttemptStatus::Declined, which
// "must propagate out of runWrongTurnSearch and runCert. It must not ...
// advance to another digit subset as though the current subset were
// impossible." So an outright decline here exits the WHOLE process with
// status 3 (BUCKET_DECLINED) immediately -- exactly the propagation
// discipline the existing bucketAdmissionGate already enforces for a single
// config, just now reached only after every legal split in both families
// has been tried and found wanting. This mirrors cert/certfm's own
// existing behavior (their first infeasible config aborts the whole run
// via bucketAdmissionGate's exit(3)), just with a smarter search before
// giving up.

// BAND-DEPTH-CERTIFICATION.md Phase 1A telemetry: append-only CSV row per
// completed scan at the certauto predicted-vs-actual print site (soundness-
// neutral -- pure instrumentation, no effect on any exit code, gate, prune,
// or enumeration decision). Header written once if the file does not yet
// exist; every subsequent call appends one row. Columns: B,|D|,family,NX,NY,
// K,suffixMult,Y,Q,predLookups,actualLookups,predScans,actualScans,
// wallSeconds,certposW,timestamp (ISO-8601 UTC). Best-effort: if the file
// can't be opened, log to stderr and continue -- telemetry must never abort
// or alter a certification run.
static void appendPlannerTelemetryCSV(int B, int Dsize, const PlanConfig &pl,
                                       const AttemptResult &ar, int certposW) {
    static const char *kPath = "planner_telemetry.csv";
    bool exists = (access(kPath, F_OK) == 0);
    FILE *f = fopen(kPath, "a");
    if (!f) {
        fprintf(stderr, "[telemetry] WARNING: could not open %s for append (errno=%d) -- skipping row\n",
                kPath, errno);
        return;
    }
    if (!exists) {
        fprintf(f, "B,|D|,family,NX,NY,K,suffixMult,Y,Q,predLookups,actualLookups,predScans,actualScans,"
                   "wallSeconds,certposW,timestamp\n");
    }
    time_t now = time(nullptr);
    struct tm tmUtc;
    gmtime_r(&now, &tmUtc);
    char tsBuf[32];
    strftime(tsBuf, sizeof(tsBuf), "%Y-%m-%dT%H:%M:%SZ", &tmUtc);
    fprintf(f, "%d,%d,%s,%d,%d,%d,%llu,%s,%s,%.6Lg,%llu,%.6Lg,%llu,%.6f,%d,%s\n",
            B, Dsize, familyName(pl.family), pl.NX, pl.NY, pl.K,
            (unsigned long long)pl.suffixMult,
            u128_to_string(pl.Y).c_str(), u128_to_string(pl.Q).c_str(),
            pl.lookups, (unsigned long long)ar.totalLookups,
            pl.recordChecks, (unsigned long long)ar.totalScans,
            ar.wallSeconds, certposW, tsBuf);
    fclose(f);
}

static void runCertAuto(int B, const char *expectedDecimalOrNull, long rssBudgetKB, double capSeconds1800, double capSeconds5400) {
    using clock = std::chrono::steady_clock;
    auto tAll0 = clock::now();
    int n = B - 1;
    certStartupGuard(B);
    fprintf(stderr, "[certauto] base=%d, n=%d, autonomous subset+divergence+PLAN discovery starting "
                     "(subsetdisc: solve_base-style descending-lex enumeration; planner: BASE-50-FULL-MODULUS-BUCKET.md Sec 9 Phase B)\n", B, n);

    selfTestSuffixDP();

    const int CANDIDATE_POS = certPos();
    subsetdisc::SB_B = B;
    long long subsetsChecked = 0, subsetsScanned = 0;

    subsetdisc::SubsetEnumerator subsetEnum(B);
    int k;
    while (subsetEnum.next(k)) {
        subsetsChecked++;
        {
            if (subsetdisc::SB_checkAndLogOrder(k) && subsetdisc::SB_subset_filters()) {
                subsetsScanned++;
                std::vector<int> D(subsetdisc::SB_S, subsetdisc::SB_S + k);

                if (getenv("DEBUG_SUBSETDISC")) {
                    std::vector<bool> present(B, false);
                    for (int d : D) present[d] = true;
                    fprintf(stderr, "[dbg-auto] k=%d dropped=", k);
                    for (int d = 1; d < B; d++) if (!present[d]) fprintf(stderr, "%d,", d);
                    fprintf(stderr, "\n");
                }

                ConstantsGen c = deriveConstantsGen(B, D);
                if (!c.ok) continue;

                {
                    int W_peeled = CANDIDATE_POS - c.T - c.Pc;
                    int P_full = deriveP_full(c);
                    int W_fm = CANDIDATE_POS - P_full;
                    bool feasPeeledShape = W_peeled >= 1;
                    bool feasFMShape = W_fm >= 1;
                    if (!feasPeeledShape && !feasFMShape) continue;

                    std::vector<int> prefixP, poolP, prefixF, poolF;
                    bool feasPeeled = feasPeeledShape && buildFeasiblePrefix(c, W_peeled, 3, prefixP, poolP);
                    ConstantsGen cFM = c; cFM.T = 0; cFM.Pc = P_full;
                    bool feasFM = feasFMShape && buildFeasiblePrefix(cFM, W_fm, 3, prefixF, poolF);
                    if (!feasPeeled && !feasFM) continue;

                    // Representative-candidate suffix multiplicity for the
                    // peeled family's cost estimate: mirror the ACTUAL
                    // search order (runWrongTurnSearch tries pool digits
                    // largest-first, silently skipping any digit whose
                    // restPool admits zero suffix tuples -- doc's "not a
                    // real refutation, just infeasible bookkeeping"). Using
                    // the single largest digit unconditionally can pick a
                    // digit with suffixMult==0 even though the family is
                    // perfectly viable for the next-largest digit (this bit
                    // base 48: candidate 21 has suffixMult==0 for its
                    // restPool while the actual winner, candidate 20, does
                    // not) -- walk the same descending order and take the
                    // first nonzero count.
                    uint64_t suffixMult = 1;
                    if (feasPeeled) {
                        std::vector<int> sortedPoolP = poolP;
                        std::sort(sortedPoolP.begin(), sortedPoolP.end(), std::greater<int>());
                        suffixMult = 0;
                        for (int repCandidate : sortedPoolP) {
                            std::vector<int> restPool;
                            for (int d : poolP) if (d != repCandidate) restPool.push_back(d);
                            uint64_t cnt = countAdmissibleSuffixTuplesDP(restPool, c.T, c.B, c.Lnil);
                            if (cnt == UINT64_MAX) { suffixMult = UINT64_MAX; break; } // DP declined; stop guessing
                            if (cnt > 0) { suffixMult = cnt; break; }
                        }
                    }

                    PlanResult plan = planBucket(B, c.T, c.Pc, c.lifts, P_full, suffixMult, rssBudgetKB,
                                                  feasPeeled ? &c : nullptr, feasFM ? &cFM : nullptr);
                    if (!plan.admitted) {
                        fprintf(stderr, "[certauto] base=%d: |D|=%d bucket planner DECLINED (no feasible "
                                        "peeled or full-modulus config within budget=%ldKB) -- this is a "
                                        "resource-policy DECLINE, not a refutation. fallback=scan\n",
                                B, k, rssBudgetKB);
                        double totalWall = std::chrono::duration<double>(clock::now() - tAll0).count();
                        fprintf(stderr, "[certauto] base=%d total autonomous-search wall=%.3fs [subsetsChecked=%lld subsetsScanned=%lld]\n",
                                B, totalWall, subsetsChecked, subsetsScanned);
                        subsetdisc::SB_printGroupDPSummary("certauto", B);
                        exit(3);
                    }

                    AttemptResult ar;
                    const PlanConfig &pl = plan.chosen;
                    if (pl.family == BucketFamily::Peeled) {
                        ar = runWrongTurnSearch(c, prefixP, poolP, pl.NX, pl.NY, pl.K, rssBudgetKB, /*useCurrentRssForAvailable=*/true);
                    } else {
                        ar = runWrongTurnSearchFM(c, prefixF, poolF, pl.P, pl.NX, pl.NY, pl.K, rssBudgetKB, /*useCurrentRssForAvailable=*/true);
                    }

                    fprintf(stderr, "[certauto]   predicted-vs-actual: family=%s predLookups=%.4Lg actualLookups=%llu "
                                    "predScans=%.4Lg actualScans=%llu actualRoots=%llu wall=%.3fs\n",
                            familyName(pl.family), pl.lookups, (unsigned long long)ar.totalLookups,
                            pl.recordChecks, (unsigned long long)ar.totalScans, (unsigned long long)ar.totalRoots,
                            ar.wallSeconds);

                    // Phase 1A telemetry (BAND-DEPTH-CERTIFICATION.md): append-only,
                    // always-on, one row per completed scan. Pure instrumentation --
                    // no effect on any exit code, gate, prune, or enumeration decision.
                    appendPlannerTelemetryCSV(B, k, pl, ar, CANDIDATE_POS);

                    if (getenv("DEBUG_SUBSETDISC")) fprintf(stderr, "[dbg-auto]   success=%d winCand=%d refuted=%d survivors=%llu verifiedOk=%llu verifiedBad=%llu family=%s NX=%d NY=%d K=%d\n",
                        ar.success, ar.winningCandidate, ar.wrongTurnsRefuted, (unsigned long long)ar.totalSurvivors,
                        (unsigned long long)ar.verifiedOk, (unsigned long long)ar.verifiedBad, familyName(pl.family), pl.NX, pl.NY, pl.K);

                    double elapsed = std::chrono::duration<double>(clock::now() - tAll0).count();
                    if (elapsed > capSeconds1800 && elapsed < capSeconds1800 + 5) {
                        fprintf(stderr, "[certauto] checkpoint at %.0fs: still searching (k=%d, subsetsChecked=%lld, subsetsScanned=%lld)\n",
                                elapsed, k, subsetsChecked, subsetsScanned);
                    }
                    if (elapsed > capSeconds5400) {
                        fprintf(stderr, "[certauto] FATAL: exceeded %.0fs cap, aborting search\n", capSeconds5400);
                        return;
                    }

                    bool certified = ar.success && ar.verifiedOk >= 1 && ar.verifiedBad == 0;
                    // C1/C3 contradiction oracle: see runCert's identical comment.
                    if (certified && subsetdisc::SB_lastShadowSubsetRejected) {
                        fprintf(stderr, "[GROUPDP-CONTRADICTION] site=1 path=certauto base=%d k=%d Qe=%llu e=%d "
                                        "-- shadow-mode Site-1 grouped-DP filter said this subset was "
                                        "INFEASIBLE, but its search just produced a certified completion. "
                                        "Aborting.\n",
                                B, k, (unsigned long long)subsetdisc::SB_lastShadowRejectQe, subsetdisc::SB_lastShadowRejectE);
                        fflush(stderr);
                        abort();
                    }
                    if (ar.success && !certified && getenv("DEBUG_SUBSETDISC")) {
                        fprintf(stderr, "[certauto] base=%d: subset |D|=%d had %llu survivor(s) but NONE "
                                        "direct-verified true (verified %llu OK / %llu FAILED) -- not a "
                                        "certified completion, continuing\n",
                                B, k, (unsigned long long)ar.totalSurvivors,
                                (unsigned long long)ar.verifiedOk, (unsigned long long)ar.verifiedBad);
                    }
                    if (certified) {
                        fprintf(stderr, "[certauto] base=%d: FOUND with |D|=%d (dropped %d digit(s)) family=%s NX=%d NY=%d K=%d "
                                        "winningCandidate=%d wrongTurnsRefuted=%d survivors=%llu "
                                        "(verified %llu OK / %llu FAILED) wall=%.3fs peakRSS~%ldKB "
                                        "[subsetsChecked=%lld subsetsScanned=%lld]\n",
                                B, k, n - k, familyName(pl.family), pl.NX, pl.NY, pl.K, ar.winningCandidate,
                                ar.wrongTurnsRefuted, (unsigned long long)ar.totalSurvivors,
                                (unsigned long long)ar.verifiedOk, (unsigned long long)ar.verifiedBad,
                                ar.wallSeconds, ar.peakRssKBSeen, subsetsChecked, subsetsScanned);
                        fprintf(stderr, "[certauto] base=%d: maximum value (%zu digits): %s\n",
                                B, ar.maxDecimal.size(), ar.maxDecimal.c_str());

                        // CHANGE 1 (b50 audit): see runCert's identical comment.
                        if (subsetsScanned > 1) {
                            fprintf(stderr, "[certauto] base=%d STRONG CANDIDATE (direct-verified completion; "
                                            "NOT certified-maximal: %lld earlier-scanned subset(s) refuted "
                                            "only within the fixed %d-position window -- see RESULTS.md b50 "
                                            "audit)\n", B, subsetsScanned - 1, CANDIDATE_POS + 1);
                            double totalWall = std::chrono::duration<double>(clock::now() - tAll0).count();
                            fprintf(stderr, "[certauto] base=%d total autonomous-search wall=%.3fs\n", B, totalWall);
                            subsetdisc::SB_printGroupDPSummary("certauto", B);
                            exit(4); // CANDIDATE_ONLY: direct-verified but not proven maximal
                        }
                        bool exact = expectedDecimalOrNull && ar.maxDecimal == expectedDecimalOrNull;
                        if (expectedDecimalOrNull) {
                            fprintf(stderr, "[certauto] base=%d CERTIFICATION (matches known target): %s\n",
                                    B, exact ? "PASS" : "FAIL");
                        } else {
                            fprintf(stderr, "[certauto] base=%d CERTIFICATION (direct-verified, no external target "
                                            "to compare): PASS\n", B);
                        }
                        double totalWall = std::chrono::duration<double>(clock::now() - tAll0).count();
                        fprintf(stderr, "[certauto] base=%d total autonomous-search wall=%.3fs\n", B, totalWall);
                        subsetdisc::SB_printGroupDPSummary("certauto", B);
                        return;
                    }
                }
            }
        }
    }
    fprintf(stderr, "[certauto] base=%d: FAILED to find a working (subset, prefix) hypothesis within bounded search "
                    "[subsetsChecked=%lld subsetsScanned=%lld]\n", B, subsetsChecked, subsetsScanned);
    subsetdisc::SB_printGroupDPSummary("certauto", B);
}

// =====================================================================
// HIGHER-BASE-CERTIFICATION-STRATEGY.md Sec3+8: outer lexicographic
// branch-and-bound certification mode.
//
// Sec8.1: `certset` -- explicit digit set D = {1..B-1}\drops, single
// subset, never enumerates/descends to any other subset.
// Sec8.2: exact terminal entry taking an EXPLICIT (possibly
// non-descending) prefix + remaining pool (obligation I3): the prefix
// contribution C is computed by runWrongTurnSearch/buildSuffixBranchGen's
// EXISTING position arithmetic (pos = certPos() + prefix.size(), stepping
// down by 1 per prefix digit in the order given) -- never re-derived here,
// so the historic bug class (wrong position/modulus arithmetic) cannot
// recur: this file simply calls the same trusted function with a
// caller-fixed prefix instead of buildFeasiblePrefix's invented one.
// Sec8.3: `certbb` -- incumbent-guided outer DFS with dispositions
// PROVED_BELOW_INCUMBENT / EXACT_TERMINAL(REFUTED|FOUND) /
// RESOURCE_DECLINED (obligation I4).
// Sec8.4-lite: append-only certbb_<base>_manifest.jsonl.
// =====================================================================

// Obligation I1: ALL outer-search value comparisons are fixed-length
// digit-sequence lexicographic comparisons on arrays of ints (base-B
// digits, MSD-first), NEVER integer arithmetic -- full values run to
// ~64^63 ~= 2^378, far beyond any fixed-width integer. Every array
// compared here has the SAME fixed length |D| (a full arrangement is
// always a permutation of D), so lexicographic array order == numeric
// order exactly, with no bignum step required at all for comparisons
// (bignum decimal reconstruction is used ONLY for human-readable output,
// via decimalFromDigitsMSBfirstBB below, never for a decision).
static int lexCompareDigitsBB(const std::vector<int> &a, const std::vector<int> &b) {
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1; // shouldn't happen (fixed |D|); stay honest
    return 0;
}

static std::vector<int> sortedDescBB(const std::vector<int> &v) {
    std::vector<int> r = v;
    std::sort(r.begin(), r.end(), std::greater<int>());
    return r;
}

// upper = prefix ++ sort_descending(remaining) (doc Sec3.2's `upper`):
// the lexicographically largest digit array reachable from this node,
// used ONLY for pruning (I1: array compare, never integer).
static std::vector<int> upperBoundArrayBB(const std::vector<int> &prefix, const std::vector<int> &remaining) {
    std::vector<int> u = prefix;
    std::vector<int> rd = sortedDescBB(remaining);
    u.insert(u.end(), rd.begin(), rd.end());
    return u;
}

// Human-readable decimal reconstruction of a full digit array (MSD-first,
// arbitrary-precision via uint32 limbs, no overflow) -- used ONLY for
// display/manifest/final-report output, never for any certbb decision.
static std::string decimalFromDigitsMSBfirstBB(int B, const std::vector<int> &digitsMSBfirst) {
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

// Comma-separated digit list parser, shared by certset/certbb's <drops> arg.
static std::vector<int> parseDropListBB(const char *s) {
    std::vector<int> out;
    if (!s) return out;
    std::string str(s);
    size_t pos = 0;
    while (pos < str.size()) {
        size_t comma = str.find(',', pos);
        std::string tok = str.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
        if (!tok.empty()) out.push_back(atoi(tok.c_str()));
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return out;
}

// One-time setup shared by certset/certbb: computes P_full/CERTPOS range
// exactly like certStartupGuard(), then latches CERTPOS's own env var (and
// thus certPos()'s own [20,max] validation -- doc's "validation reuses
// certPos()-style range checks") from CERTSET_W (default 22), so
// certPos() below IS W_terminal for the rest of this run. MUST be called
// before the first certPos() call of the process.
static int certSetupWBB(int B) {
    u128 Lfull = checkedLcmUpToOrDie(B - 1);
    int P_full = 0; { u128 v = 1; while (v < Lfull) { v *= (u128)B; P_full++; } }
    configureCertPosForBase(B, P_full);
    const char *w = getenv("CERTSET_W");
    std::string wStr = (w && *w) ? std::string(w) : std::string("22");
    setenv("CERTPOS", wStr.c_str(), 1); // certPos() below validates this against [20, g_certPosMaxOverride]
    certStartupGuard(B); // sizeof asserts + (idempotent) configureCertPosForBase; does not itself call certPos()
    int W = certPos(); // first real call anywhere in the process for this run -- latches now
    fprintf(stderr, "[certset/bb] base=%d W_terminal=%d (CERTSET_W env, default 22; validated range [20,%d])\n",
            B, W, g_certPosMaxOverride);
    return W;
}

// Sec8.2: exact terminal disposition. FOUND/REFUTED are exhaustive over
// the ENTIRE remaining pool at this node (every pool digit was tried as
// candidate, descending, by the existing wrong-turn engine -- see
// runWrongTurnSearch's own header comment). DECLINED covers both the
// NX+NY<2 window-starvation case (checked here, BEFORE calling
// runWrongTurnSearch, so its own exit(3) path is never reached from
// certbb) and a deadline reached mid-search (ar.timedOut).
struct TerminalOutcomeBB {
    enum Disposition { REFUTED, FOUND, DECLINED } disposition = DECLINED;
    AttemptResult ar;
    const char *declineReason = "";
};

static TerminalOutcomeBB runExactTerminalBB(const ConstantsGen &c, const std::vector<int> &prefix,
                                             const std::vector<int> &pool, long rssBudgetKB,
                                             std::chrono::steady_clock::time_point deadline) {
    TerminalOutcomeBB out;
    int W = certPos();
    if ((int)pool.size() != W + 1) {
        fprintf(stderr, "[certbb] FATAL: exact terminal called with |remaining|=%zu != W_terminal+1=%d "
                        "(obligation I2 violated)\n", pool.size(), W + 1);
        exit(1);
    }
    // Same chooseWY derivation as runCert (Sec3.2: "the required
    // architectural change is to call it with the prefix fixed by the
    // outer recursion" -- NX/NY selection itself is unchanged).
    int NX = 2;
    int NY = W - c.T - c.Pc - NX;
    if (NY < 1) { NX = 1; NY = W - c.T - c.Pc - NX; }
    if (NY < 1) { NX = 0; NY = W - c.T - c.Pc - NX; }
    if (NX + NY < 2) {
        out.disposition = TerminalOutcomeBB::DECLINED;
        out.declineReason = "NX+NY<2 (CERTPOS/W_terminal window starved for this D's T+Pc)";
        return out;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
        out.disposition = TerminalOutcomeBB::DECLINED;
        out.declineReason = "deadline already reached before terminal started";
        return out;
    }
    AttemptResult ar = runWrongTurnSearch(c, prefix, pool, NX, NY, 3, rssBudgetKB,
                                           /*useCurrentRssForAvailable=*/true, deadline);
    out.ar = ar;
    if (ar.timedOut) {
        out.disposition = TerminalOutcomeBB::DECLINED;
        out.declineReason = "deadline reached mid-search (timedOut)";
        return out;
    }
    bool certified = ar.success && ar.verifiedOk >= 1 && ar.verifiedBad == 0;
    out.disposition = certified ? TerminalOutcomeBB::FOUND : TerminalOutcomeBB::REFUTED;
    return out;
}

// Sec8.1: `certset` -- single EXPLICIT digit set, discovery-heuristic
// prefix (buildFeasiblePrefix, per Sec3.4: legitimate for discovery, never
// for a NO), one exact-terminal call. Never enumerates or descends to any
// other subset -- there is only ever one D here.
static void runCertSet(int B, const std::vector<int> &drops, long rssBudgetKB) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    int W = certSetupWBB(B);
    std::vector<int> D;
    for (int d = 1; d < B; d++) if (std::find(drops.begin(), drops.end(), d) == drops.end()) D.push_back(d);
    fprintf(stderr, "[certset] base=%d |D|=%zu dropped=%zu W_terminal=%d\n", B, D.size(), drops.size(), W);
    ConstantsGen c = deriveConstantsGen(B, D);
    if (!c.ok) {
        fprintf(stderr, "[certset] base=%d: deriveConstantsGen FAILED for this EXPLICIT digit set "
                        "(arithmetically infeasible -- not a search failure)\n", B);
        exit(2);
    }
    int targetWY = W - c.T - c.Pc;
    if (targetWY < 1) {
        fprintf(stderr, "[certset] base=%d FATAL: W_terminal=%d too small for T=%d+Pc=%d -- widen CERTSET_W\n",
                B, W, c.T, c.Pc);
        exit(3);
    }
    std::vector<int> prefix, pool;
    bool feas = buildFeasiblePrefix(c, targetWY, 3, prefix, pool);
    if (!feas) {
        fprintf(stderr, "[certset] base=%d: buildFeasiblePrefix found no feasible heuristic top prefix at "
                        "W_terminal=%d -- INCONCLUSIVE (not a refutation of D); try a larger CERTSET_W or use "
                        "certbb for the full outer proof.\n", B, W);
        exit(3);
    }
    TerminalOutcomeBB outc = runExactTerminalBB(c, prefix, pool, rssBudgetKB, clock::time_point::max());
    double wall = std::chrono::duration<double>(clock::now() - t0).count();
    if (outc.disposition == TerminalOutcomeBB::FOUND) {
        fprintf(stderr, "[certset] base=%d FOUND: winningCandidate=%d survivors=%llu (verified %llu OK / %llu "
                        "FAILED) wall=%.3fs\n", B, outc.ar.winningCandidate, (unsigned long long)outc.ar.totalSurvivors,
                (unsigned long long)outc.ar.verifiedOk, (unsigned long long)outc.ar.verifiedBad, wall);
        fprintf(stderr, "[certset] base=%d: maximum value (%zu digits): %s\n",
                B, outc.ar.maxDecimal.size(), outc.ar.maxDecimal.c_str());
        fprintf(stderr, "[certset] base=%d CERTIFIED for this explicit digit set (heuristic-prefix search "
                        "exhaustive at W_terminal=%d; use certbb for the full outer-prefix proof)\n", B, W);
    } else if (outc.disposition == TerminalOutcomeBB::REFUTED) {
        fprintf(stderr, "[certset] base=%d REFUTED at the heuristic prefix (wall=%.3fs) -- this does NOT prove D "
                        "has no completion (only this one prefix's window is exhausted); use certbb for the "
                        "full outer proof.\n", B, wall);
        exit(4);
    } else {
        fprintf(stderr, "[certset] base=%d DECLINED: %s (wall=%.3fs)\n", B, outc.declineReason, wall);
        exit(3);
    }
}

// Sec8.3+8.4-lite: incumbent-guided outer DFS state + manifest.
struct CertBBContext {
    ConstantsGen c;
    int W = 0;
    long rssBudgetKB = -1;
    std::chrono::steady_clock::time_point deadline;
    FILE *manifest = nullptr;
    std::vector<int> incumbent; // MSD-first digit array; valid iff incumbentSet
    bool incumbentSet = false;
    long long nodesVisited = 0;
    long long nFound = 0, nRefuted = 0, nPruned = 0, nDeclined = 0;
    bool anyUnfinished = false;
};

static void writeManifestLineBB(CertBBContext &ctx, const std::vector<int> &prefix, const char *disposition,
                                 unsigned long long survivors, double wall, const char *note = "") {
    if (!ctx.manifest) return;
    fprintf(ctx.manifest, "{\"prefix\":[");
    for (size_t i = 0; i < prefix.size(); i++) fprintf(ctx.manifest, "%s%d", i ? "," : "", prefix[i]);
    fprintf(ctx.manifest, "],\"disposition\":\"%s\",\"survivors\":%llu,\"wall\":%.3f,\"note\":\"%s\"}\n",
            disposition, survivors, wall, note);
    fflush(ctx.manifest);
}

// Sec3.2 recurrence, obligation I1 (array compares only), I4 (RESOURCE_DECLINED
// stays unfinished, never folded into a refutation). Returns true iff this
// entire subtree is fully accounted for (no unfinished descendant).
static bool certBBProve(CertBBContext &ctx, const std::vector<int> &prefix, const std::vector<int> &remaining) {
    ctx.nodesVisited++;
    if (std::chrono::steady_clock::now() >= ctx.deadline) {
        ctx.nDeclined++;
        ctx.anyUnfinished = true;
        writeManifestLineBB(ctx, prefix, "RESOURCE_DECLINED", 0, 0, "global deadline reached before this node");
        return false;
    }

    std::vector<int> upper = upperBoundArrayBB(prefix, remaining);
    if (ctx.incumbentSet && lexCompareDigitsBB(upper, ctx.incumbent) <= 0) {
        ctx.nPruned++;
        return true; // PROVED_BELOW_INCUMBENT
    }

    if ((int)remaining.size() == ctx.W + 1) {
        auto t0 = std::chrono::steady_clock::now();
        TerminalOutcomeBB outc = runExactTerminalBB(ctx.c, prefix, remaining, ctx.rssBudgetKB, ctx.deadline);
        double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        if (outc.disposition == TerminalOutcomeBB::DECLINED) {
            ctx.nDeclined++;
            ctx.anyUnfinished = true;
            writeManifestLineBB(ctx, prefix, "RESOURCE_DECLINED", (unsigned long long)outc.ar.totalSurvivors,
                                 wall, outc.declineReason);
            return false;
        }
        if (outc.disposition == TerminalOutcomeBB::FOUND) {
            ctx.nFound++;
            writeManifestLineBB(ctx, prefix, "EXACT_TERMINAL_FOUND", (unsigned long long)outc.ar.totalSurvivors, wall);
            // Obligation I3 of doc Sec4 (direct verification already exists):
            // incumbent updates take the terminal's max survivor, compared by
            // fixed-length array lex order only (I1).
            if (!ctx.incumbentSet || lexCompareDigitsBB(outc.ar.maxDigitsMSBfirst, ctx.incumbent) > 0) {
                ctx.incumbent = outc.ar.maxDigitsMSBfirst;
                ctx.incumbentSet = true;
                fprintf(stderr, "[certbb] NEW INCUMBENT (prefixLen=%zu wall=%.3fs): %s\n", prefix.size(), wall,
                        decimalFromDigitsMSBfirstBB(ctx.c.B, ctx.incumbent).c_str());
            }
        } else {
            ctx.nRefuted++;
            writeManifestLineBB(ctx, prefix, "EXACT_TERMINAL_REFUTED", 0, wall);
        }
        return true;
    }

    // Internal node: descend, digit choices tried strictly descending
    // (doc Sec3.2's `for digit in remaining, descending`). Bound is
    // strictly lexicographically monotonic in the branch digit (same
    // prefix, differing only at this position, arrays of equal fixed
    // length) so the first child whose bound fails to beat the incumbent
    // proves every smaller digit's child also fails -- safe to break
    // rather than continue.
    std::vector<int> sorted = sortedDescBB(remaining);
    bool allFinished = true;
    for (int d : sorted) {
        std::vector<int> childPrefix = prefix; childPrefix.push_back(d);
        std::vector<int> childRemaining;
        childRemaining.reserve(remaining.size() - 1);
        for (int x : remaining) if (x != d) childRemaining.push_back(x);
        std::vector<int> childUpper = upperBoundArrayBB(childPrefix, childRemaining);
        if (ctx.incumbentSet && lexCompareDigitsBB(childUpper, ctx.incumbent) <= 0) {
            ctx.nPruned++;
            break; // monotonic: every remaining (smaller) digit choice also fails
        }
        bool finished = certBBProve(ctx, childPrefix, childRemaining);
        allFinished = allFinished && finished;
    }
    return allFinished;
}

static void runCertBB(int B, const std::vector<int> &drops, long rssBudgetKB, double capSecondsGlobal) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    int W = certSetupWBB(B);
    std::vector<int> D;
    for (int d = 1; d < B; d++) if (std::find(drops.begin(), drops.end(), d) == drops.end()) D.push_back(d);
    fprintf(stderr, "[certbb] base=%d |D|=%zu dropped=%zu W_terminal=%d capSeconds=%.0f\n",
            B, D.size(), drops.size(), W, capSecondsGlobal);

    ConstantsGen c = deriveConstantsGen(B, D);
    if (!c.ok) {
        fprintf(stderr, "[certbb] base=%d: deriveConstantsGen FAILED for this EXPLICIT digit set "
                        "(arithmetically infeasible -- not a search failure)\n", B);
        exit(2);
    }
    if (W + 1 > (int)D.size()) {
        fprintf(stderr, "[certbb] base=%d FATAL: W_terminal+1=%d exceeds |D|=%zu\n", B, W + 1, D.size());
        exit(1);
    }

    char manifestPath[128];
    snprintf(manifestPath, sizeof(manifestPath), "certbb_%d_manifest.jsonl", B);
    FILE *manifest = fopen(manifestPath, "w"); // fresh manifest each run (Sec8.4-lite: rerun-from-scratch OK)
    if (!manifest) fprintf(stderr, "[certbb] WARNING: could not open %s for writing (errno=%d)\n", manifestPath, errno);
    else fprintf(stderr, "[certbb] base=%d manifest: %s\n", B, manifestPath);

    CertBBContext ctx;
    ctx.c = c; ctx.W = W; ctx.rssBudgetKB = rssBudgetKB;
    ctx.deadline = t0 + std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(capSecondsGlobal));
    ctx.manifest = manifest;

    // Discovery seed (doc Sec3.4): buildFeasiblePrefix + one terminal call
    // at the FULL digit set's own heuristic top prefix, purely to seed the
    // incumbent for faster pruning. A DECLINED/REFUTED seed is NOT treated
    // as any kind of refutation of D -- the DFS below covers the entire
    // tree regardless of whether this seed found anything.
    {
        int targetWY = W - c.T - c.Pc;
        if (targetWY >= 1) {
            std::vector<int> seedPrefix, seedPool;
            if (buildFeasiblePrefix(c, targetWY, 3, seedPrefix, seedPool)) {
                TerminalOutcomeBB seedOut = runExactTerminalBB(c, seedPrefix, seedPool, rssBudgetKB, ctx.deadline);
                if (seedOut.disposition == TerminalOutcomeBB::FOUND) {
                    ctx.incumbent = seedOut.ar.maxDigitsMSBfirst;
                    ctx.incumbentSet = true;
                    fprintf(stderr, "[certbb] base=%d discovery seed incumbent: %s\n", B,
                            decimalFromDigitsMSBfirstBB(B, ctx.incumbent).c_str());
                    writeManifestLineBB(ctx, seedPrefix, "DISCOVERY_SEED_FOUND",
                                         (unsigned long long)seedOut.ar.totalSurvivors, 0, "not part of the DFS proof itself");
                } else {
                    fprintf(stderr, "[certbb] base=%d discovery seed: no immediate completion at the heuristic "
                                    "prefix (disposition=%s) -- DFS proceeds unseeded (this is NOT a refutation)\n",
                            B, seedOut.disposition == TerminalOutcomeBB::REFUTED ? "REFUTED" : "DECLINED");
                }
            }
        }
    }

    bool finished = certBBProve(ctx, {}, D);

    double wall = std::chrono::duration<double>(clock::now() - t0).count();
    fprintf(stderr, "[certbb] base=%d DFS DONE: nodesVisited=%lld found=%lld refuted=%lld pruned=%lld "
                    "unfinished(declined)=%lld wall=%.3fs\n",
            B, ctx.nodesVisited, ctx.nFound, ctx.nRefuted, ctx.nPruned, ctx.nDeclined, wall);
    if (ctx.incumbentSet) {
        std::string dec = decimalFromDigitsMSBfirstBB(B, ctx.incumbent);
        fprintf(stderr, "[certbb] base=%d incumbent (%zu decimal digits): %s\n", B, dec.size(), dec.c_str());
    } else {
        fprintf(stderr, "[certbb] base=%d: NO SURVIVOR FOUND ANYWHERE in this digit set\n", B);
    }
    if (manifest) fclose(manifest);

    bool allAccounted = finished && !ctx.anyUnfinished;
    if (allAccounted) {
        fprintf(stderr, "[certbb] base=%d CERTIFIED (zero unfinished branches)%s\n", B,
                ctx.incumbentSet ? "" : " -- and NO valid arrangement exists for this digit set");
    } else {
        fprintf(stderr, "[certbb] base=%d INCOMPLETE: %lld branches unfinished -- best incumbent is a LOWER "
                        "BOUND\n", B, ctx.nDeclined);
        exit(4);
    }
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
    } else if (mode == "certfm") {
        // certfm <base> [expectedDecimal] [rssBudgetKB]
        // BASE-50-FULL-MODULUS-BUCKET.md Sec 9 Phase A: full-modulus shallow
        // bucket join (modulus L, no nilpotent suffix peeling), same CLI
        // argument conventions as `cert`.
        if (argc < 3) { fprintf(stderr, "certfm requires <base>\n"); return 1; }
        int base = atoi(argv[2]);
        const char *expected = (argc >= 4 && std::string(argv[3]) != "-") ? argv[3] : nullptr;
        long rssBudgetKB = -1;
        if (argc >= 5) rssBudgetKB = atol(argv[4]);
        fprintf(stderr, "[certfm] base=%d rssBudgetKB=%ld expected=%s\n", base, rssBudgetKB, expected ? expected : "(none)");
        certdrv::runCertFM(base, expected, rssBudgetKB, 1800.0, 5400.0);
    } else if (mode == "certauto") {
        // certauto <base> [expectedDecimal] [rssBudgetKB]
        // BASE-50-FULL-MODULUS-BUCKET.md Sec 9 Phase B: planner-driven driver
        // -- chooses peeled vs full-modulus and (NX,NY,K) per surviving
        // subset via a cost model, instead of cert/certfm's hardcoded split.
        if (argc < 3) { fprintf(stderr, "certauto requires <base>\n"); return 1; }
        int base = atoi(argv[2]);
        const char *expected = (argc >= 4 && std::string(argv[3]) != "-") ? argv[3] : nullptr;
        long rssBudgetKB = -1;
        if (argc >= 5) rssBudgetKB = atol(argv[4]);
        fprintf(stderr, "[certauto] base=%d rssBudgetKB=%ld expected=%s\n", base, rssBudgetKB, expected ? expected : "(none)");
        certdrv::runCertAuto(base, expected, rssBudgetKB, 1800.0, 5400.0);
    } else if (mode == "certset") {
        // HIGHER-BASE-CERTIFICATION-STRATEGY.md Sec8.1:
        // certset <base> <comma-separated-dropped-digits> [rssBudgetKB]
        // Runs on the EXPLICIT digit set D={1..B-1}\drops only -- never
        // enumerates other subsets, never descends to a smaller set on
        // failure. CERTSET_W env (default 22) sets W_terminal.
        if (argc < 4) { fprintf(stderr, "certset requires <base> <comma-separated-dropped-digits>\n"); return 1; }
        int base = atoi(argv[2]);
        std::vector<int> drops = certdrv::parseDropListBB(argv[3]);
        long rssBudgetKB = -1;
        if (argc >= 5) rssBudgetKB = atol(argv[4]);
        fprintf(stderr, "[certset] base=%d dropsArg=%s rssBudgetKB=%ld\n", base, argv[3], rssBudgetKB);
        certdrv::runCertSet(base, drops, rssBudgetKB);
    } else if (mode == "certbb") {
        // HIGHER-BASE-CERTIFICATION-STRATEGY.md Sec8.3:
        // certbb <base> <comma-separated-dropped-digits> [rssBudgetKB] [capSeconds]
        // Incumbent-guided outer lexicographic branch-and-bound over the
        // EXPLICIT digit set D={1..B-1}\drops. CERTSET_W env (default 22)
        // sets W_terminal. capSeconds (default 5400) is the GLOBAL wall
        // budget for the whole DFS (per-terminal deadline == same clock).
        if (argc < 4) { fprintf(stderr, "certbb requires <base> <comma-separated-dropped-digits>\n"); return 1; }
        int base = atoi(argv[2]);
        std::vector<int> drops = certdrv::parseDropListBB(argv[3]);
        long rssBudgetKB = -1;
        if (argc >= 5) rssBudgetKB = atol(argv[4]);
        double capSeconds = 5400.0;
        if (argc >= 6) capSeconds = atof(argv[5]);
        fprintf(stderr, "[certbb] base=%d dropsArg=%s rssBudgetKB=%ld capSeconds=%.0f\n",
                base, argv[3], rssBudgetKB, capSeconds);
        certdrv::runCertBB(base, drops, rssBudgetKB, capSeconds);
    } else {
        fprintf(stderr, "unknown mode %s\n", mode.c_str());
        return 1;
    }
    return 0;
}
