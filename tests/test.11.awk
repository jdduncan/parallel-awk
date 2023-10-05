

assign(NR,100) { if(/googlebot/) i++ }

END {
       t=reduce(sum(i))
       if(RANK==0) print "Process " RANK " Googles: " t
    } 
