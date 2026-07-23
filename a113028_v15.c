//  A113028 v5: Engine C — deterministic prefix DFS + CRT suffix-feasibility
//  pruning + residue-leaf decoding (see ALGORITHMS.md section A).
//  MSB-first DFS over positions k-1 .. P only (P = min{p : B^p >= LCM}).
//  At depth P, the remaining P-position suffix value is forced mod LCM; it is
//  decoded directly (no brute force) by testing the O(B^P/LCM) candidates
//  descending and checking digit-multiset equality by bitmask. Every DFS node
//  is additionally pruned by sound order-e (e=2..6) CRT partition-feasibility
//  checks generalizing e2_check/order_e_check to nonzero prefix targets.
//  Fully deterministic — no budget-doubling/engine-alternation machinery.
// Order-1 moduli are filtered at subset level and cost nothing at runtime.
// Subpower fix: for each prime power q=p^a, filter on the largest subpower with
// multiplicative order 1 (digit-sum) and order <=2 (split feasibility) — orders
// can drop on subpowers, and missing this makes dead subsets undetectable.
//
// build:  gcc -O2 -march=native -o a113028_v5 a113028_v5.c
// run:    ./a113028_v5 [lo] [hi] [verbose]     e.g.  ./a113028_v5 2 48 1

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned __int128 u128;
typedef uint64_t u64;

#define MAXB 64
#define MAXK (MAXB)
#define MAXM 20     // max tracked moduli
#define MAXOE 128   // max order-e (e=3..6) node-prune entries

static int B;

static u64 gcd64(u64 a, u64 b){ while(b){ u64 t=a%b; a=b; b=t; } return a; }

static void digits_to_decimal(const int *nd, int k, int base, char *out){
    static int dec[512]; int dl=1; dec[0]=0;
    for(int i=k-1;i>=0;i--){
        int carry = nd[i];
        for(int j=0;j<dl;j++){ int v=dec[j]*base+carry; dec[j]=v%10; carry=v/10; }
        while(carry){ dec[dl++]=carry%10; carry/=10; }
    }
    int j=0; for(int i=dl-1;i>=0;i--) out[j++]='0'+dec[i]; out[j]=0;
}

// ---------- subset state ----------
static int S[MAXK]; static int k;
static u128 LCM;
// v13: generalized modulus/target (see below near POW_) declared here so
// early users like e2_check can see them too.
static u128 MOD_ = 1;
static u128 TARGET_ = 0;
static u64 setmask;
static u64 D1;

typedef struct { u64 q; int is_nil; int t; int e; u64 q1, q2; u64 qe[7]; } PP;
// qe[3..6]: largest subpower p^b<=q with B^e==1 (mod p^b), for e=3..6.
// qe[0],qe[1],qe[2] unused (q1,q2 fields already cover e=1,2).
static PP pps[32]; static int npps;

static u64 TQ[MAXM]; static int TE[MAXM]; static int ntq;
static int e2idx[MAXM]; static int ne2;
static u64 E2Q[MAXM];   // split modulus per e2 prune entry

static long long g_iters, g_nodes;

static int vp(int n, int p){ int v=0; while(n%p==0){ v++; n/=p; } return v; }

static int build_pps(void){
    npps=0; D1=1; LCM=1;
    for(int p=2;p<B;p++){
        int isp=1; for(int d=2;d*d<=p;d++) if(p%d==0){isp=0;break;}
        if(!isp) continue;
        int a=0;
        for(int i=0;i<k;i++){ int v=vp(S[i],p); if(v>a)a=v; }
        if(a==0) continue;
        u64 q=1; for(int i=0;i<a;i++) q*=(u64)p;
        u128 nl = LCM*q;
        if(nl/q != LCM){ fprintf(stderr,"LCM overflow base %d\n",B); exit(2); }
        LCM=nl;
        PP*pp=&pps[npps++]; pp->q=q;
        int vB=vp(B,p);
        if(vB>0){ pp->is_nil=1; pp->t=(a+vB-1)/vB; if(pp->t==1) D1*=q; }
        else{
            pp->is_nil=0;
            u64 r=B%q; int e=1;
            while(r!=1 && e<=2){ r=(r*((u64)B%q))%q; e++; }
            pp->e=(r==1)?e:3;
            // largest subpowers p^b <= q with B==1 (q1) and B^2==1 (q2) mod p^b
            u64 q1=1,q2=1,pb=1;
            u64 qe3=1,qe4=1,qe5=1,qe6=1;
            for(int b=1;b<=a;b++){
                pb*=(u64)p;
                if(((u64)B-1)%pb==0) q1=pb;
                u64 Bm=(u64)B%pb;
                u64 p2=(Bm*Bm)%pb;              // B^2
                if(p2==1%pb) q2=pb;
                u64 p3=(p2*Bm)%pb;              // B^3
                if(p3==1%pb) qe3=pb;
                u64 p4=(p2*p2)%pb;              // B^4
                if(p4==1%pb) qe4=pb;
                u64 p5=(p4*Bm)%pb;              // B^5
                if(p5==1%pb) qe5=pb;
                u64 p6=(p3*p3)%pb;              // B^6
                if(p6==1%pb) qe6=pb;
            }
            pp->q1=q1; pp->q2=q2;
            pp->qe[3]=qe3; pp->qe[4]=qe4; pp->qe[5]=qe5; pp->qe[6]=qe6;
        }
    }
    if((u64)(LCM%(unsigned)B)==0) return 0; // ten rule
    return 1;
}

static void build_tracked(void){
    ntq=0; ne2=0;
    for(int j=0;j<npps;j++){
        if(pps[j].is_nil){ TQ[ntq]=pps[j].q; TE[ntq]=0; ntq++; }
        else if(pps[j].q>pps[j].q1){
            if(pps[j].q2>pps[j].q1){ e2idx[ne2]=ntq; E2Q[ne2]=pps[j].q2; ne2++; }
            TQ[ntq]=pps[j].q; TE[ntq]=3; ntq++;
        }
    }
}

// order-2 feasibility for available-digit mask, m remaining positions,
// current prefix residue r, modulus q (must satisfy B^2 == 1 mod q). q<64.
static int e2_check(u64 q, u64 avail, int m, u64 r){
    int co=m/2;
    u64 Bq=(u64)B%q;
    u64 bm = (m&1)? Bq : 1;
    // v13: target generalized from 0 to TARGET_ (mod q) — reduces to the
    // original formula exactly when TARGET_==0 (ordinary engineC() mode).
    u64 target = ((u64)(TARGET_%q) + q - (r*bm)%q)%q;
    u64 R=0;
    for(int d=1; d<B; d++) if(avail>>d&1) R=(R+d)%q;
    u64 Bm1=((u64)B-1)%q;
    u64 need=(target + q - R)%q;
    u64 dp[MAXK/2+2]; memset(dp,0,sizeof(u64)*(co+2));
    dp[0]=1;
    u64 full=(q>=64)?~0ULL:((1ULL<<q)-1);
    int seen=0;
    for(int d=B-1; d>=1; d--){
        if(!(avail>>d&1)) continue;
        seen++;
        int dd=d%(int)q;
        int hi=(seen<co)?seen:co;
        for(int c=hi;c>=1;c--){
            u64 mm=dp[c-1];
            dp[c]|= dd? (((mm<<dd)|(mm>>(q-dd)))&full) : mm;
        }
    }
    u64 mask=dp[co];
    while(mask){
        int so=__builtin_ctzll(mask); mask&=mask-1;
        if((Bm1*(u64)so)%q==need) return 1;
    }
    return 0;
}

// order-e subset-level infeasibility DP (generalizes e2_check to e=3..6).
// Positions i=0..k-1 (weight B^i mod q) fall into e classes by i mod e, with
// fixed class counts c_j (j=0..e-1). Existence check: can the k digits in
// `setmask` be partitioned into groups of exactly c_0..c_{e-1} elements so
// that sum_j w_j*(group sum) == 0 (mod q)?  Pure filter: returns 1 (feasible)
// whenever unsure or when the state space is too large to explore.
#define OE_MAXSTATES 2000000
static uint8_t oe_cur[OE_MAXSTATES], oe_nxt[OE_MAXSTATES];
// order_e_check_m: generalized order-e (e classes by i mod e over the m
// positions 0..m-1) CRT partition-feasibility DP. Existence check: can the
// digits present in `avail` (must number exactly m) be assigned to the m
// positions (m/e or so per class j, weight B^j mod q) so that the weighted
// sum == target (mod q)?  Pure sound filter: returns 1 (feasible) whenever
// unsure or when the state space is too large to explore.
static int order_e_check_m(u64 q, int e, u64 avail, int m, u64 target){
    if(q<=1) return 1;
    int c[8]; for(int j=0;j<e;j++) c[j]=0;
    for(int i=0;i<m;i++) c[i%e]++;
    u64 w[8]; { u64 wv=1%q; u64 Bq=(u64)B%q; for(int j=0;j<e;j++){ w[j]=wv; wv=(wv*Bq)%q; } }
    u64 dims[8]; for(int j=0;j<e-1;j++) dims[j]=(u64)(c[j]+1);
    u64 stride[8]; u64 M=1;
    for(int j=0;j<e-1;j++){ stride[j]=M; M*=dims[j]; }
    u64 total = M*q;
    if(total==0 || total>OE_MAXSTATES) return 1; // too big: skip, don't filter
    memset(oe_cur,0,(size_t)total);
    oe_cur[0]=1; // n_j=0 all, s=0
    uint8_t *cur=oe_cur,*nxt=oe_nxt;
    int processed=0;
    for(int d=1; d<B; d++){
        if(!(avail>>d & 1ULL)) continue;
        memset(nxt,0,(size_t)total);
        u64 dm=(u64)d%q;
        for(u64 idx=0; idx<total; idx++){
            if(!cur[idx]) continue;
            u64 s=idx/M, rem=idx%M;
            int nv[8], sumtracked=0;
            for(int j=0;j<e-1;j++){ nv[j]=(int)((rem/stride[j])%dims[j]); sumtracked+=nv[j]; }
            for(int j=0;j<e-1;j++){
                if(nv[j] < c[j]){
                    u64 nrem = rem + stride[j];
                    u64 ns = (s + w[j]*dm) % q;
                    nxt[ns*M+nrem]=1;
                }
            }
            int implied = processed - sumtracked;
            if(implied < c[e-1]){
                u64 ns=(s + w[e-1]*dm) % q;
                nxt[ns*M+rem]=1;
            }
        }
        uint8_t *t=cur; cur=nxt; nxt=t;
        processed++;
    }
    // full assignment: all tracked n_j == c_j  <=>  rem == M-1; need s==target
    u64 tt = target % q;
    return cur[tt*M + (M-1)] ? 1 : 0;
}
// subset-level wrapper (target=0, full k positions, full setmask) — kept as a
// distinct entry point so subset_filters (unchanged) keeps working verbatim.
static int order_e_check(u64 q, int e){
    return order_e_check_m(q, e, setmask, k, 0);
}

static int subset_filters(void){
    u64 T=0; for(int i=0;i<k;i++) T+=S[i];
    for(int j=0;j<npps;j++){
        if(pps[j].is_nil) continue;
        if(pps[j].q1>1 && (T%pps[j].q1)) return 0;
    }
    if(D1>1){
        int ok=0; for(int i=0;i<k;i++) if((u64)S[i]%D1==0){ ok=1; break; }
        if(!ok) return 0;
    }
    for(int j=0;j<npps;j++)
        if(!pps[j].is_nil && pps[j].q2>pps[j].q1)
            if(!e2_check(pps[j].q2,setmask,k,0)) return 0;
    for(int j=0;j<npps;j++){
        if(pps[j].is_nil) continue;
        u64 q1=pps[j].q1, q2=pps[j].q2;
        u64 qe3=pps[j].qe[3], qe4=pps[j].qe[4], qe5=pps[j].qe[5], qe6=pps[j].qe[6];
        // run (e,qe) only when it strictly beats every qd for proper divisors
        // d of e (d in {1,2,3,4,5,6}) — i.e. it carries genuinely new info.
        if(qe3>q1)
            if(!order_e_check(qe3,3)) return 0;
        if(qe4>q1 && qe4>q2)
            if(!order_e_check(qe4,4)) return 0;
        if(qe5>q1)
            if(!order_e_check(qe5,5)) return 0;
        if(qe6>q1 && qe6>q2 && qe6>qe3)
            if(!order_e_check(qe6,6)) return 0;
    }
    return 1;
}

// ---------- engine C: prefix DFS + leaf residue decode + CRT pruning ----------
// P = min{p : B^p >= LCM}; positions 0..P-1 are the "leaf window" whose value
// is uniquely forced (mod LCM) by the prefix already placed. Positions
// P..k-1 are explored by DFS, largest digit first, with sound per-node
// order-e (e=2..6) CRT feasibility pruning. First DFS+leaf success (in
// descending digit order) is the global maximum for the subset.
static u128 POW_[MAXK+2];   // B^i mod MOD_
static u128 BP_;            // B^P (exact, not reduced)
static int  P_;             // leaf window width
static u128 rho_;           // prefix residue: sum d*B^pos (mod MOD_) over placed digits
static u64  c_avail;
// v13: MOD_/TARGET_ (declared near LCM above) let engineC_rec/leaf_solve be
// retargeted from (LCM,0) to any (M,t) — this is exactly what the
// per-suffix peeled subsearch needs (NILPOTENT-PEELING.md production step
// 2: "Engine C instance modulo Lc" with a nonzero suffix-determined
// target). Ordinary engineC() always runs with MOD_=LCM, TARGET_=0;
// peeled_solve() below saves/restores these around each per-witness
// subsearch.
static u64  c_res[MAXM];
static int  c_out[MAXK];    // c_out[pos] = digit at place value B^pos
static int  c_cntD1;
static long long c_nodes;
static long long c_leaves, c_cands;
// incumbent-prefix pruning for peeled subsearches (branch-and-bound seed):
// when active and still tied to the incumbent prefix, a digit strictly below
// the incumbent's digit at this level closes the node (descending d loop).
static int inc_active=0, inc_state=0, inc_startm=0;
static int inc_h[MAXK];
static int  gate_;   // run feasibility DPs only when remaining m <= gate_:
                     // above it (m! >> L_eff) they essentially never prune and
                     // per-node DP cost dominates the whole search (v5 lesson).
static char g_engine;

// ---------- v14: CANDIDATE mode (ENGC_GUESS) ----------
// See task brief / header comment near candidate_attempt() below for the
// full mode spec. g_guess_mode: 0 = off (certified engineC/peeled_solve
// paths run unmodified); 1 = auto W0 (= gate_+1, i.e. m*+2 per the
// divergence-depth law, ALGORITHMS.md); N>1 = explicit initial free-tail
// width W0=N. g_final_W records the W at which the reported candidate was
// actually found (widening loop may have grown past W0), for reporting.
static int g_guess_mode=0;
static int g_final_W=0;

#define NODE_CAP 50000000000LL   // 5e10

// order-3..6 node-prune list: for each tracked non-nil modulus with a
// strictly-informative subpower at order e, remember (e, subpower q, index
// into TQ/c_res for the parent full modulus so we can reduce its residue).
static int oe36_e[MAXOE]; static u64 oe36_q[MAXOE]; static int oe36_tqidx[MAXOE];
static int noe36;
static int oe56_e[MAXOE]; static u64 oe56_q[MAXOE]; static int oe56_tqidx[MAXOE];
static int noe56;

// Single-modulus sufficiency (SINGLE-MODULUS.md): for modulus q = p^a with
// p not dividing B(B-1), if w = min(m/e, l/2) satisfies w^2 >= q-1 (l =
// longest run of consecutive values in the remaining digit set), EVERY
// target is achievable mod q — the DP check provably returns feasible, so
// skip it. elig flags are per prime; w is re-evaluated per node.
// Theorem 1' (THEOREM-1-PRIME.md) per-node discharge: for a check modulus Q
// whose component orders all divide e, with V = prod p^{min(v_p(B-1), v_p(Q))},
// the achievable image is EXACTLY the invariant V-coset whenever
// w^2 >= Q/V - 1 (w = min(m/e, run/2)) — and the invariant congruence is
// automatic within a subset, so above that depth the check provably cannot
// prune and is skipped. Subsumes the v9 single-modulus (V=1) discharge.
static u64 e2_cosC[MAXM], oe36_cosC[MAXOE], ql_cosC[2];
static u64 coset_size(u64 q){
    u64 V=1, t=q;
    while(t>1){
        u64 p=2; while(p*p<=t && t%p) p++;
        if(p*p>t) p=t;
        int a=0; while(t%p==0){ t/=p; a++; }
        int vB=0; u64 x=(u64)B-1; while(x%p==0){ x/=p; vB++; }
        int v = vB<a ? vB : a;
        for(int i=0;i<v;i++) V*=p;
    }
    return q/V;
}

// Joint cyclotomic-layer moduli (EXTERNAL-REVIEW-2026-07-22.md #1):
// Q_e = product over tracked primes of the largest subpower with B^e == 1.
// The class-partition DP mod the WHOLE Q_e is exact for the layer (base-16
// demo: joint mod 819 = the true image; separate 9,7,13 marginals are not).
// Tracked residues r mod Q_e are maintained incrementally like c_res.
static u64 QL_[2];       // QL_[0]=Q_2, QL_[1]=Q_3 (1 if trivial)
static u64 ql_res[2];
static u64 ql_bm[2];     // Barrett magic for QL_

// ---------- v12: p-adic suffix-peeling witness-liveness prune ----------
// PEELING-SUPPORT.md §1,§4; CORRECTION-SPECTRUM.md. Lnil = product of the
// is_nil prime powers of LCM (those p|B); s = max is_nil t (t already =
// ceil(a/vB), the depth at which B^t == 0 mod that prime power). A witness
// is an ordered s-tuple of distinct digits (d0..d_{s-1}) from S[] with
// sum d_j*B^j == 0 (mod Lnil) — i.e. a legal last-s-digit suffix mod Lnil.
// If none exist the subset can never reach a multiple of Lnil (hence never
// of LCM, since Lnil | LCM): engineC fails immediately (§4 existence
// check). Otherwise, at every DFS node, if no witness's digit set is still
// fully available, no completion can ever place a valid suffix: prune.
// This is embedded as a pure liveness prune (never reorders MSB-first
// search) — it does not attempt suffix-first decomposition.
#define MAXWIT 4096
static u64 wmasks[MAXWIT];
static int nwmasks;
static int witness_active;
static void build_layers(void){
    QL_[0]=QL_[1]=1;
    for(int j=0;j<npps;j++){
        if(pps[j].is_nil) continue;
        u64 q2=pps[j].q2, q1=pps[j].q1, qe3=pps[j].qe[3];
        if(q2>q1) QL_[0]*=q2;
        if(qe3>q1) QL_[1]*=qe3;
    }
    for(int i=0;i<2;i++){ if(QL_[i]<2) QL_[i]=1; ql_bm[i]=QL_[i]>1?(u64)(~0ULL)/QL_[i]:0; ql_res[i]=0; }
    // Guard rail (PEELING-SUPPORT.md 5.6): layer moduli must be coprime to B.
    // The b39 analysis bug was exactly a violation of this in offline code.
    for(int i=0;i<2;i++) if(QL_[i]>1 && gcd64(QL_[i],(u64)B)!=1){
        fprintf(stderr,"FATAL: layer modulus %llu shares a factor with B=%d\n",
                (unsigned long long)QL_[i],B);
        exit(9);
    }
}
static u64 smallest_p(u64 q){ for(u64 p=2;p*p<=q;p++) if(q%p==0) return p; return q; }
static int mod_elig(u64 q){ u64 p=smallest_p(q); return ((u64)B%p)!=0 && ((u64)(B-1))%p!=0; }
static int longest_run(u64 x){ int r=0; while(x){ x &= x<<1; r++; } return r; }

// Built once per subset, after build_pps()/build_tracked() (both left
// untouched). Mirrors exactly the criteria build_tracked/subset_filters use
// to decide which (e,q) pairs carry genuinely new information, so it stays
// in lockstep with the unmodified TQ[] indexing.
static void build_oe36(void){
    noe36=0; noe56=0;
    int idx=0;
    for(int j=0;j<npps;j++){
        if(pps[j].is_nil){ idx++; continue; }
        if(pps[j].q>pps[j].q1){
            u64 q1=pps[j].q1, q2=pps[j].q2;
            u64 qe3=pps[j].qe[3], qe4=pps[j].qe[4], qe5=pps[j].qe[5], qe6=pps[j].qe[6];
            // Node-level: e=3 every band node (cheap). e=4..6 (~50-100us/call,
            // the v5 gprof lesson) go in a second list checked only every
            // OTHER band level — half the cost, keeps most of the pruning.
            if(qe3>q1 && noe36<MAXOE){ oe36_e[noe36]=3; oe36_q[noe36]=qe3; oe36_tqidx[noe36]=idx; noe36++; }
            if(qe4>q1 && qe4>q2 && noe56<MAXOE){ oe56_e[noe56]=4; oe56_q[noe56]=qe4; oe56_tqidx[noe56]=idx; noe56++; }
            if(qe5>q1 && noe56<MAXOE){ oe56_e[noe56]=5; oe56_q[noe56]=qe5; oe56_tqidx[noe56]=idx; noe56++; }
            if(qe6>q1 && qe6>q2 && qe6>qe3 && noe56<MAXOE){ oe56_e[noe56]=6; oe56_q[noe56]=qe6; oe56_tqidx[noe56]=idx; noe56++; }
            idx++;
        }
    }
}

// Barrett reduction for the tracked moduli: BM_[j] = floor(2^64/TQ[j]);
// x mod TQ[j] via one 64x64->128 mul + correction. x < TQ[j]*B always here.
static u64 BM_[MAXM];
static void build_barrett(void){
    for(int j=0;j<ntq;j++) BM_[j] = (u64)(~0ULL) / TQ[j];
}
static inline u64 barrett(u64 x, int j){
    u64 q = (u64)(((u128)x * BM_[j]) >> 64);
    u64 r = x - q*TQ[j];
    while(r >= TQ[j]) r -= TQ[j];
    return r;
}

static u64 modpow_u64(u64 base, u64 e, u64 mod){
    u64 r=1%mod; base%=mod;
    while(e){ if(e&1) r=(r*base)%mod; base=(base*base)%mod; e>>=1; }
    return r;
}

// Leaf decode at depth P_: the required suffix value v satisfies
// v == (LCM - rho_) mod LCM, 0 <= v < B^P_. Candidates are v0, v0+LCM, ...
// walked DESCENDING; first candidate that decomposes into P_ nonzero base-B
// digits whose bitmask exactly equals c_avail is the maximum completion.
// Widened leaf: solve the last m positions (m >= P_) by direct candidate
// enumeration. The window value must be ≡ -rho (mod LCM) and < B^m; there are
// ~B^m/LCM candidates, each checked in O(m). Widening past P_ absorbs the
// bushiest DFS levels (the win: BW_ chosen so candidate count stays <= ~256).
static u128 BPOW_[MAXK+1];  // exact B^i for i <= BW_
static int  BW_;            // leaf window width (>= P_)
static int leaf_solve(int m){
    c_leaves++;
    if(m==0) return (rho_ % MOD_)==(TARGET_ % MOD_);
    u128 v0 = (MOD_ + (TARGET_ % MOD_) - (rho_ % MOD_)) % MOD_;
    if(v0 >= BPOW_[m]) return 0;         // no candidate fits in the window
    u128 span = BPOW_[m] - 1 - v0;
    u128 j = span / MOD_;
    int digs[MAXK];
    for(;;){
        c_cands++;
        u128 v = v0 + j*MOD_;
        u128 vv=v; u64 mask=0; int ok=1;
        for(int i=0;i<m;i++){
            int dgt=(int)(vv % (unsigned)B); vv/=(unsigned)B;
            if(dgt==0){ ok=0; break; }
            digs[i]=dgt; mask|=(1ULL<<dgt);
        }
        if(ok && mask==c_avail){
            for(int i=0;i<m;i++) c_out[i]=digs[i];
            return 1;
        }
        if(j==0) break;
        j--;
    }
    return 0;
}

static int engineC_rec(int m){
    // long-run heartbeat: every 2^28 nodes, emit nodes + wall clock so an
    // external watcher can compute nodes/sec and a projection (ENGC_HEARTBEAT)
    if((c_nodes & ((1LL<<28)-1))==0 && c_nodes>0 && getenv("ENGC_HEARTBEAT")){
        struct timespec hbts; clock_gettime(CLOCK_MONOTONIC,&hbts);
        fprintf(stderr,"HB nodes=%lld t=%ld\n",c_nodes,(long)hbts.tv_sec);
        fflush(stderr);
    }
    if(++c_nodes > NODE_CAP){
        fprintf(stderr,"ENGINE C NODE CAP EXCEEDED base %d k=%d S=[",B,k);
        for(int i=0;i<k;i++) fprintf(stderr,"%d ",S[i]);
        fprintf(stderr,"]\n");
        return -2;
    }
    if(m<=BW_) return leaf_solve(m);
    for(int d=B-1; d>=1; d--){
        if(!(c_avail>>d & 1ULL)) continue;
        int sv_inc = inc_state;
        if(inc_active && inc_state==0){
            int idx = inc_startm - m;
            if(d < inc_h[idx]) break;          // all later d smaller: close node
            inc_state = (d > inc_h[idx]) ? 1 : 0;
        }
        int isD1 = (D1>1 && (u64)d % D1==0);
        // don't consume the sole D1-divisible digit while positions >= P_
        // remain — leaf_solve's exact multiset match handles the rest.
        if(isD1 && c_cntD1==1){ inc_state=sv_inc; continue; }
        u64 sv[MAXM];
        for(int j=0;j<ntq;j++){ sv[j]=c_res[j]; c_res[j]=barrett(c_res[j]*(u64)B+(u64)d, j); }
        u64 svq[2];
        for(int i=0;i<2;i++){
            svq[i]=ql_res[i];
            if(QL_[i]>1){
                u64 x=ql_res[i]*(u64)B+(u64)d;
                u64 qq=(u64)(((u128)x*ql_bm[i])>>64);
                u64 r=x-qq*QL_[i]; while(r>=QL_[i]) r-=QL_[i];
                ql_res[i]=r;
            }
        }
        // rho_ accumulates UNREDUCED: each term < LCM*B < 2^77 and depth <= k,
        // so the u128 sum never overflows; leaf_solve reduces once.
        rho_ += (u128)d*POW_[m-1];
        c_avail &= ~(1ULL<<d); if(isD1) c_cntD1--;
        c_out[m-1]=d;
        int ok=1;
        // Feasibility DPs only inside the critical band (m-1 <= gate_): above
        // it completions almost surely exist so the DPs never prune, and their
        // per-node cost dominated v5 (b37: 31x slower than the v2 scan).
        if(m-1 <= gate_){
        int lrun = longest_run(c_avail);
        // order-2 pruning (existing e2 machinery, unchanged, node-level)
        for(int j=0;j<ne2 && ok;j++){
            { u64 w=(u64)((m-1)/2); u64 w2=(u64)(lrun/2); if(w2<w) w=w2;
              if(w*w >= e2_cosC[j]-1) continue; }  // Thm 1': cannot prune
            int t=e2idx[j];
            if(!e2_check(E2Q[j], c_avail, m-1, c_res[t]%E2Q[j])) ok=0;
        }
        // order 3..6 pruning (generalized order_e_check_m, node-level)
        for(int i=0;i<noe36 && ok;i++){
            u64 q=oe36_q[i]; int e=oe36_e[i];
            { u64 w=(u64)((m-1)/e); u64 w2=(u64)(lrun/2); if(w2<w) w=w2;
              if(w*w >= oe36_cosC[i]-1) continue; }  // Thm 1': cannot prune
            u64 r = c_res[oe36_tqidx[i]] % q;
            u64 bm = modpow_u64((u64)B % q, (u64)(m-1), q);
            u64 target = ((u64)(TARGET_%q) + q - (r*bm)%q) % q;
            if(!order_e_check_m(q, e, c_avail, m-1, target)) ok=0;
        }
        // joint cyclotomic layers, every other band level (cost discipline;
        // the cheap per-modulus marginals above run first and early-out)
        if(ok && QL_[0]>1 && ((gate_-(m-1)) & (QL_[0]<=1024?1:3))==0){
            u64 w=(u64)((m-1)/2), w2=(u64)(lrun/2); if(w2<w) w=w2;
            if(w*w < ql_cosC[0]-1){            // else Thm 1': cannot prune
                u64 q=QL_[0];
                u64 bm=modpow_u64((u64)B%q,(u64)(m-1),q);
                u64 target=((u64)(TARGET_%q)+q-(ql_res[0]*bm)%q)%q;
                if(!order_e_check_m(q,2,c_avail,m-1,target)) ok=0;
            }
        }
        if(ok && QL_[1]>1 && ((gate_-(m-1)) & (QL_[1]<=8192?1:3))==0){
            u64 w=(u64)((m-1)/3), w2=(u64)(lrun/2); if(w2<w) w=w2;
            if(w*w < ql_cosC[1]-1){            // else Thm 1': cannot prune
                u64 q=QL_[1];
                u64 bm=modpow_u64((u64)B%q,(u64)(m-1),q);
                u64 target=((u64)(TARGET_%q)+q-(ql_res[1]*bm)%q)%q;
                if(!order_e_check_m(q,3,c_avail,m-1,target)) ok=0;
            }
        }
        }
        // v12 witness-liveness prune (PEELING-SUPPORT.md §1,§4): at least one
        // admissible suffix-mod-Lnil witness must still have all its digits
        // available, else no completion of this subtree can ever hit a
        // multiple of Lnil (hence of LCM). Cheap early-out loop.
        if(ok && witness_active){
            int live=0;
            for(int t=0;t<nwmasks;t++){
                if((wmasks[t] & ~c_avail)==0){ live=1; break; }
            }
            if(!live) ok=0;
        }
        int rc=0;
        if(ok) rc=engineC_rec(m-1);
        c_avail |= 1ULL<<d; if(isD1) c_cntD1++;
        rho_ -= (u128)d*POW_[m-1];
        ql_res[0]=svq[0]; ql_res[1]=svq[1];
        for(int j=0;j<ntq;j++) c_res[j]=sv[j];
        inc_state = sv_inc;
        if(rc==1) return 1;
        if(rc==-2) return -2;
    }
    return 0;
}

// ---------- v14: CANDIDATE mode forced-prefix machinery ----------
// MODE SPEC: instead of DFS over all `total_n` positions, FIX the assumed
// prefix — the largest (total_n - W) digits of the available set, sorted
// descending, placed at positions total_n-1 .. W — by replaying exactly the
// same residue-update arithmetic engineC_rec's DFS placement block performs
// (barrett c_res update, ql_res update, rho_ accumulation, c_avail/c_cntD1
// bookkeeping, c_out write), but with NO choice: one digit per level, taken
// straight from the sorted list. Then run the existing engineC_rec(W) on the
// remaining W-position tail with the residual target/residues already in
// place. This is deliberately UNVERIFIED for the fixed part (e.g. it does
// not honor the c_cntD1 reservation the real DFS applies) — that is exactly
// why every result produced through this path must carry the mandatory
// CANDIDATE-UNPROVEN label; the widening loop (driven by the callers below)
// is what lets it converge to the true answer once W reaches the actual
// divergence depth.
static void place_digit_forced(int m, int d){
    for(int j=0;j<ntq;j++) c_res[j] = barrett(c_res[j]*(u64)B+(u64)d, j);
    for(int i=0;i<2;i++){
        if(QL_[i]>1){
            u64 x=ql_res[i]*(u64)B+(u64)d;
            u64 qq=(u64)(((u128)x*ql_bm[i])>>64);
            u64 r=x-qq*QL_[i]; while(r>=QL_[i]) r-=QL_[i];
            ql_res[i]=r;
        }
    }
    rho_ += (u128)d*POW_[m-1];
    c_avail &= ~(1ULL<<d);
    if(D1>1 && (u64)d % D1==0) c_cntD1--;
    c_out[m-1]=d;
}

// Runs one forced-prefix + searched-tail attempt for an instance of
// `total_n` positions holding digit set `avail0`, free tail width W (clamped
// to [0,total_n]). On return, if the result is 1, c_out[0..total_n-1] holds
// the FULL candidate arrangement (position-indexed, c_out[total_n-1] = MSB):
// the forced descending prefix is already written into c_out by
// place_digit_forced, and engineC_rec(W) fills c_out[0..W-1]. Caller must
// have already set MOD_/TARGET_ and called setup_engine_params for the
// modulus in force; inc_active/inc_h/inc_startm (if used) must likewise
// already be set by the caller for the W-wide tail before calling this.
static int candidate_attempt(int total_n, u64 avail0, int W){
    if(W<0) W=0;
    if(W>total_n) W=total_n;
    int sorted[MAXK]; int ns=0;
    for(int d=B-1; d>=1; d--) if(avail0>>d & 1ULL) sorted[ns++]=d;
    c_avail = avail0;
    for(int j=0;j<ntq;j++) c_res[j]=0;
    ql_res[0]=0; ql_res[1]=0;
    rho_=0;
    c_cntD1=0;
    if(D1>1) for(int i=0;i<ns;i++) if((u64)sorted[i]%D1==0) c_cntD1++;
    int fixedCount = total_n - W;
    for(int i=0;i<fixedCount;i++){
        int m_cur = total_n - i;   // window size just before placing this digit
        place_digit_forced(m_cur, sorted[i]);
    }
    c_nodes=0; c_leaves=0; c_cands=0;
    return engineC_rec(W);
}

// v12: p-adic suffix-peeling witness-liveness module init. Depends only on
// pps[]/npps/LCM (from build_pps()) and k/S[]/B — callable standalone (no
// DFS state needed), which is how the ENGC_WITNESS cross-check harness
// exercises it without running the search. Returns 0 iff the subset is
// proven infeasible (§4 existence check: zero admissible suffix witnesses),
// in which case the caller (engineC) must fail the subset immediately.
static int witness_init(void){
    witness_active=0; nwmasks=0;
    {
        u128 Lnil=1; int s=0;
        for(int j=0;j<npps;j++){
            if(pps[j].is_nil){ Lnil*=pps[j].q; if(pps[j].t>s) s=pps[j].t; }
        }
        if(Lnil>1){
            // Guard rail 1 (PEELING-SUPPORT.md §5.6): gcd(Lprime, B) == 1.
            u128 Lprime = LCM / Lnil;   // exact: Lnil | LCM by construction
            u64 Lp_mod_B = (u64)(Lprime % (unsigned)B);
            u64 g = gcd64(Lp_mod_B, (u64)B);
            if(g!=1){
                fprintf(stderr,"FATAL: guard rail violated — gcd(Lprime,B)!=1 "
                        "(base %d, g=%llu)\n",B,(unsigned long long)g);
                exit(9);
            }
            // Guard rail 2: B^s == 0 (mod Lnil). Lnil < 2^40, B<=64, s<=6 in
            // practice (B^6 <= 64^6 = 2^36) so u64 arithmetic is exact; guard
            // against the unexpected in case s grows past that.
            if(s>12){
                fprintf(stderr,"NOTE: witness module skipped base %d — s=%d "
                        "too large for guarded u64 B^s computation\n",B,s);
            } else if(Lnil >= ((u128)1<<40)){
                fprintf(stderr,"NOTE: witness module skipped base %d — "
                        "Lnil too large (%llu)\n",B,(unsigned long long)(u64)Lnil);
            } else {
                u64 Lnil64=(u64)Lnil;
                u64 bs=1; for(int i=0;i<s;i++) bs*=(unsigned)B;
                if(bs % Lnil64 != 0){
                    fprintf(stderr,"FATAL: guard rail violated — B^s != 0 "
                            "(mod Lnil) (base %d, s=%d, Lnil=%llu)\n",
                            B,s,(unsigned long long)Lnil64);
                    exit(9);
                }
                // Module active only for s in {2,3} (D1 already covers s<=1).
                if(s>=2 && s<=3){
                    u64 Bpow[3];
                    Bpow[0]=1%Lnil64;
                    for(int i=1;i<s;i++) Bpow[i]=(Bpow[i-1]*(unsigned)B)%Lnil64;
                    int overflow=0;
                    if(s==2){
                        for(int a=0;a<k && !overflow;a++)
                        for(int b=0;b<k && !overflow;b++){
                            if(b==a) continue;
                            u64 val=((u64)S[a]*Bpow[0]+(u64)S[b]*Bpow[1])%Lnil64;
                            if(val==0){
                                u64 mask=(1ULL<<S[a])|(1ULL<<S[b]);
                                int found=0;
                                for(int t=0;t<nwmasks;t++) if(wmasks[t]==mask){found=1;break;}
                                if(!found){
                                    if(nwmasks>=MAXWIT){ overflow=1; break; }
                                    wmasks[nwmasks++]=mask;
                                }
                            }
                        }
                    } else { // s==3
                        for(int a=0;a<k && !overflow;a++)
                        for(int b=0;b<k && !overflow;b++){
                            if(b==a) continue;
                            for(int c=0;c<k;c++){
                                if(c==a||c==b) continue;
                                u64 val=((u64)S[a]*Bpow[0]+(u64)S[b]*Bpow[1]+(u64)S[c]*Bpow[2])%Lnil64;
                                if(val==0){
                                    u64 mask=(1ULL<<S[a])|(1ULL<<S[b])|(1ULL<<S[c]);
                                    int found=0;
                                    for(int t=0;t<nwmasks;t++) if(wmasks[t]==mask){found=1;break;}
                                    if(!found){
                                        if(nwmasks>=MAXWIT){ overflow=1; break; }
                                        wmasks[nwmasks++]=mask;
                                    }
                                }
                            }
                        }
                    }
                    if(overflow){
                        fprintf(stderr,"NOTE: witness enumeration exceeded cap "
                                "(%d) base=%d k=%d Lnil=%llu s=%d — module "
                                "disabled for this subset\n",
                                MAXWIT,B,k,(unsigned long long)Lnil64,s);
                        nwmasks=0; witness_active=0;
                    } else {
                        witness_active=1;
                        if(getenv("ENGC_WITNESS"))
                            fprintf(stderr,"W base=%d k=%d Lnil=%llu s=%d nwitness=%d\n",
                                    B,k,(unsigned long long)Lnil64,s,nwmasks);
                        if(nwmasks==0) return 0; // §4 existence check: infeasible subset
                    }
                } else if(s>3){
                    fprintf(stderr,"NOTE: witness module skipped base %d — "
                            "s=%d exceeds supported tuple size (3)\n",B,s);
                }
            }
        }
    }
    return 1;
}

// v13: shared engine-parameter setup, generalized from engineC()'s original
// inline block so the per-suffix peeled subsearch (peeled_solve(), below) can
// retarget the SAME machinery to (Lc, k-s) instead of (LCM, k) — design
// point 4: "generalize engineC to take (digit set mask, modulus M, target t,
// leaf horizon P_M)". Depends only on ne2/oe36/oe56/QL_ (already built from
// pps[]/npps, unchanged across the split) plus the modulus/position-count
// actually in force, so it is called once per (subset, modulus) pair: once
// by engineC() with (LCM,k), and once per subset (not per witness — the
// witness only changes avail/target) by peeled_solve() with (Lc,k-s).
static void setup_engine_params(u128 modv, int nposi){
    u128 pw=1; int p=0;
    while(pw < modv){ pw*=(unsigned)B; p++; }
    P_ = (p>nposi)? nposi : p;   // clamp: whole window forced -> leaf covers all of it
    BP_ = pw;
    POW_[0]=1%modv;
    for(int i=1;i<nposi;i++) POW_[i]=(POW_[i-1]*(unsigned)B)%modv;
    for(int j=0;j<ne2;j++)  e2_cosC[j]  = coset_size(E2Q[j]);
    for(int i=0;i<noe36;i++) oe36_cosC[i]= coset_size(oe36_q[i]);
    ql_cosC[0] = QL_[0]>1 ? coset_size(QL_[0]) : 1;
    ql_cosC[1] = QL_[1]>1 ? coset_size(QL_[1]) : 1;
    // BW_: widen the leaf window while candidate count B^w/modv stays <= 256 —
    // but ONLY when the band has no DP pruning to lose (ne2==0 && noe36==0,
    // the "blind" bases like B=40): wide leaves replace subtrees the DPs
    // would otherwise kill early, so widening hurts structured bases
    // (measured: b37 34s -> 152s with unconditional widening).
    {
        int w=P_;
        u64 qprod=1;
        for(int j=0;j<ne2;j++){ qprod*=E2Q[j]; if(qprod>1000000){qprod=1000000;break;} }
        for(int i=0;i<noe36 && qprod<1000000;i++){ qprod*=oe36_q[i]; if(qprod>1000000)qprod=1000000; }
        if(qprod <= 32){   // band pruning too weak to beat direct enumeration
            u128 lim=(u128)256*modv; u128 bw=pw;
            while(w<nposi && bw <= lim/(unsigned)B){ bw*=(unsigned)B; w++; }
        }
        BW_=w;
        BPOW_[0]=1;
        for(int i=1;i<=BW_;i++) BPOW_[i]=BPOW_[i-1]*(unsigned)B;
    }
    // gate_ = min{m : m! >= L_eff} + 1, L_eff = modv / (order-1 subpowers):
    // the existence-transition depth from the divergence law (ALGORITHMS.md).
    {
        u128 leff=modv;
        for(int j=0;j<npps;j++) if(!pps[j].is_nil && pps[j].q1>1) leff/=pps[j].q1;
        u128 f=1; int m=1;
        while(f<leff && m<nposi){ m++; f*=(unsigned)m; }
        gate_ = m+1;
    }
}

static int engineC(int *nd_result){
    MOD_ = LCM; TARGET_ = 0;   // ordinary mode: full modulus, target 0
    build_oe36();
    build_barrett();
    build_layers();
    if(!witness_init()) return 0; // §4 existence check: infeasible subset
    setup_engine_params(LCM, k);
    c_avail=setmask;
    for(int j=0;j<ntq;j++) c_res[j]=0;
    rho_=0;
    c_cntD1=0;
    if(D1>1) for(int i=0;i<k;i++) if((u64)S[i]%D1==0) c_cntD1++;
    c_nodes=0; c_leaves=0; c_cands=0;
    inc_active=0;
    int rc=engineC_rec(k);
    if(getenv("ENGC_STATS"))
        fprintf(stderr,"engC stats: B=%d k=%d P=%d gate=%d nodes=%lld leaves=%lld cands=%lld rc=%d\n",
                B,k,P_,gate_,c_nodes,c_leaves,c_cands,rc);
    if(rc==1){ for(int i=0;i<k;i++) nd_result[i]=c_out[i]; g_engine='C'; return 1; }
    return 0; // rc==0 (no arrangement) or rc==-2 (node cap tripped): subset fails
}

// v14 CANDIDATE-mode counterpart of engineC() for the plain (Lnil==1, e.g.
// B prime) path: no witnesses to split over, just the whole k-digit set
// against the full modulus LCM. Fix the top (k-W) digits descending, search
// only the bottom W. Widen W by 1 and retry the WHOLE pass whenever no
// completion is found, up to the loop cap W<=k (which is an exact search,
// guaranteed to match engineC()). Every call is CANDIDATE-UNPROVEN.
static int engineC_candidate(int *nd_result){
    MOD_ = LCM; TARGET_ = 0;
    build_oe36();
    build_barrett();
    build_layers();
    if(!witness_init()) return 0; // §4 existence check still exact: infeasible subset
    setup_engine_params(LCM, k);

    int W0 = (g_guess_mode==1) ? (gate_+1) : g_guess_mode;
    if(W0<1) W0=1;
    if(W0>k) W0=k;

    for(int W=W0; W<=k; W++){
        inc_active=0;
        int rc = candidate_attempt(k, setmask, W);
        if(getenv("ENGC_STATS"))
            fprintf(stderr,"engC candidate stats: B=%d k=%d W=%d nodes=%lld leaves=%lld "
                    "cands=%lld rc=%d\n",B,k,W,c_nodes,c_leaves,c_cands,rc);
        if(rc==1){
            for(int i=0;i<k;i++) nd_result[i]=c_out[i];
            g_final_W=W; g_engine='C';
            return 1;
        }
        fprintf(stderr,"CANDIDATE-UNPROVEN (prefix-guess W=%d): base %d k=%d plain-engineC "
                "path found no completion, widening\n",W,B,k);
    }
    return 0; // exhausted to W==k (== exact search) with no completion: subset truly infeasible
}

// ---------- v13: full per-suffix nilpotent-peeling decomposition ----------
// NILPOTENT-PEELING.md (theorem + "Production implementation" section) and
// PEELING-SUPPORT.md §1 (identity), §5 (guard rails / audit checklist).
// Replaces engineC()'s DFS outright for subsets where the module is active
// (Lnil>1, s>=1, admissible-witness count>0): split N = S + B^s*H, enumerate
// every ordered s-tuple suffix witness u with S_u == sum d_j*B^j == 0 (mod
// Lnil), solve each independent coprime-core instance "H arrangement of
// A_u == S\u achieving H == (Lc-(S_u mod Lc))*inv(B^s mod Lc) (mod Lc)" with
// the SAME engineC_rec/leaf_solve machinery retargeted via MOD_/TARGET_, and
// take the lexicographically maximal combination B^s*H_u + S_u. All non-nil
// tracked-moduli/e2/oe36/layer pruning state is reused byte-for-byte from
// pps[]/npps (built once from the FULL original digit set) — those are
// exactly Lc's own factors (PEELING-SUPPORT.md §1/§5 point 3), independent
// of which s digits a given witness reserves for the suffix.

// u128 extended Euclid: returns a^{-1} mod m. Independent derivation (not
// copied from sol-reference, which the task brief marks untrusted for
// constants/guard-rails, though the algorithm itself is the standard one).
static u128 egcd_inv_u128(u128 a, u128 mod){
    typedef __int128 s128;
    s128 old_r=(s128)(a%mod), r=(s128)mod;
    s128 old_s=1, s=0;
    while(r!=0){
        s128 q=old_r/r;
        s128 t2=old_r-q*r; old_r=r; r=t2;
        t2=old_s-q*s;       old_s=s; s=t2;
    }
    if(old_r!=1){
        fprintf(stderr,"FATAL: egcd_inv_u128 gcd!=1 (base %d) — guard rail "
                "gcd(Lc,B)==1 should make this unreachable\n",B);
        exit(9);
    }
    s128 res=old_s % (s128)mod;
    if(res<0) res += (s128)mod;
    return (u128)res;
}
// Binary (add-and-double) modmul: safe for our magnitudes (Lc comfortably
// under 2^96 in every base we run) without needing a wider intermediate type.
static u128 mulmod_u128(u128 a, u128 b, u128 m){
    a%=m; b%=m;
    u128 result=0;
    while(b>0){
        if(b&1){ result+=a; if(result>=m) result-=m; }
        a<<=1; if(a>=m) a-=m;
        b>>=1;
    }
    return result;
}
static u128 powmod_u128(u128 base, u128 e, u128 m){
    u128 r=1%m; base%=m;
    while(e>0){ if(e&1) r=mulmod_u128(r,base,m); base=mulmod_u128(base,base,m); e>>=1; }
    return r;
}

// ---- witness-tuple enumeration (ordered s-digit suffixes, S_u==0 mod Lnil)
// Extends v12's witness_init (which only tracked bitmasks, and only s in
// {2,3}) to store the actual ordered TUPLES for s>=1, needed because the
// peeled search needs each tuple's suffix value S_u and its exact digit
// order (positions 0..s-1), not just which digits are reserved.
#define MAXWITT 8192
#define MAXWITER 30000000LL
static int   wtup_s;
static u128  wtup_Lnil;
static u128  wtup_Bpow[16];
static u64   wtup_usedmask;
static int   wtup_cur[16];
static int   wtup_digits[MAXWITT][16];
static u128  wtup_Su[MAXWITT];
static int   nwtup;
static long long wtup_iters;
static int   wtup_overflow;

static void wtup_rec(int depth, u128 partial){
    if(wtup_overflow) return;
    if(++wtup_iters > MAXWITER){ wtup_overflow=1; return; }
    if(depth==wtup_s){
        if(partial % wtup_Lnil == 0){
            if(nwtup>=MAXWITT){ wtup_overflow=1; return; }
            u128 su=0;
            for(int i=0;i<wtup_s;i++){ wtup_digits[nwtup][i]=wtup_cur[i]; su += (u128)wtup_cur[i]*wtup_Bpow[i]; }
            wtup_Su[nwtup]=su;
            nwtup++;
        }
        return;
    }
    for(int idx=0; idx<k && !wtup_overflow; idx++){
        int d=S[idx]; u64 bit=1ULL<<d;
        if(wtup_usedmask & bit) continue;
        wtup_usedmask |= bit;
        wtup_cur[depth]=d;
        wtup_rec(depth+1, partial + (u128)d*wtup_Bpow[depth]);
        wtup_usedmask &= ~bit;
    }
}

// per-witness complement-set (A_u) sorted descending, and the sort order
// used for upper-vector domination (design point 5: no arrangement of a
// digit set can lex-exceed that set sorted descending — mined from Sol's
// notes, independently justified: the sorted-descending arrangement IS the
// lexicographic maximum of all arrangements of a fixed multiset).
static int peel_comp[MAXWITT][MAXK];
static int peel_order[MAXWITT];
static int peel_m;
static int peel_cmp(const void*pa,const void*pb){
    int a=*(const int*)pa, b=*(const int*)pb;
    for(int i=0;i<peel_m;i++)
        if(peel_comp[a][i]!=peel_comp[b][i]) return peel_comp[b][i]-peel_comp[a][i];
    return 0;
}

// Returns 1 (arrangement found, nd_result filled), 0 (subset proven
// infeasible — exact, per the theorem: no admissible suffix witness exists),
// or -1 (module not applicable or bailed out; caller MUST fall back to
// engineC() — never silently treated as infeasible).
static int peeled_solve(int *nd_result){
    u128 Lnil=1; int s=0;
    for(int j=0;j<npps;j++) if(pps[j].is_nil){ Lnil*=pps[j].q; if(pps[j].t>s) s=pps[j].t; }
    if(Lnil<=1 || s<1) return -1; // no nilpotent part to peel (e.g. B prime): fall back

    if(s>=16 || Lnil >= ((u128)1<<62)){
        fprintf(stderr,"NOTE: peeled solve skipped base %d k=%d — s=%d/Lnil=%llu "
                "out of guarded range, falling back to engineC\n",
                B,k,s,(unsigned long long)(u64)(Lnil>>0));
        return -1;
    }
    // Guard rail 2 (mandatory): B^s == 0 (mod Lnil).
    {
        u128 bs=1; for(int i=0;i<s;i++) bs*=(unsigned)B;
        if(bs % Lnil != 0){
            fprintf(stderr,"FATAL: guard rail violated — B^s != 0 (mod Lnil) "
                    "base %d s=%d Lnil=%llu\n",B,s,(unsigned long long)(u64)Lnil);
            exit(9);
        }
    }
    u128 Lc = LCM / Lnil;
    if(Lc*Lnil != LCM){
        fprintf(stderr,"FATAL: guard rail violated — Lnil does not divide LCM "
                "exactly, base %d\n",B);
        exit(9);
    }
    // Guard rail 1 (mandatory): gcd(Lc mod B, B) == 1.
    {
        u64 g = gcd64((u64)(Lc % (unsigned)B), (u64)B);
        if(g!=1){
            fprintf(stderr,"FATAL: guard rail violated — gcd(Lc mod B, B)!=1 "
                    "base %d g=%llu\n",B,(unsigned long long)g);
            exit(9);
        }
    }

    // Enumerate admissible ordered s-tuple suffix witnesses.
    nwtup=0; wtup_overflow=0; wtup_iters=0; wtup_usedmask=0;
    wtup_s=s; wtup_Lnil=Lnil;
    wtup_Bpow[0]=1; for(int i=1;i<s;i++) wtup_Bpow[i]=wtup_Bpow[i-1]*(unsigned)B;
    wtup_rec(0,0);
    if(wtup_overflow){
        fprintf(stderr,"NOTE: witness-tuple enumeration overflow base %d k=%d "
                "s=%d — falling back to engineC\n",B,k,s);
        return -1;
    }
    if(nwtup==0) return 0; // §4 existence check: no admissible suffix, exact infeasibility

    int m = k - s;
    peel_m = m;
    inc_startm = m;
    for(int w=0; w<nwtup; w++){
        u64 usedmask=0; for(int i=0;i<s;i++) usedmask |= 1ULL<<wtup_digits[w][i];
        int idx=0;
        for(int i=0;i<k;i++){ int d=S[i]; if(!(usedmask>>d&1)) peel_comp[w][idx++]=d; }
        for(int a=0;a<m;a++) for(int b=a+1;b<m;b++)
            if(peel_comp[w][b]>peel_comp[w][a]){ int t=peel_comp[w][a]; peel_comp[w][a]=peel_comp[w][b]; peel_comp[w][b]=t; }
        peel_order[w]=w;
    }
    qsort(peel_order, nwtup, sizeof(int), peel_cmp);

    // Shared sub-engine setup for modulus Lc (once per subset — reused
    // verbatim across all witnesses, since pps[]/npps/ne2/oe36/QL_ do not
    // depend on which s digits a witness reserves).
    build_oe36();
    build_barrett();
    build_layers();
    u128 save_MOD=MOD_, save_TARGET=TARGET_;
    u64 save_D1=D1; int save_witness_active=witness_active;
    MOD_ = Lc;
    D1 = 1;              // neutralize nil D1 skip-logic: no nil part left to track here
    witness_active = 0;  // neutralize v12's per-node nil witness-liveness prune: superseded
    setup_engine_params(Lc, m);

    u128 Bs_modLc = powmod_u128((u128)B % Lc, (u128)s, Lc);
    u128 invBs = egcd_inv_u128(Bs_modLc, Lc);

    int have=0, best_wit=-1;
    int best_h[MAXK];

    static int hb_on=-1;
    if(hb_on<0) hb_on = getenv("ENGC_HEARTBEAT")!=0;
    int topk = getenv("ENGC_TOPK") ? atoi(getenv("ENGC_TOPK")) : 0;
    for(int oi=0; oi<nwtup; oi++){
        if(topk>0 && oi>=topk){
            fprintf(stderr,"NOTE: ENGC_TOPK=%d — stopping after top-%d witnesses; "
                    "result is a CANDIDATE, not a certificate\n",topk,topk);
            break;
        }
        int w = peel_order[oi];
        if(hb_on){
            struct timespec hbts; clock_gettime(CLOCK_MONOTONIC,&hbts);
            fprintf(stderr,"HB witness %d/%d nodes=%lld t=%ld\n",
                    oi,nwtup,c_nodes,(long)hbts.tv_sec);
            fflush(stderr);
        }
        if(have){
            int cmp=0;
            for(int i=0;i<m;i++){ if(peel_comp[w][i]!=best_h[m-1-i]){ cmp = peel_comp[w][i]>best_h[m-1-i] ? 1 : -1; break; } }
            // sorted descending by upper bound: once one witness's own upper
            // bound (its complement set sorted descending) can't beat the
            // best ACHIEVED high word so far, no later witness in this order
            // can either (design point 5) — safe to stop entirely.
            if(cmp<0) break;
        }
        u128 Su_mod = wtup_Su[w] % Lc;
        u128 t = mulmod_u128((Lc - Su_mod) % Lc, invBs, Lc);

        u64 avail=0; for(int i=0;i<m;i++) avail |= 1ULL<<peel_comp[w][i];
        // incumbent-prefix pruning: seed the subsearch with the best high
        // word found by earlier (higher-bound) witnesses; the DFS abandons
        // any prefix strictly lex-below it (sound: such a completion cannot
        // become the global max; ties allowed, broken later by suffix).
        if(have){ inc_active=1; for(int i=0;i<m;i++) inc_h[i]=best_h[m-1-i]; inc_state=0; }
        else inc_active=0;
        c_avail=avail;
        for(int j=0;j<ntq;j++) c_res[j]=0;
        rho_=0; c_cntD1=0;
        TARGET_ = t;
        c_nodes=0; c_leaves=0; c_cands=0;
        int rc = engineC_rec(m);
        if(getenv("ENGC_STATS"))
            fprintf(stderr,"peel stats: B=%d k=%d s=%d witness=%d Lc=%llu nodes=%lld "
                    "leaves=%lld cands=%lld rc=%d\n",
                    B,k,s,w,(unsigned long long)(u64)Lc,c_nodes,c_leaves,c_cands,rc);
        if(rc==-2){
            fprintf(stderr,"NOTE: peeled subsearch node cap exceeded base %d k=%d "
                    "witness=%d — aborting peeled solve, falling back to engineC\n",B,k,w);
            MOD_=save_MOD; TARGET_=save_TARGET; D1=save_D1; witness_active=save_witness_active;
            return -1;
        }
        if(rc==1){
            int better = !have;
            if(have){
                int cmp=0;
                for(int i=m-1;i>=0;i--){ if(c_out[i]!=best_h[i]){ cmp = c_out[i]>best_h[i] ? 1 : -1; break; } }
                if(cmp>0) better=1;
                else if(cmp==0 && wtup_Su[w] > wtup_Su[best_wit]) better=1; // tie-break: larger suffix value
            }
            if(better){ have=1; best_wit=w; memcpy(best_h,c_out,sizeof(int)*m); }
        }
    }
    MOD_=save_MOD; TARGET_=save_TARGET; D1=save_D1; witness_active=save_witness_active;
    if(!have) return 0;

    for(int i=0;i<s;i++) nd_result[i]  = wtup_digits[best_wit][i];
    for(int i=0;i<m;i++) nd_result[s+i]= best_h[i];
    g_engine='P';
    return 1;
}

// v14 CANDIDATE mode counterpart of peeled_solve(). Same witness
// enumeration/ordering/setup as peeled_solve() (shared verbatim: guard
// rails, witness-tuple enumeration, per-witness descending-complement sort
// peel_comp[]/peel_order[], shared Lc sub-engine setup), but instead of a
// full engineC_rec(m) DFS per witness it runs candidate_attempt(m,avail,W)
// — forced descending prefix of width m-W, searched tail of width W — and
// widens W across the WHOLE witness pass when no witness at the current W
// produces a completion. Returns 1 (candidate found, nd_result filled),
// 0 (exact infeasibility per the peeling theorem: no admissible suffix
// witness exists at all — same as peeled_solve()'s 0), or -1 (module not
// applicable / bailed out, caller must fall back to engineC_candidate()).
// Every successful return here is CANDIDATE-UNPROVEN: the m-window result
// is a guess above the search band, not a certified maximum.
static int peeled_solve_candidate(int *nd_result){
    u128 Lnil=1; int s=0;
    for(int j=0;j<npps;j++) if(pps[j].is_nil){ Lnil*=pps[j].q; if(pps[j].t>s) s=pps[j].t; }
    if(Lnil<=1 || s<1) return -1;

    if(s>=16 || Lnil >= ((u128)1<<62)){
        fprintf(stderr,"NOTE: peeled candidate solve skipped base %d k=%d — s=%d/Lnil=%llu "
                "out of guarded range, falling back to engineC_candidate\n",
                B,k,s,(unsigned long long)(u64)(Lnil>>0));
        return -1;
    }
    {
        u128 bs=1; for(int i=0;i<s;i++) bs*=(unsigned)B;
        if(bs % Lnil != 0){
            fprintf(stderr,"FATAL: guard rail violated — B^s != 0 (mod Lnil) "
                    "base %d s=%d Lnil=%llu\n",B,s,(unsigned long long)(u64)Lnil);
            exit(9);
        }
    }
    u128 Lc = LCM / Lnil;
    if(Lc*Lnil != LCM){
        fprintf(stderr,"FATAL: guard rail violated — Lnil does not divide LCM "
                "exactly, base %d\n",B);
        exit(9);
    }
    {
        u64 g = gcd64((u64)(Lc % (unsigned)B), (u64)B);
        if(g!=1){
            fprintf(stderr,"FATAL: guard rail violated — gcd(Lc mod B, B)!=1 "
                    "base %d g=%llu\n",B,(unsigned long long)g);
            exit(9);
        }
    }

    nwtup=0; wtup_overflow=0; wtup_iters=0; wtup_usedmask=0;
    wtup_s=s; wtup_Lnil=Lnil;
    wtup_Bpow[0]=1; for(int i=1;i<s;i++) wtup_Bpow[i]=wtup_Bpow[i-1]*(unsigned)B;
    wtup_rec(0,0);
    if(wtup_overflow){
        fprintf(stderr,"NOTE: witness-tuple enumeration overflow base %d k=%d "
                "s=%d — falling back to engineC_candidate\n",B,k,s);
        return -1;
    }
    if(nwtup==0) return 0; // exact infeasibility (theorem-level), unaffected by candidate mode

    int m = k - s;
    peel_m = m;
    for(int w=0; w<nwtup; w++){
        u64 usedmask=0; for(int i=0;i<s;i++) usedmask |= 1ULL<<wtup_digits[w][i];
        int idx=0;
        for(int i=0;i<k;i++){ int d=S[i]; if(!(usedmask>>d&1)) peel_comp[w][idx++]=d; }
        for(int a=0;a<m;a++) for(int b=a+1;b<m;b++)
            if(peel_comp[w][b]>peel_comp[w][a]){ int t=peel_comp[w][a]; peel_comp[w][a]=peel_comp[w][b]; peel_comp[w][b]=t; }
        peel_order[w]=w;
    }
    qsort(peel_order, nwtup, sizeof(int), peel_cmp);

    build_oe36();
    build_barrett();
    build_layers();
    u128 save_MOD=MOD_, save_TARGET=TARGET_;
    u64 save_D1=D1; int save_witness_active=witness_active;
    MOD_ = Lc;
    D1 = 1;
    witness_active = 0;
    setup_engine_params(Lc, m);

    u128 Bs_modLc = powmod_u128((u128)B % Lc, (u128)s, Lc);
    u128 invBs = egcd_inv_u128(Bs_modLc, Lc);

    int W0 = (g_guess_mode==1) ? (gate_+1) : g_guess_mode;
    if(W0<1) W0=1;
    if(W0>m) W0=m;
    if(getenv("ENGC_GUESS_DEBUG"))
        fprintf(stderr,"DBG peeled_candidate: base=%d k=%d s=%d m=%d Lc=%llu gate_=%d W0=%d "
                "nwtup=%d P_=%d BW_=%d ne2=%d noe36=%d noe56=%d QL0=%llu QL1=%llu\n",
                B,k,s,m,(unsigned long long)(u64)Lc,gate_,W0,nwtup,P_,BW_,ne2,noe36,noe56,
                (unsigned long long)QL_[0],(unsigned long long)QL_[1]);

    int best_h[MAXK];
    for(int W=W0; W<=m; W++){
        int have=0, best_wit=-1;
        for(int oi=0; oi<nwtup; oi++){
            int w = peel_order[oi];
            if(have){
                // Domination bound: peel_comp[w][i] is the i-th largest digit
                // of witness w's complement (i=0 => would-be MSB); best_h is
                // position-indexed with best_h[m-1] == MSB. Compare MSB-first
                // by reading best_h back-to-front — NOT the same index as
                // peel_comp[w][i] directly (that indexing pairs a rank with a
                // position and is not a valid lexicographic comparison; fixed
                // here for the new candidate-mode comparison).
                int cmp=0;
                for(int i=0;i<m;i++){
                    int bh = best_h[m-1-i];
                    if(peel_comp[w][i] != bh){ cmp = peel_comp[w][i]>bh ? 1 : -1; break; }
                }
                if(cmp<0) break;
            }
            u128 Su_mod = wtup_Su[w] % Lc;
            u128 t = mulmod_u128((Lc - Su_mod) % Lc, invBs, Lc);
            u64 avail=0; for(int i=0;i<m;i++) avail |= 1ULL<<peel_comp[w][i];
            TARGET_ = t;
            if(have){
                inc_active=1; inc_startm=W;
                for(int idx2=0; idx2<W; idx2++) inc_h[idx2]=best_h[W-1-idx2];
                inc_state=0;
            } else inc_active=0;
            int rc = candidate_attempt(m, avail, W);
            if(getenv("ENGC_STATS"))
                fprintf(stderr,"peel candidate stats: B=%d k=%d s=%d witness=%d Lc=%llu W=%d "
                        "nodes=%lld leaves=%lld cands=%lld rc=%d\n",
                        B,k,s,w,(unsigned long long)(u64)Lc,W,c_nodes,c_leaves,c_cands,rc);
            if(rc==1){
                int better = !have;
                if(have){
                    int cmp=0;
                    for(int i=m-1;i>=0;i--){ if(c_out[i]!=best_h[i]){ cmp = c_out[i]>best_h[i] ? 1 : -1; break; } }
                    if(cmp>0) better=1;
                    else if(cmp==0 && wtup_Su[w] > wtup_Su[best_wit]) better=1;
                }
                if(better){ have=1; best_wit=w; memcpy(best_h,c_out,sizeof(int)*m); }
            }
        }
        if(have){
            for(int i=0;i<s;i++) nd_result[i]  = wtup_digits[best_wit][i];
            for(int i=0;i<m;i++) nd_result[s+i]= best_h[i];
            g_final_W=W; g_engine='P';
            MOD_=save_MOD; TARGET_=save_TARGET; D1=save_D1; witness_active=save_witness_active;
            return 1;
        }
        fprintf(stderr,"CANDIDATE-UNPROVEN (prefix-guess W=%d): base %d k=%d s=%d — no "
                "witness completed, widening\n",W,B,k,s);
    }
    MOD_=save_MOD; TARGET_=save_TARGET; D1=save_D1; witness_active=save_witness_active;
    return 0; // exhausted to W==m (== exact per-witness search) with no completion
}

// ---------- driver ----------
static int best_nd[MAXK]; static int best_k=0;
static int best_W=0;   // v14: g_final_W snapshot for the current best_nd (CANDIDATE mode)

static int nd_greater(const int *a,int ka,const int *b,int kb){
    if(ka!=kb) return ka>kb;
    for(int i=ka-1;i>=0;i--){ if(a[i]!=b[i]) return a[i]>b[i]; }
    return 0;
}

int solve_base(int base,int verbose){
    B=base; best_k=0; g_iters=0;
    long long checked=0, scanned=0;
    char used_engine='-';
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    int nD=B-1;
    for(k=nD;k>=1;k--){
        if(best_k>k) break;
        int c[MAXK]; for(int i=0;i<k;i++) c[i]=i;
        int done=0;
        while(!done){
            for(int i=0;i<k;i++) S[i]=B-1-c[i];
            checked++;
            if(best_k==k){
                int nd[MAXK]; for(int i=0;i<k;i++) nd[k-1-i]=S[i];
                if(!nd_greater(nd,k,best_nd,best_k)) break;
            }
            setmask=0; for(int i=0;i<k;i++) setmask|=1ULL<<S[i];
            if(build_pps() && subset_filters()){
                build_tracked();
                scanned++;
                int nd[MAXK];
                // v13 dispatch: try the per-suffix peeled solve first; it
                // returns -1 (module not applicable, or bailed out with a
                // loud stderr NOTE) whenever engineC() must be used instead
                // — Lnil==1 (e.g. B prime), witness enumeration overflow, or
                // a node-cap trip inside a subsearch. Never silently skipped.
                // v14: when CANDIDATE mode (ENGC_GUESS) is active, dispatch to
                // the forced-prefix/widening counterparts instead; every path
                // through them is CANDIDATE-UNPROVEN, never a certified solve.
                // v15: peeled_solve_candidate() re-paid a full W-tail search
                // once per per-suffix witness (dozens to hundreds) for every
                // W in the widening loop — a severe performance bug. Go
                // straight to engineC_candidate() unconditionally instead;
                // it is a single global search per W with all the same
                // pruning (CRT DPs, cyclotomic layers, D1 bookkeeping, v12
                // witness-liveness prune) applied inside engineC_rec().
                // peeled_solve_candidate() itself is left defined but is now
                // dead code from this dispatch site.
                int got;
                if(g_guess_mode){
                    if(getenv("ENGC_GUESS_DEBUG")){
                        fprintf(stderr,"DBG subset attempt: base=%d k=%d S=[",B,k);
                        for(int di=0;di<k;di++) fprintf(stderr,"%d ",S[di]);
                        fprintf(stderr,"]\n"); fflush(stderr);
                    }
                    got = engineC_candidate(nd);
                } else {
                    int pr;
                    pr = peeled_solve(nd);
                    got = (pr==-1) ? engineC(nd) : pr;
                }
                if(got){
                    if(nd_greater(nd,k,best_nd,best_k)){
                        if(best_k==k) fprintf(stderr,
                            "UNIQUENESS VIOLATION base %d\n",B);
                        memcpy(best_nd,nd,sizeof(int)*k); best_k=k;
                        used_engine=g_engine;
                        if(g_guess_mode) best_W=g_final_W;
                    }
                }
            }
            int i=k-1;
            while(i>=0 && c[i]==nD-k+i) i--;
            if(i<0) done=1;
            else { c[i]++; for(int j=i+1;j<k;j++) c[j]=c[j-1]+1; }
        }
        if(best_k==k) break;
    }
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double secs=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)/1e9;
    if(best_k){
        char dec[600]; digits_to_decimal(best_nd,best_k,B,dec);
        printf("%d %s",B,dec);
        if(g_guess_mode)
            printf(" CANDIDATE-UNPROVEN (prefix-guess W=%d)",best_W);
        if(verbose){
            printf("  [eng=%c subsets=%lld scanned=%lld t=%.3fs]",used_engine,checked,scanned,secs);
        }
        printf("\n"); fflush(stdout);
        if(g_guess_mode)
            fprintf(stderr,"CANDIDATE-UNPROVEN (prefix-guess W=%d): base=%d result=%s t=%.3fs "
                    "[NOT A CERTIFIED SOLVE]\n",best_W,B,dec,secs);
        return 1;
    }
    printf("%d FAILED\n",B); fflush(stdout);
    if(g_guess_mode)
        fprintf(stderr,"CANDIDATE-UNPROVEN (prefix-guess W=%d): base=%d FAILED even at W=k/m "
                "(exact-search width) — genuinely infeasible, not a mode artifact\n",best_W,B);
    return 0;
}

int main(int argc,char**argv){
    int lo=2,hi=48,verbose=1;
    if(argc>1) lo=atoi(argv[1]);
    if(argc>2) hi=atoi(argv[2]);
    if(argc>3) verbose=atoi(argv[3]);
    if(hi>MAXB) hi=MAXB;
    {
        char *ge = getenv("ENGC_GUESS");
        if(ge){
            int v = atoi(ge);
            if(v>0){
                g_guess_mode=v;
                fprintf(stderr,"CANDIDATE-UNPROVEN mode ACTIVE (ENGC_GUESS=%d) — all results "
                        "below are unproven prefix-guess candidates, not certified solves.\n",v);
            }
        }
    }
    for(int b=lo;b<=hi;b++) solve_base(b,verbose);
    return 0;
}
