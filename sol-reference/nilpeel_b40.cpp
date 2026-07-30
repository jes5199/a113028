#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
using u64=std::uint64_t;using u128=unsigned __int128;using s128=__int128;
static constexpr int B=40,LEAF=8;
static u128 invmod(u128 a,u128 m){s128 r0=a,r1=m,s0=1,s1=0;while(r1){s128 q=r0/r1,t=r0-q*r1;r0=r1;r1=t;t=s0-q*s1;s0=s1;s1=t;}s0%=s128(m);if(s0<0)s0+=s128(m);return u128(s0);}
struct Search{u128 mod,target,pow[16],bound;u64 avail,nodes=0,leaves=0,cands=0;int out[16]{};
 bool leaf(u128 rho){++leaves;u128 r=rho%mod,v0=(target+mod-r)%mod;if(v0>=bound)return 0;u128 j=(bound-1-v0)/mod;for(;;){++cands;u128 v=v0+j*mod;u64 mask=0;int ds[LEAF];bool ok=1;for(int i=0;i<LEAF;i++){int d=int(v%B);v/=B;if(!d||(mask>>d&1)){ok=0;break;}ds[i]=d;mask|=u64{1}<<d;}if(ok&&v==0&&mask==avail){for(int i=0;i<LEAF;i++)out[i]=ds[i];return 1;}if(!j)break;--j;}return 0;}
 bool dfs(int m,u128 rho){++nodes;if(m==LEAF)return leaf(rho);for(int d=39;d;--d){u64 bit=u64{1}<<d;if(!(avail&bit))continue;avail^=bit;out[m-1]=d;if(dfs(m-1,rho+u128(d)*pow[m-1]))return 1;avail^=bit;}return 0;}};
int main(int argc,char**argv){int only=argc>1?std::atoi(argv[1]):-1;const u128 L=18050444111700ULL,mod=L/100,inv=invmod(u128(B)*B%mod,mod);
 std::vector<int>D;for(int d=1;d<40;d++)if(d!=8&&d!=16&&d!=24&&d!=32&&d!=37)D.push_back(d);std::vector<int>pre={39,38,36,35,34,33,31,30,29,28,27,26,25,23,22,21,19};
 std::vector<int>A;for(int d:D)if(std::find(pre.begin(),pre.end(),d)==pre.end())A.push_back(d);u128 pm=0,bp=1;for(int pos=0;pos<34;pos++){if(pos>=17)pm=(pm+u128(pre[33-pos])*bp)%mod;bp=bp*B%mod;}
 std::vector<std::pair<int,int>>P;for(int a:A)for(int b:A)if(a!=b&&(a+B*b)%100==0)P.push_back({a,b});std::sort(P.begin(),P.end(),[&](auto x,auto y){auto up=[&](auto z){std::vector<int>v;for(int d:A)if(d!=z.first&&d!=z.second)v.push_back(d);std::sort(v.rbegin(),v.rend());return v;};return up(x)>up(y);});
 std::cout<<"patterns="<<P.size()<<"\n";for(int pi=0;pi<(int)P.size();pi++){if(only>=0&&pi!=only)continue;auto[d0,d1]=P[pi];Search s;s.mod=mod;s.pow[0]=1;for(int i=1;i<16;i++)s.pow[i]=s.pow[i-1]*B%mod;s.bound=1;for(int i=0;i<LEAF;i++)s.bound*=B;s.avail=0;for(int d:A)if(d!=d0&&d!=d1)s.avail|=u64{1}<<d;u128 S=d0+u128(B)*d1;s.target=((mod-pm+mod-S%mod)%mod)*inv%mod;auto st=std::chrono::steady_clock::now();bool f=s.dfs(15,0);double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-st).count();std::cout<<pi<<" low="<<d1<<','<<d0<<" found="<<f<<" leaves="<<s.leaves<<" cands="<<s.cands<<" sec="<<sec<<"\n";if(f){std::cout<<"high=";for(int i=14;i>=0;i--)std::cout<<s.out[i]<<(i?',':'\n');}}
}
