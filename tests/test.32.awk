BEGIN {

	a[17] = "freedom"
	a[28] = "beer"

	if(RANK == 1) peer = 0;
	else peer = 1;

	send a > peer
	recv b < peer

	print "here is b" , b[17] , b[28]
}

