BEGIN {

	a[0] = "freedom"
	a[1] = "beer"

        b[2] = "libraries"

	if(RANK == 1) peer = 0;
	else peer = 1;

	send a > peer
	recv b < peer

        if(RANK == 0) for(i in b) print i, b[i]
}

