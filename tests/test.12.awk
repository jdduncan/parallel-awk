

BEGIN { 
	# when you reduce a string, the value reduced is its length.
	# elements.  
	j="That night, there was a gathering by the great oak of all good creatures."
	k=reduce(sum(j)) 
	print length(j)
	print k
} 



