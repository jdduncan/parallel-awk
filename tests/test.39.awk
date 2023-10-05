# In this test, three nodes send very long messages to their neighbors,
# but one node sends a short message

assign(NR,5)  { 
	message = message "\n" $0 
	msglen += length($0) + 1
} 

END {
	to = (RANK + 1 ) % SIZE
        from = (SIZE + RANK - 1) % SIZE 

        if(RANK == 2) {
		message = "hi"
		msglen = 2
	}

	print "Node " RANK " sending message of length " msglen " to " to

	sendrecv(message,to,other,from)

	print "Node " RANK " got a message of length " length(other) " from " from
}
