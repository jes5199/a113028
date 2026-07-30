#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gmpxx.h>

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

#define digit int
#define multiple mpz_class
#define USE_MPZ_CLASS


void fatal(char * s){
  printf("%s\n",s);
  abort();
}

void * xmalloc (size_t size)
{
 register void *value = malloc (size);
 if (value == 0)
   fatal ("virtual memory exhausted");
 return value;
}

void * xrealloc (void *ptr, size_t size)
{
 register void *value = realloc (ptr, size);
 if (value == 0)
   fatal ("Virtual memory exhausted");
 return value;
}

multiple gcd(multiple a, multiple b){
  if (b == 0)
    return a;
  else
    return gcd(b, a % b);
}

multiple lcm(multiple a, multiple b){
  return (a*b)/(gcd(a,b));
}

multiple ary_lcm(digit * ary){
  int i;
  multiple injection;
  injection = ary[0];
  for(i = 1; (injection > 0) && (ary[i] > 0); i++){
    injection = lcm(injection,ary[i]);
  }
  if(injection <= 0){return 1;}
  return injection;
}

multiple ary_sum(digit * ary){
  int i;
  multiple injection;
  injection = ary[0];
  for(i = 1; (injection > 0) && (ary[i] > 0); i++){
    injection += ary[i];
  }
  return injection;
}

digit ary_length(digit * ary){
  int i;
  for(i = 0; ary[i]; i++){}
  return i;
}

void inspect_memory(digit * ary, int length){
  int i;
  printf("(");
  for(i = 0; i < length; i++){
    printf("%d ",ary[i]);
  }
  printf(")\n");
}

void inspect_ary(digit * ary){
  int i;
  printf("[");
  for(i = 0; ary[i]; i++){
    printf("%d ",ary[i]);
  }
  printf("]\n");
}

char * DIG[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "&alpha;", "&beta;", "&gamma;", "&delta;", "&epsilon;", "&zeta;", "&eta;", "&theta;", "&iota;", "&kappa;", "&lambda;", "&mu;", "&nu;", "&xi;", "&omicron;", "&pi;", "&rho;", "&sigma;", "&tau;", "&upsilon;", "&phi;", "&chi;", "&psi;", "&omega;", "&Alpha;", "&Beta;", "&Gamma;", "&Delta;", "&Epsilon;", "&Zeta;", "&Eta;", "&Theta;", "&Iota;", "&Kappa;", "&Lambda;", "&Mu;", "&Nu;", "&Xi;", "&Omicron;", "&Pi;", "&Rho;", "&Sigma;", "&Tau;", "&Upsilon;", "&Phi;", "&Chi;", "&Psi;", "&Omega;"};

void inspect_digits(digit * ary){
  int i;
  for(i = 0; ary[i]; i++){
    printf("%s",DIG[ary[i]]);
  }
  printf("\n");
}

int number_size;

void make_subset(digit * active, int base, int length = 0){
  int slack = 0;
  int i = 1;
  int backup = 0;
  multiple my_sum;
  multiple my_lcm;
  
  if(!length){
    length = base - 1;
  }
  slack = base - 1 - length;
  printf("%d ",length);
  //printf("\n",length);
  //printf("%d slack\n",slack);

  active[0] = base - 1;
  active[1] = 0;
  my_sum = active[0];
  while(i < length){
    if(backup){
      while(!slack){
	if(i > 0){
	  slack += (active[i - 1] - active[i]) - 1;
	  my_sum -= active[i];
	  i--;
	  //printf("rewind\n");
	} else {
	  //printf("fail\n");
	  make_subset(active, base, length - 1);
	  return;
	}
      }
      //printf("%d=%d<\%dn",i,active[i],active[i] - 1);
      active[i]--;
      my_sum--;
      slack--;
      backup = 0;
    } else {
      active[i] = active[i - 1] - 1 ;
      //printf("%d=%d\n",i,active[i]);
      my_sum += active[i];
    }
    active[i+1] = 0;
    my_lcm = ary_lcm(active);
    if(my_lcm % base == 0){ // ten rule
      backup = 1;
      //printf("ten\n");
    }
    if( (i == length - 1) && ( my_sum % gcd(base - 1, my_lcm) != 0 ) ) { // nine rule
      backup = 1;
    }
    if(!backup){
      i++;
    }
  }
}

int permute_below(digit base, digit * self, digit * other, int my_length, digit five){
  //digit target[256];
  //memcpy(target,other,sizeof(target));

  digit * target;
  target = other;
    
  int digits_left[256];
  int i;
  digit d;
  for(i = 0; i < base; i++){
    digits_left[i] = 0;
  }
  //inspect_memory(target,my_length);
  for(i = 0; i < my_length; i++){
    d = self[i];
    digits_left[d] = 1;
  }
  if(five){
    if(digits_left[five]){
      digits_left[five] = 0;
    } else {
      five = 0;
    }
  }
  for(i = 0; i < my_length ; i++){
    for(d = base; d >= 0; d--){ //find the greatest digits lessthan target[i]
      if(digits_left[d] && d <= target[i]){
	break;
      }
    }
    self[i] = d;
    //printf("plop %d\n",d);
    if(d >= 0){digits_left[d] = 0;} else {
      if((five) && (i == my_length - 1)){
	//printf("plop five\n",d);
	self[i] = five;
      }
    }
    if(d != target[i]){
      break;
    }
  }
  //inspect_memory(self,my_length);
  while(self[i] < 0){
    i--;
    if(i < 0){
      //printf("failed");
      return 0;
    }
    //printf("correcting %d\n", self[i]);
    digits_left[self[i]] = 1;
    for(d = base; d >= 0; d--){ //find the greatest digits lessthan target[i]
      if(digits_left[d] && d < self[i]){
	break;
      }
    }
    self[i] = d;
    if(d >= 0){digits_left[d] = 0;}
    //printf("corrected %d\n", self[i]);
  }
  d = base - 1;
  //inspect_memory(self,my_length);
  while(++i < my_length){
    for(d = d; d >= 0; d--){ //find the greatest digits lessthan target[i]
      if(digits_left[d]){
	break;
      }
    }
    if(d < 0){
      self[i] = five;
    } else {
      self[i] = d--;
    }
  }
  return 1;
}

int round_to(digit * target, int base, int my_length, digit * self, multiple m){
  //int my_length = ary_length(self);
  int i;
  //digit * target;
  //target = (digit *) xmalloc(256 * sizeof(digit));
  memcpy(target,self,sizeof(digit) * (my_length + 1));
  multiple remainder = 0;
  for(i = 0; i < my_length; i++){
    remainder *= base;
    remainder += self[i];
    remainder %= m;
  }

  if(remainder <= 0){return 0;}

  for(i--; i >= 0; i--){
    multiple foo = (remainder % base);
#ifdef USE_MPZ_CLASS
    target[i] = target[i] - foo.get_si();
#else
    target[i] = target[i] - foo;
#endif
    if(target[i] < 0){
      target[i - 1] -= 1;
      if(target[i - 1] < 0){return 0;}
      target[i] += base;
    }
    remainder /= base;
  }
  return 1;
}

int test_ary_lcm(){
  digit test_ary[10];
  test_ary[0] = 2;
  test_ary[1] = 3;
  test_ary[2] = 4;
  test_ary[3] = 6;
  test_ary[4] = 0;
  if(ary_lcm(test_ary) != 12){
    return 0;
  }
  test_ary[0] = 7;
  if(ary_lcm(test_ary) != 84){
    return 0;
  }
  test_ary[1] = 0;
  if(ary_lcm(test_ary) != 7){
    return 0;
  }
  test_ary[0] = 0;
  if(ary_lcm(test_ary) != 1){
    return 0;
  }
  return 1;
}

int test_permute_below_five(){
  digit test_ary[10];
  digit test_ary2[10];
  test_ary[0] = 5;
  test_ary[1] = 4;
  test_ary[2] = 3;
  test_ary[3] = 2;
  test_ary[4] = 1;
  test_ary[5] = 0;
  
  test_ary2[0] = 5;
  test_ary2[1] = 4;
  test_ary2[2] = 0;
  test_ary2[3] = 0;
  test_ary2[4] = 0;
  test_ary2[5] = 0;

  permute_below(10,test_ary, test_ary2, 5, 3);
  if(  
    test_ary[0] == 5 &&
    test_ary[1] == 2 &&
    test_ary[2] == 4 &&
    test_ary[3] == 1 &&
    test_ary[4] == 3 &&
    test_ary[5] == 0
  ){ /* ok */ } else { return 0; }
 
  //printf("foo\n");
  test_ary[0] = 3;
  test_ary[1] = 2;
  test_ary[2] = 1;
  test_ary[3] = 0;
  
  test_ary2[0] = 3;
  test_ary2[1] = 1;
  test_ary2[2] = 2;
  test_ary2[3] = 0;

  permute_below(4,test_ary, test_ary2, 3, 2);
  if(  
    test_ary[0] == 3 &&
    test_ary[1] == 1 &&
    test_ary[2] == 2 &&
    test_ary[3] == 0
  ){ /* ok */ } else { return 0; }
 
  return 1;
}

int test_permute_below(){
  digit test_ary[10];
  digit test_ary2[10];
  test_ary[0] = 5;
  test_ary[1] = 4;
  test_ary[2] = 3;
  test_ary[3] = 2;
  test_ary[4] = 1;
  test_ary[5] = 0;
  
  test_ary2[0] = 5;
  test_ary2[1] = 4;
  test_ary2[2] = 0;
  test_ary2[3] = 0;
  test_ary2[4] = 0;
  test_ary2[5] = 0;

  permute_below(6, test_ary, test_ary2, 5, 0);
  if(  
    test_ary[0] == 5 &&
    test_ary[1] == 3 &&
    test_ary[2] == 4 &&
    test_ary[3] == 2 &&
    test_ary[4] == 1 &&
    test_ary[5] == 0
  ){ /* ok */ } else { return 0; }
  
  test_ary[0] = 5;
  test_ary[1] = 4;
  test_ary[2] = 3;
  test_ary[3] = 2;
  test_ary[4] = 1;
  test_ary[5] = 0;
  
  test_ary2[0] = 5;
  test_ary2[1] = 4;
  test_ary2[2] = 3;
  test_ary2[3] = 2;
  test_ary2[4] = 1;
  test_ary2[5] = 0;

  permute_below(10,test_ary, test_ary2, 5, 0);
  if(  
    test_ary[0] == 5 &&
    test_ary[1] == 4 &&
    test_ary[2] == 3 &&
    test_ary[3] == 2 &&
    test_ary[4] == 1 &&
    test_ary[5] == 0
  ){ /* ok */ } else { 

    inspect_memory(test_ary, 5);
    return 0; 
  }
  
  return 1;
}

int test_round_to(){
  digit test_ary[10];
  digit * test_result;
  test_result = (digit *) xmalloc(6 * sizeof(digit));
  test_ary[0] = 5;
  test_ary[1] = 4;
  test_ary[2] = 3;
  test_ary[3] = 2;
  test_ary[4] = 1;
  test_ary[5] = 0;
  round_to(test_result, 10, 5, test_ary, 25);
  if(  
    test_result[0] == 5 &&
    test_result[1] == 4 &&
    test_result[2] == 3 &&
    test_result[3] == 0 &&
    test_result[4] == 0 &&
    test_result[5] == 0
  ){ /* ok */ } else { return 0; }

  test_ary[0] = 4;
  test_ary[1] = 3;
  test_ary[2] = 1;
  test_ary[3] = 0;
  test_ary[4] = 0;
  test_ary[5] = 0;
  round_to(test_result, 5, 3, test_ary, 12);
  if(  
    test_result[0] == 4 &&
    test_result[1] == 1 &&
    test_result[2] == 3 &&
    test_result[3] == 0 &&
    test_result[4] == 0 &&
    test_result[5] == 0
  ){ /* ok */ } else { return 0; }
  //inspect_memory(test_result, 5);
  free(test_result);
  return 1;
}

void runtests(){
  if(!test_ary_lcm()){
    fatal("failed lcm test");
  }
  if(!test_permute_below()){
    fatal("failed permute_below test");
  }
  if(!test_permute_below_five()){
    fatal("failed permute_below with five test");
  }
  if(!test_round_to()){
    fatal("failed round_to test");
  }
  
}

main(){
  runtests();
  int base = 42;
  digit temporary[255];
  digit active[255];
  digit topdigit;
  for(base = 2; base <= 36; base++){
    topdigit = 0;
    int i;
    int count;
    int my_length;
    digit five;
    fflush ( stdout );

    printf("\n%d: ", base);
    make_subset(active,base);
    printf("\n");
    multiple active_lcm = ary_lcm(active);
    inspect_ary(active);
    my_length = ary_length(active);
    if(base % 2){
      five = 0;
    } else {
      five = base / 2;
      printf("( five is %d )\n", five);
    }
    while(round_to(temporary,base,my_length,active,active_lcm)){
      //inspect_ary(temporary);
      permute_below(base,active,temporary,my_length,five);
      //inspect_ary(active);
    }
    topdigit = active[0];
    inspect_digits(active);
  }
}



