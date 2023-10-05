

NR % SIZE == RANK  { i++ }

END { 
	x = RANK + 3
	j=reduce(or( x ) )
	k=reduce(and( x ))
	l=reduce(sum( x  ))
	m=reduce(max( x ))
	n=reduce(prod( x))
	o=reduce(min( x))
	print "RANK: " RANK " x: " x " OR: " j " AND: " k " SUM: " l " MAX: " m " MIN: " o " PROD: " n
} 



