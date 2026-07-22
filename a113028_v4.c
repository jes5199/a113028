//  A113028 v2: two engines per surviving subset, alternated with doubling budgets.
//  Engine S (scan): descend through multiples of lcm(S) with digit-prefix skip.
//    Wins when lcm is large relative to the gap (multiples sparser than arrangements).
//  Engine D (DFS): descend through arrangements MSB-first, brute-solving the last
//    T digits per prefix, pruning prefixes by order-2 split feasibility and
//    forced-last-digit availability. Wins when lcm is small (arrangements sparser
//    in the modular sense: expected prefix trials ~ prod(leaf moduli)/T!).
// Order-1 moduli are filtered at subset level and cost nothing at runtime.
// Subpower fix: for each prime power q=p^a, filter on the largest subpower with
// multiplicative order 1 (digit-sum) and order <=2 (split feasibility) — orders
// can drop on subpowers, and missing this makes dead subsets undetectable.
//
// build:  gcc -O2 -march=native -o a113028 a113028.c
// run:    ./a113028 [lo] [hi] [verbose]     e.g.  ./a113028 2 48 1

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
static int order_e_check(u64 q, int e){
    if(q<=1) return 1;
    int c[8]; for(int j=0;j<e;j++) c[j]=0;
    for(int i=0;i<k;i++) c[i%e]++;
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
        if(!(setmask>>d & 1ULL)) continue;
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
    // full assignment: all tracked n_j == c_j  <=>  rem == M-1; need s==0
    return cur[0*M + (M-1)] ? 1 : 0;
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

// ---------- engine S: resumable multiple-scan ----------
typedef struct { int nd[MAXK]; int active; } ScanState;

static int asc_[MAXK];

// Suffix-incremental residue maintenance: H[k]=0; H[i]=(H[i+1]*B+nd[i])%LCM,
// so r=H[0]. H[i] depends only on nd[i..k-1] (the digits at position i and
// above), so if digits only change at positions <= dirty, H stays valid for
// all i > dirty and we only need to recompute H[dirty..0]. This turns the
// per-iteration O(k) residue recompute into O(dirty), where dirty is usually
// small because the borrow-subtract and skip/refill mutations mostly touch
// low digits. Full recompute (dirty=k-1) happens once per scan_run entry
// (rare: only on cap-doubling resumption), which keeps ScanState itself
// unchanged (no need to persist H across calls).
static int scan_run(ScanState *st, long long budget){
    int *nd=st->nd;
    int rd[64];
    long long it=0;
    static u128 H[MAXK+1];
    H[k]=0;
    for(int i=k-1;i>=0;i--) H[i]=(H[i+1]*(unsigned)B+(unsigned)nd[i])%LCM;
    int dirty=-1; // H fully valid after the recompute above
    for(;;){
        if(it++>=budget) return -1;
        g_iters++;
        for(int i=dirty;i>=0;i--) H[i]=(H[i+1]*(unsigned)B+(unsigned)nd[i])%LCM;
        u128 r=H[0];
        int hi1=-1;
        if(r){
            int rl=0; u128 rr=r;
            while(rr){ rd[rl++]=(int)(rr%(unsigned)B); rr/=(unsigned)B; }
            int borrow=0;
            for(int i=0;i<k;i++){
                int sub=(i<rl?rd[i]:0)+borrow;
                int old=nd[i];
                int v=nd[i]-sub;
                if(v<0){ v+=B; borrow=1; } else borrow=0;
                nd[i]=v;
                if(v!=old) hi1=i;
            }
            if(borrow) return 0;
        }
        { int less=0,more=0;
          for(int i=k-1;i>=0;i--){
              if(nd[i]<asc_[i]){ less=1; break; }
              if(nd[i]>asc_[i]){ more=1; break; } }
          if(less&&!more) return 0; }
        u64 used=0; int p=-1;
        for(int i=k-1;i>=0;i--){
            u64 bit=1ULL<<nd[i];
            if(nd[i]==0 || !(setmask&bit) || (used&bit)){ p=i; break; }
            used|=bit;
        }
        if(p<0) return 1;
        int hi2=-1;
        for(int i=0;i<p;i++){ if(nd[i]!=B-1) hi2=i; nd[i]=B-1; }
        int i=p;
        for(;;){ nd[i]--; if(nd[i]>=0) break; nd[i]=B-1; i++; if(i>=k) return 0; }
        if(i>hi2) hi2=i;
        dirty = hi1>hi2?hi1:hi2;
    }
}

// ---------- engine D: prefix DFS + tail brute ----------
static int T_tail;
static u64 d_avail;
static u64 d_res[MAXM];
static int d_out[MAXK];
static long long d_budget;
static int d_cntD1;

static int tail_rec(int m){
    if(g_nodes++>d_budget) return -2;
    if(m==0){
        for(int j=0;j<ntq;j++) if(d_res[j]) return 0;
        return 1;
    }
    for(int d=B-1;d>=1;d--){
        if(!(d_avail>>d&1)) continue;
        if(m==1 && D1>1 && (u64)d%D1) continue;
        int isD1 = (D1>1 && (u64)d%D1==0);
        if(m>1 && isD1 && d_cntD1==1) continue;
        u64 sv[MAXM];
        for(int j=0;j<ntq;j++){ sv[j]=d_res[j]; d_res[j]=(d_res[j]*(u64)B+(u64)d)%TQ[j]; }
        d_avail&=~(1ULL<<d); if(isD1) d_cntD1--;
        d_out[k-m]=d;
        int rc=tail_rec(m-1);
        d_avail|=1ULL<<d; if(isD1) d_cntD1++;
        for(int j=0;j<ntq;j++) d_res[j]=sv[j];
        if(rc) return rc;
    }
    return 0;
}

static int dfs_rec(int m){
    if(g_nodes++>d_budget) return -2;
    if(m<=T_tail) return tail_rec(m);
    for(int d=B-1;d>=1;d--){
        if(!(d_avail>>d&1)) continue;
        int isD1=(D1>1 && (u64)d%D1==0);
        if(isD1 && d_cntD1==1) continue;
        u64 sv[MAXM];
        for(int j=0;j<ntq;j++){ sv[j]=d_res[j]; d_res[j]=(d_res[j]*(u64)B+(u64)d)%TQ[j]; }
        d_avail&=~(1ULL<<d); if(isD1) d_cntD1--;
        d_out[k-m]=d;
        int ok=1;
        for(int j=0;j<ne2 && ok;j++){
            int t=e2idx[j];
            if(!e2_check(E2Q[j], d_avail, m-1, d_res[t]%E2Q[j])) ok=0;
        }
        int rc=0;
        if(ok) rc=dfs_rec(m-1);
        d_avail|=1ULL<<d; if(isD1) d_cntD1++;
        for(int j=0;j<ntq;j++) d_res[j]=sv[j];
        if(rc) return rc;
    }
    return 0;
}

static int dfs_run(long long budget, int *nd){
    d_avail=setmask;
    for(int j=0;j<ntq;j++) d_res[j]=0;
    d_cntD1=0;
    if(D1>1) for(int i=0;i<k;i++) if((u64)S[i]%D1==0) d_cntD1++;
    d_budget=budget; g_nodes=0;
    T_tail = (k<7)? k : 7;
    int rc=dfs_rec(k);
    if(rc==1){ for(int i=0;i<k;i++) nd[k-1-i]=d_out[i]; return 1; }
    if(rc==-2) return -1;
    return 0;
}

// ---------- hybrid ----------
static char g_engine;
static int hybrid(int *nd_result){
    for(int i=0;i<k;i++) asc_[i]=S[i];
    ScanState ss; for(int i=0;i<k;i++) ss.nd[k-1-i]=S[i]; ss.active=1;
    long long cap=1<<18;
    for(;;){
        int rc=dfs_run(cap,nd_result);
        if(rc==1){ g_engine='D'; return 1; }
        if(rc==0) return 0;
        if(ss.active){
            rc=scan_run(&ss,cap);
            if(rc==1){ memcpy(nd_result,ss.nd,sizeof(int)*k); g_engine='S'; return 1; }
            if(rc==0) return 0;
        }
        cap*=4;
        if(cap>=(1LL<<24)){
            fprintf(stderr,"b%d k=%d cap=%lld S=[",B,k,cap);
            for(int i=0;i<k;i++) fprintf(stderr,"%d ",S[i]);
            fprintf(stderr,"]\n");
        }
        if(cap > (1LL<<44)){ fprintf(stderr,"base %d: budget blown on a subset\n",B); exit(3); }
    }
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
                if(hybrid(nd)){
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
