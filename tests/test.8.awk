
### This should compile and run in awk:

BEGIN	{  
	if(PARALLEL) k=reduce(sum(j)) 
	else print "OK"
	} 



