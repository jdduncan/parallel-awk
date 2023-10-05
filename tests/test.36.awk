

assign(NR,5) && msglen < 20000  { 
	message = message "\n" $0 
	msglen += length($0) + 1
} 

END {
	partner = (RANK + 1 ) % SIZE

	print "Node " RANK " sending message of length " msglen

	sendrecv(message,partner,other)

	print "Node " RANK " got a message of length " length(other)
}
