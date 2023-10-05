
		{ i++ }
NR == 2000	{ print "Waiting ..." 
		  barrier()
		  print "OK at barrier 1, read " i " lines " }

END 		{ print "Waiting ..." 
		  barrier()
		  print "OK at barrier 2, read " i " lines " }
