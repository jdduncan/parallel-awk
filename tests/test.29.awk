
BEGIN	{ 
	  strings[0] = "0. frivolous"
	  strings[1] = "1. inopportune"
	  strings[2] = "2. decadent"
	  strings[3] = "3. unwarranted"

          s = strings[RANK] 

	  r=comm_set(comm_split((RANK % 2),0))
          if(SIZE == 1 ) print "all alone."
	  else {
	    partner = RANK ? 0 : 1;
	    send s > partner
	    recv j < partner
            print RANK " in comm " r " got \"" j "\" from " partner
	  }
	}

