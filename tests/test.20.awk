

assign(NR,10)	{ i++ }

END	{ print "Process " RANK " read " i " lines " 
	  j = reduce(sum(i))
	  if(RANK==0) print " ... Total lines read: " j
	}
