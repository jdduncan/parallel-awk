


assign(NR,3)  { print "Process " RANK " gets line " NR " and P is " P++ }
assign(NR,4)  {
		printf("    %d at %d ",RANK,NR)
		if ((int(NR/4) % SIZE) != RANK) 
			printf(" -- That's wrong!\n")
		else printf("\n")
	      }
 
