

NR % SIZE == RANK  { if ( /googlebot/ )  { i++ } }

END { 
	j=reduce(sum(i))
	print "I am node " RANK " and i is " i " and j is " j
} 



