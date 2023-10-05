
assign(NR,10)  && /googlebot/   { i++ }

END { 
      j=reduce(sum(i))
      if(RANK==0) print j " googles"
    }
