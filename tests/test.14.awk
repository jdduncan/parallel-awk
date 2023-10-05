

BEGIN { 
	# when you reduce a string, the value reduced is its length.
	# elements.  
	if (RANK == 0) j =""
	else j="When the moon had risen, the lion began speaking."
	k=reduce(and(j)) 
	print k
	if (k == 0) print "test 14 passed."
} 



