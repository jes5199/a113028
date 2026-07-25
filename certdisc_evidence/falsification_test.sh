# FALSIFICATION TEST: regions that PROVABLY contain a completion must report FEASIBLE>=1.
# Take a known-good completion, truncate its prefix by n digits, and census that region.
# The true completion's own digit-subset lies inside, so a correct necessary-condition
# test MUST find at least one feasible subset. Zero would prove false-infeasibles.
run() { # base drops W prefix label
  L=$(CERTDISC_REGION_DESC=0 CERTDISC_REGION_PREFIX=$4 CERTSET_W=$3 ./carrytrie_cert.new19 certdisc $1 "$2" 3000000 2>&1 | grep -oE 'tested=[0-9]+ subsets, FEASIBLE=[0-9]+')
  echo "  $5 -> $L"
}
echo "### b63 engine-confirmed (B=63 W=22 prefixLen=32) -- each region CONTAINS the known completion ###"
run 63 "9,18,27,28,36,45,54" 22 62,61,60,59,58,57,56,55,53,52,51,50,49,48,47,46,44,43,42,41,40,39,38,37,35,34,33,32,31,30,24,16 "truncate 0 (C(pool,0) subsets)"
run 63 "9,18,27,28,36,45,54" 22 62,61,60,59,58,57,56,55,53,52,51,50,49,48,47,46,44,43,42,41,40,39,38,37,35,34,33,32,31,30 "truncate 2 (C(pool,2) subsets)"
run 63 "9,18,27,28,36,45,54" 22 62,61,60,59,58,57,56,55,53,52,51,50,49,48,47,46,44,43,42,41,40,39,38,37,35,34,33,32 "truncate 4 (C(pool,4) subsets)"
run 63 "9,18,27,28,36,45,54" 22 62,61,60,59,58,57,56,55,53,52,51,50,49,48,47,46,44,43,42,41,40,39,38,37,35,34 "truncate 6 (C(pool,6) subsets)"
echo "### b60 certified (B=60 W=22 prefixLen=24) -- each region CONTAINS the known completion ###"
run 60 "5,10,15,20,24,25,30,35,40,45,50,55" 22 59,58,57,56,54,53,52,51,49,48,47,46,44,43,42,41,39,38,37,36,34,33,32,31 "truncate 0 (C(pool,0) subsets)"
run 60 "5,10,15,20,24,25,30,35,40,45,50,55" 22 59,58,57,56,54,53,52,51,49,48,47,46,44,43,42,41,39,38,37,36,34,33 "truncate 2 (C(pool,2) subsets)"
run 60 "5,10,15,20,24,25,30,35,40,45,50,55" 22 59,58,57,56,54,53,52,51,49,48,47,46,44,43,42,41,39,38,37,36 "truncate 4 (C(pool,4) subsets)"
run 60 "5,10,15,20,24,25,30,35,40,45,50,55" 22 59,58,57,56,54,53,52,51,49,48,47,46,44,43,42,41,39,38 "truncate 6 (C(pool,6) subsets)"
echo "### b64 incumbent (B=64 W=24 prefixLen=38) -- each region CONTAINS the known completion ###"
run 64 "" 24 63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,31,30,29,28,27,26,25 "truncate 0 (C(pool,0) subsets)"
run 64 "" 24 63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,31,30,29,28,27 "truncate 2 (C(pool,2) subsets)"
run 64 "" 24 63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,31,30,29 "truncate 4 (C(pool,4) subsets)"
run 64 "" 24 63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,31 "truncate 6 (C(pool,6) subsets)"
