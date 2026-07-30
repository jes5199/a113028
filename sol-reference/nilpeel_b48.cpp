#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

using u64 = std::uint64_t;
using u128 = unsigned __int128;
using s128 = __int128;

static constexpr int B = 48;
static constexpr int LEAF = 11;

static u128 parse128(const char *s) {
    u128 x=0; while(*s) x=10*x+unsigned(*s++-'0'); return x;
}
static u128 invmod(u128 a,u128 m){
    s128 r0=a,r1=m,s0=1,s1=0;
    while(r1){ s128 q=r0/r1; s128 r=r0-q*r1;r0=r1;r1=r;
        s128 s=s0-q*s1;s0=s1;s1=s; }
    s0%=s128(m); if(s0<0)s0+=s128(m); return u128(s0);
}

struct Search {
    u128 mod,target,pow[18],bound;
    u64 avail,nodes=0,leaves=0,candidates=0;
    int out[18]{};
    bool leaf(u128 rho){
        ++leaves; u128 r=rho%mod, v0=(target+mod-r)%mod;
        if(v0>=bound)return false;
        u128 j=(bound-1-v0)/mod;
        for(;;){
            ++candidates; u128 v=v0+j*mod; u64 mask=0; int dgt[LEAF]; bool ok=1;
            for(int i=0;i<LEAF;i++){
                int d=int(v%B);v/=B;
                if(!d || (mask>>d&1)){ok=0;break;} dgt[i]=d;mask|=u64{1}<<d;
            }
            if(ok && v==0 && mask==avail){
                for(int i=0;i<LEAF;i++) out[i]=dgt[i];
                return true;
            }
            if(!j) break;
            --j;
        }
        return false;
    }
    bool dfs(int m,u128 rho){
        ++nodes;if(m==LEAF)return leaf(rho);
        for(int d=47;d>=1;d--){u64 bit=u64{1}<<d;if(!(avail&bit))continue;
            avail^=bit;out[m-1]=d;
            if(dfs(m-1,rho+u128(d)*pow[m-1]))return true;
            avail^=bit;
        }
        return false;
    }
};

struct Pattern { int d0,d1,d2; std::vector<int> upper; };

static bool lex_greater(const int *a,const int *b,int n){
    for(int i=n-1;i>=0;i--)if(a[i]!=b[i])return a[i]>b[i];
    return false;
}

int main(int argc,char **argv){
    int candidate=argc>1?std::atoi(argv[1]):21;
    int only=argc>2?std::atoi(argv[2]):-1;
    const u128 L=parse128("110680160865928453800"), nil=216, mod=L/nil;
    const u128 invBT=invmod(u128(B)*B*B%mod,mod);
    std::vector<int>D;for(int d=1;d<48;d++)if(d!=16&&d!=32&&d!=46)D.push_back(d);
    std::vector<int>prefix;
    for(int d:{47,45,44,43,42,41,40,39,38,37,36,35,34,33,31,30,29,28,27,26,25,23,22})prefix.push_back(d);
    std::vector<int>A;
    for(int d:D)if(std::find(prefix.begin(),prefix.end(),d)==prefix.end()&&d!=candidate)A.push_back(d);

    u128 prefix_mod=0,bp=1;
    for(int pos=0;pos<=43;pos++){
        if(pos==20)prefix_mod=(prefix_mod+u128(candidate)*bp)%mod;
        else if(pos>=21){int d=prefix[43-pos];prefix_mod=(prefix_mod+u128(d)*bp)%mod;}
        bp=bp*B%mod;
    }
    std::vector<Pattern>patterns;
    for(int d0:A)for(int d1:A)for(int d2:A){
        if(d0==d1||d0==d2||d1==d2)continue;
        u128 S=u128(d0)+u128(B)*d1+u128(B)*B*d2;
        if(S%nil)continue;
        Pattern p{d0,d1,d2,{}};
        for(int d:A)if(d!=d0&&d!=d1&&d!=d2)p.upper.push_back(d);
        std::sort(p.upper.rbegin(),p.upper.rend());patterns.push_back(std::move(p));
    }
    std::sort(patterns.begin(),patterns.end(),[](const Pattern&a,const Pattern&b){return a.upper>b.upper;});
    std::cout<<"candidate="<<candidate<<" patterns="<<patterns.size()<<"\n";

    bool have=0;int best[18]{};Pattern bestp{};u64 total_leaves=0,total_cands=0;double total_sec=0;
    for(int pi=0;pi<(int)patterns.size();pi++){
        if(only>=0&&pi!=only)continue;
        Pattern&p=patterns[pi];
        if(have){
            int cmp=0;
            for(int i=0;i<17;i++) if(p.upper[i]!=best[16-i]){
                cmp=p.upper[i]>best[16-i]?1:-1; break;
            }
            if(cmp<0)continue;
            if(cmp==0){
                const int lo[3]={p.d2,p.d1,p.d0};
                const int blo[3]={bestp.d2,bestp.d1,bestp.d0};
                for(int i=0;i<3;i++)if(lo[i]!=blo[i]){cmp=lo[i]>blo[i]?1:-1;break;}
                if(cmp<=0)continue;
            }
        }
        Search s;s.mod=mod;s.pow[0]=1;for(int i=1;i<18;i++)s.pow[i]=s.pow[i-1]*B%mod;
        s.bound=1;for(int i=0;i<LEAF;i++)s.bound*=B;
        s.avail=0;for(int d:A)if(d!=p.d0&&d!=p.d1&&d!=p.d2)s.avail|=u64{1}<<d;
        u128 S=u128(p.d0)+u128(B)*p.d1+u128(B)*B*p.d2;
        s.target=((mod-prefix_mod+mod-(S%mod))%mod)*invBT%mod;
        auto st=std::chrono::steady_clock::now();bool found=s.dfs(17,0);
        double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-st).count();
        total_leaves+=s.leaves;total_cands+=s.candidates;total_sec+=sec;
        std::cout<<"  p="<<pi<<" low="<<p.d2<<','<<p.d1<<','<<p.d0
                 <<" found="<<found<<" leaves="<<s.leaves<<" sec="<<sec<<"\n";
        if(found&&(!have||lex_greater(s.out,best,17))){have=1;std::copy(s.out,s.out+17,best);bestp=p;}
    }
    std::cout<<"result="<<have<<" total_leaves="<<total_leaves<<" candidates="<<total_cands<<" seconds="<<total_sec<<"\n";
    if(have){
        std::cout<<"high=";for(int i=16;i>=0;i--)std::cout<<best[i]<<(i?',':'\n');
        std::cout<<"low="<<bestp.d2<<','<<bestp.d1<<','<<bestp.d0<<"\n";
    }
}
