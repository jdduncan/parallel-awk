# Test a set of files, out.1 .. out.n
# Are they sorted?

# First, merge them together using the "paste" utility;
# then run this script on the combined file


  BEGIN       {   error = 0   }
  

assign(NR,10) {
                  for(i=1 ; i < NR ; i++) {
                    if ($(i+1) > $i) {
                      print "Not sorted at line " NR
                      error++
                    } 
                  }
               }
               
    END        { 
                  error = reduce(sum(error)) 
                  if(error) exit 1 
                  else if(!RANK) print "OK"
               }

