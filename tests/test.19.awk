
BEGIN  { blk = SIZE * 4 }

assign(NR,blk)	{ i++ }

END	{ print "Process " RANK " read " i " lines " 
	  j = reduce(sum(i))
	  if(RANK==0) print " ... Block size " blk ", Total lines read: " j
	}
