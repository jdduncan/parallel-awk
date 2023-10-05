# A generalized sorting network, 
# as in Knuth, _Searching and Sorting_, 2nd ed., pp. 219-234.
# 
# Read a sorting network specification;
# then read values from in.x, sort them in the network,
# and write to out.x.

BEGIN  { nrec = 0 }

/^#/ || /^$/    { next }  # skip comments and blank lines in spec


{   if (nrec++ == RANK) nrounds = split($0,p)   }
  
  
END {
  if (nrec != SIZE) {
    print "invalid specification",nrec,SIZE
    exit
  }
  infile = "in." RANK
  outfile = "out." RANK
 
  while(getline x < infile > 0) {
    for(i=1 ; i <= nrounds ; i++) {
      if(p[i] != "b") {
        sendrecv(x,p[i],y)
        if (RANK < p[i]) {
          if (y > x) x = y
        }
        else if (y < x) x = y
      } 
    } 
    # barrier()
    print x > outfile
  }
}
