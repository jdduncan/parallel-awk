
END {
	o = RANK
	c = comm_split ( (RANK == 0), 0)
	comm_set(c)
	printf("RANK was %d and c is %d and now RANK is %d/%d ",
		o,c,RANK,SIZE)
	comm_set(0)
	printf(" ... and %d again.\n",RANK) 
} 
