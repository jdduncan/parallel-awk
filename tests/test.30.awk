BEGIN {

	split("rabbit bear deer elk antelope racoon opossum bird frog", a)
	mystr = a[RANK+1]
	print RANK " " mystr
	lo = reduce(min(mystr))
 	hi = reduce(max(mystr))
 	
	if(RANK == 0)  print "n=" SIZE ": " lo " sorts low, " hi " sorts hi."
}
