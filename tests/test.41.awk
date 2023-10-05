# This tests sending long arrays from one node to another

assign(NR,10)  { 
	if(NR % (RANK+2)) {
		message[NR] = $0
		nelem++
	}
} 

END {
	to = (RANK + 1 ) % SIZE
        from = (SIZE + RANK - 1) % SIZE 

	print "Node " RANK " sending " nelem " element array to " to

	sendrecv(message,to,other,from)

	print "Node " RANK " got an array of length " length(other) " from " from
	if(RANK == 1) for(i in other) print i
}
