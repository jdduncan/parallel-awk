
BEGIN	{ half = SIZE / 2 
	  partner = RANK % half
	  if (partner == RANK) partner += half
	}

assign(NR,12)	{ i += NR  }

END 	{ sendrecv(i,partner,j)
	  
	  printf("I am %d, I got %d; my partner, %d, got %d.\n",
		 RANK, i, partner, j)

	}
