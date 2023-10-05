
BEGIN	{ half = SIZE / 2 
	  partner = RANK % half
	  if (partner == RANK) partner += half
	}

assign(NR,12)	{ i += NR  }

END 	{ 
          send i > partner
	  recv j < partner
	  
	  printf("I am %d, I got %d; my partner, %d, got %d.\n",
		 RANK, i, partner, j)

	}
