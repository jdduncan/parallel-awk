# Verify sorting network specifications.

BEGIN  { nrec = 0 }


/^#/ || /^$/    { next }  # skip comments and blank lines in spec

{   if(!nrounds) nrounds = NF
    for(i=1; i <= nrounds ; i++)
      p[nrec,i] = $i
    nrec++
}
  

END {
  for(i = 0 ; i < nrec ; i++) { 
    for(j = 1 ; j <= nrounds ; j++ ) {
      x = p[i,j]
      if(x != "b" && p[x,j] != i) {
        errors++
        print "error at row " i ", round " j "; x is " x
        print "but row " x " round " j " is " p[x,j]
      }
    }
  }
  if(!errors) printf("Spec OK: n=%d, %d rounds\n",nrec,nrounds)
}
