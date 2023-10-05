BEGIN {
   if(RANK == 0) {
	print "RANK: " RANK
 	print "dog: "  hash("dog")
	print "cat: "  hash("cat")
	print "freddie: " hash("freddie")
	print "3: " hash(3)
	a=3
	print "a (which is 3): " hash(a)
   }
}
