#
# ... but is it really a sorting network?
#
# Theorem Z (Zero-one principle):  If a network with n input lines sorts
# all 2^n sequences of 0s and 1s into nondecreasing order, it will sort
# any arbitrary sequence of n numbers into nondecreasing order.

# This script will build a set of input files to test an n-line sorter

# Run it with e.g. "awk -v n=4 -f prover.awk" 

BEGIN {
  for(i = 0 ; i < 2^n ; i++ ) {
    for(j = 0 ; j < n ; j++) {
      outfile = "in." j
      x = (i % (2^(j+1)) < 2^j ? 0 : 1)
      printf("%d\n",x) > outfile
    }
  }
}

