

NR % SIZE == RANK  && /googlebot/   { i++ }

END { 
	print "I am node " RANK " and i is " i
	j=reduce(sum(i)) 
	print "and j is " j
} 



