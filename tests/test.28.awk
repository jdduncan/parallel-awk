
BEGIN	{ partner = (RANK + (SIZE / 2)) % SIZE 

	  strings[0] = "0. frivolous"
	  strings[1] = "1. inopportune"
	  strings[2] = "2. decadent"
	  strings[3] = "3. unwarranted"

	  send strings[RANK] > partner
	  recv j < partner

	  print RANK " got <<" j ">> from " partner
	}

