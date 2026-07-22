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
};

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
struct TrieNode {
    // children: (edge digit 0..48, child node index)
    std::vector<std::pair<uint8_t,int32_t>> children;
    // present only at depth==DEPTH nodes: ordered y-digit-lists (positions 12..12+NY-1,
    // i.e. index0 = relative position 12) that hash to this trie leaf.
    std::vector<std::vector<int>> terminalY;
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
};

static inline uint64_t maskOf(const std::vector<int> &digits) {
    uint64_t m = 0;
    for (int d : digits) m |= ((uint64_t)1 << d);
    return m;
}

static inline uint64_t bit(int d) { return (uint64_t)1 << d; }

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
};

// Walk the trie for one (x, lift j) pair. allowedMinusX = A19mask & ~xmask.
static void walkOne(const Trie &trie, u128 W, uint64_t allowedMinusX, uint64_t xmask,
                     uint64_t A19mask, WalkStats &stats,
                     std::vector<std::array<int,19>> *survivorLog,
                     const std::vector<int> &xdigits) {
    auto Wdig = digitsLSDof(W);
    std::vector<StackFrame> stack;
    stack.push_back({0, 0, 0, 0});
    while (!stack.empty()) {
        StackFrame f = stack.back();
        stack.pop_back();
        stats.visits++;
        const TrieNode &node = trie.nodes[f.node];
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
                stack.push_back({child, f.depth + 1, nb, f.used | wdbit});
            } else {
                if (wd != 0) continue;
                stack.push_back({child, f.depth + 1, nb, f.used});
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

        WalkStats stats;
        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            walkOne(trie, W, allowedMinusX, xmask, A19mask, stats, nullptr, xdigits);
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

        for (int j = 0; j < c.lifts; j++) {
            u128 W = c_x + (u128)j * Lc;
            auto Wdig = digitsLSDof(W);
            std::vector<DecodeFrame> stack;
            stack.push_back({0,0,0,0,{}});
            while (!stack.empty()) {
                DecodeFrame f = stack.back(); stack.pop_back();
                totalVisits++;
                const TrieNode &node = trie.nodes[f.node];
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
        fprintf(stderr, "usage: %s smoke|gate|full <cand> <NX> <NY>\n", argv[0]);
        return 1;
    }
    std::string mode = argv[1];
    if (mode == "smoke") {
        runSmoke(c);
    } else if (mode == "gate") {
        runGate(c);
    } else if (mode == "full") {
        if (argc < 5) { fprintf(stderr, "full requires <cand> <NX> <NY>\n"); return 1; }
        int cand = atoi(argv[2]);
        int NX = atoi(argv[3]);
        int NY = atoi(argv[4]);
        runFull(c, cand, NX, NY);
    } else {
        fprintf(stderr, "unknown mode %s\n", mode.c_str());
        return 1;
    }
    return 0;
}
