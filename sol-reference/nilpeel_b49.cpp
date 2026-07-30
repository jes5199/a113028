#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

using u64 = std::uint64_t;
using u128 = unsigned __int128;
using s128 = __int128;

static constexpr int B = 49;
static constexpr int LEAF = 12;

static u128 parse128(const char *s) {
    u128 x = 0;
    while (*s) x = 10 * x + unsigned(*s++ - '0');
    return x;
}

static u128 egcd_inv(u128 a, u128 mod) {
    s128 old_r = s128(a), r = s128(mod);
    s128 old_s = 1, s = 0;
    while (r) {
        const s128 q = old_r / r;
        const s128 nr = old_r - q * r; old_r = r; r = nr;
        const s128 ns = old_s - q * s; old_s = s; s = ns;
    }
    old_s %= s128(mod);
    if (old_s < 0) old_s += s128(mod);
    return u128(old_s);
}

struct Search {
    u128 mod, target, pow[20], bound;
    u64 avail;
    int out[20]{};
    u64 nodes = 0, leaves = 0, candidates = 0;

    bool leaf(u128 rho) {
        ++leaves;
        const u128 r = rho % mod;
        const u128 v0 = (target + mod - r) % mod;
        if (v0 >= bound) return false;
        u128 j = (bound - 1 - v0) / mod;
        for (;;) {
            ++candidates;
            u128 v = v0 + j * mod;
            u64 mask = 0;
            int digs[LEAF];
            bool ok = true;
            for (int i = 0; i < LEAF; ++i) {
                const int d = int(v % B); v /= B;
                if (!d || (mask >> d & 1)) { ok = false; break; }
                digs[i] = d; mask |= u64{1} << d;
            }
            if (ok && v == 0 && mask == avail) {
                for (int i = 0; i < LEAF; ++i) out[i] = digs[i];
                return true;
            }
            if (!j) break;
            --j;
        }
        return false;
    }

    bool dfs(int m, u128 rho) {
        ++nodes;
        if (m == LEAF) return leaf(rho);
        for (int d = 21; d >= 1; --d) {
            const u64 bit = u64{1} << d;
            if (!(avail & bit)) continue;
            avail ^= bit;
            out[m - 1] = d;
            if (dfs(m - 1, rho + u128(d) * pow[m - 1])) return true;
            avail ^= bit;
        }
        return false;
    }
};

int main(int argc, char **argv) {
    const int candidate = argc > 1 ? std::atoi(argv[1]) : 21;
    const int forced_lsd = argc > 2 ? std::atoi(argv[2]) : 0;
    const u128 L = parse128("442720643463713815200");
    const u128 mod = L / 7;
    const u128 invB = egcd_inv(B, mod);

    // Prefix 48..25,23,22,candidate occupies original positions 46..20.
    std::vector<int> prefix;
    for (int d = 48; d >= 25; --d) prefix.push_back(d);
    prefix.push_back(23); prefix.push_back(22); prefix.push_back(candidate);
    u128 prefix_mod = 0, bp = 1;
    for (int pos = 0; pos <= 46; ++pos) {
        if (pos >= 20) {
            const int d = prefix[46 - pos];
            prefix_mod = (prefix_mod + u128(d) * bp) % mod;
        }
        bp = bp * B % mod;
    }
    const u128 residual = (mod - prefix_mod) % mod;

    std::vector<int> remaining;
    for (int d = 1; d <= 21; ++d) if (d != candidate) remaining.push_back(d);
    std::vector<int> lsds;
    for (int d : remaining)
        if (d % 7 == 0 && (!forced_lsd || d == forced_lsd)) lsds.push_back(d);
    for (int d0 : lsds) {
        Search s;
        s.mod = mod;
        s.target = ((residual + mod - d0) % mod) * invB % mod;
        s.pow[0] = 1;
        for (int i = 1; i < 20; ++i) s.pow[i] = s.pow[i - 1] * B % mod;
        s.bound = 1;
        for (int i = 0; i < LEAF; ++i) s.bound *= B;
        s.avail = 0;
        for (int d : remaining) if (d != d0) s.avail |= u64{1} << d;

        const auto started = std::chrono::steady_clock::now();
        const bool found = s.dfs(19, 0);
        const double seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
        std::cout << "candidate=" << candidate << " d0=" << d0
                  << " found=" << found << " nodes=" << s.nodes
                  << " leaves=" << s.leaves << " candidates=" << s.candidates
                  << " seconds=" << seconds << '\n';
        if (found) {
            std::cout << "  high suffix=";
            for (int i = 18; i >= 0; --i) std::cout << s.out[i] << (i ? ',' : '\n');
        }
    }
}
