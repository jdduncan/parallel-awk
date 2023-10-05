# Run in 4 processes, this implements a four-element sorting 
# network, cf. Knuth _Searching and Sorting_, 2nd ed., p.223.
# 
# Each process reads a line from an input file (in.1 through in.4),
# then the network sorts the lines, and the top-sorting line is written
# into file out.1, the next line into out.2, etc.

BEGIN {

  # Each sort requires three rounds of comparisons;
  # a node uses its line of the Partners[] array 
  # to find its partners for each round.

  Partners[0] = "1 3 1"
  Partners[1] = "0 2 0"
  Partners[2] = "3 1 3"
  Partners[3] = "2 0 2"
  
  split(Partners[RANK],p)
    
  infile = "in." RANK
  outfile = "out." RANK  

  while(getline x < infile > 0) {
    for(i=1; i<4; i++) {
      sendrecv(x,p[i],y)
      x = choose(RANK,x,p[i],y)
    }
    barrier()
    print x > outfile
  }
}

  
function choose(my_rank,my_val,your_rank,your_val) {
    if (my_rank < your_rank) 
      return (your_val > my_val ? your_val : my_val )
    else
      return (your_val < my_val ? your_val : my_val )
}

