BEGIN {

	a[0] = "freedom"
	a[1] = "beer"

	if(RANK == 1) peer = 0;
	else peer = 1;

	send a[RANK] > peer
	recv a[peer] < peer

	print "here is a" , a[RANK] , a[peer] 
}

