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
    u64 target = (q - (r*bm)%q)%q;
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
static u128 POW_[MAXK+2];   // B^i mod LCM
static u128 BP_;            // B^P (exact, not reduced)
static int  P_;             // leaf window width
static u128 rho_;           // prefix residue: sum d*B^pos (mod LCM) over placed digits
static u64  c_avail;
static u64  c_res[MAXM];
static int  c_out[MAXK];    // c_out[pos] = digit at place value B^pos
static int  c_cntD1;
static long long c_nodes;
static long long c_leaves, c_cands;
static int  gate_;   // run feasibility DPs only when remaining m <= gate_:
                     // above it (m! >> L_eff) they essentially never prune and
                     // per-node DP cost dominates the whole search (v5 lesson).
static char g_engine;

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
    if(m==0) return (rho_ % LCM)==0;
    u128 v0 = (LCM - (rho_ % LCM)) % LCM;
    if(v0 >= BPOW_[m]) return 0;         // no candidate fits in the window
    u128 span = BPOW_[m] - 1 - v0;
    u128 j = span / LCM;
    int digs[MAXK];
    for(;;){
        c_cands++;
        u128 v = v0 + j*LCM;
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
    if(++c_nodes > NODE_CAP){
        fprintf(stderr,"ENGINE C NODE CAP EXCEEDED base %d k=%d S=[",B,k);
        for(int i=0;i<k;i++) fprintf(stderr,"%d ",S[i]);
        fprintf(stderr,"]\n");
        return -2;
    }
    if(m<=BW_) return leaf_solve(m);
    for(int d=B-1; d>=1; d--){
        if(!(c_avail>>d & 1ULL)) continue;
        int isD1 = (D1>1 && (u64)d % D1==0);
        // don't consume the sole D1-divisible digit while positions >= P_
        // remain — leaf_solve's exact multiset match handles the rest.
        if(isD1 && c_cntD1==1) continue;
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
            u64 target = (q - (r*bm)%q) % q;
            if(!order_e_check_m(q, e, c_avail, m-1, target)) ok=0;
        }
        // joint cyclotomic layers, every other band level (cost discipline;
        // the cheap per-modulus marginals above run first and early-out)
        if(ok && QL_[0]>1 && ((gate_-(m-1)) & (QL_[0]<=1024?1:3))==0){
            u64 w=(u64)((m-1)/2), w2=(u64)(lrun/2); if(w2<w) w=w2;
            if(w*w < ql_cosC[0]-1){            // else Thm 1': cannot prune
                u64 q=QL_[0];
                u64 bm=modpow_u64((u64)B%q,(u64)(m-1),q);
                u64 target=(q-(ql_res[0]*bm)%q)%q;
                if(!order_e_check_m(q,2,c_avail,m-1,target)) ok=0;
            }
        }
        if(ok && QL_[1]>1 && ((gate_-(m-1)) & (QL_[1]<=8192?1:3))==0){
            u64 w=(u64)((m-1)/3), w2=(u64)(lrun/2); if(w2<w) w=w2;
            if(w*w < ql_cosC[1]-1){            // else Thm 1': cannot prune
                u64 q=QL_[1];
                u64 bm=modpow_u64((u64)B%q,(u64)(m-1),q);
                u64 target=(q-(ql_res[1]*bm)%q)%q;
                if(!order_e_check_m(q,3,c_avail,m-1,target)) ok=0;
            }
        }
        }
        int rc=0;
        if(ok) rc=engineC_rec(m-1);
        c_avail |= 1ULL<<d; if(isD1) c_cntD1++;
        rho_ -= (u128)d*POW_[m-1];
        ql_res[0]=svq[0]; ql_res[1]=svq[1];
        for(int j=0;j<ntq;j++) c_res[j]=sv[j];
        if(rc==1) return 1;
        if(rc==-2) return -2;
    }
    return 0;
}

static int engineC(int *nd_result){
    // P_ = min{p : B^p >= LCM}, BP_ = B^P_ (exact)
    u128 pw=1; int p=0;
    while(pw < LCM){ pw*=(unsigned)B; p++; }
    P_ = (p>k)? k : p;   // clamp: if even the whole k-digit window is forced, leaf covers all of it
    BP_ = pw;
    POW_[0]=1%LCM;
    for(int i=1;i<k;i++) POW_[i]=(POW_[i-1]*(unsigned)B)%LCM;
    build_oe36();
    build_barrett();
    build_layers();
    for(int j=0;j<ne2;j++)  e2_cosC[j]  = coset_size(E2Q[j]);
    for(int i=0;i<noe36;i++) oe36_cosC[i]= coset_size(oe36_q[i]);
    ql_cosC[0] = QL_[0]>1 ? coset_size(QL_[0]) : 1;
    ql_cosC[1] = QL_[1]>1 ? coset_size(QL_[1]) : 1;
    // BW_: widen the leaf window while candidate count B^w/LCM stays <= 256 —
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
            u128 lim=(u128)256*LCM; u128 bw=pw;
            while(w<k && bw <= lim/(unsigned)B){ bw*=(unsigned)B; w++; }
        }
        BW_=w;
        BPOW_[0]=1;
        for(int i=1;i<=BW_;i++) BPOW_[i]=BPOW_[i-1]*(unsigned)B;
    }
    // gate_ = min{m : m! >= L_eff} + 1, L_eff = LCM / (order-1 subpowers):
    // the existence-transition depth from the divergence law (ALGORITHMS.md).
    {
        u128 leff=LCM;
        for(int j=0;j<npps;j++) if(!pps[j].is_nil && pps[j].q1>1) leff/=pps[j].q1;
        u128 f=1; int m=1;
        while(f<leff && m<k){ m++; f*=(unsigned)m; }
        gate_ = m+1;
    }
    c_avail=setmask;
    for(int j=0;j<ntq;j++) c_res[j]=0;
    rho_=0;
    c_cntD1=0;
    if(D1>1) for(int i=0;i<k;i++) if((u64)S[i]%D1==0) c_cntD1++;
    c_nodes=0; c_leaves=0; c_cands=0;
    int rc=engineC_rec(k);
    if(getenv("ENGC_STATS"))
        fprintf(stderr,"engC stats: B=%d k=%d P=%d gate=%d nodes=%lld leaves=%lld cands=%lld rc=%d\n",
                B,k,P_,gate_,c_nodes,c_leaves,c_cands,rc);
    if(rc==1){ for(int i=0;i<k;i++) nd_result[i]=c_out[i]; g_engine='C'; return 1; }
    return 0; // rc==0 (no arrangement) or rc==-2 (node cap tripped): subset fails
}

// ---------- driver ----------
static int best_nd[MAXK]; static int best_k=0;

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
                if(engineC(nd)){
                    if(nd_greater(nd,k,best_nd,best_k)){
                        if(best_k==k) fprintf(stderr,
                            "UNIQUENESS VIOLATION base %d\n",B);
                        memcpy(best_nd,nd,sizeof(int)*k); best_k=k;
                        used_engine=g_engine;
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
        if(verbose){
            printf("  [eng=%c subsets=%lld scanned=%lld t=%.3fs]",used_engine,checked,scanned,secs);
        }
        printf("\n"); fflush(stdout);
        return 1;
    }
    printf("%d FAILED\n",B); fflush(stdout);
    return 0;
}

int main(int argc,char**argv){
    int lo=2,hi=48,verbose=1;
    if(argc>1) lo=atoi(argv[1]);
    if(argc>2) hi=atoi(argv[2]);
    if(argc>3) verbose=atoi(argv[3]);
    if(hi>MAXB) hi=MAXB;
    for(int b=lo;b<=hi;b++) solve_base(b,verbose);
    return 0;
}
