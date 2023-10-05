

BEGIN       { if (!PARALLEL) { 
                  SIZE = 1 ; RANK = 0 ; 
                  print "Not parallel!" 
               } 
            }

NR % SIZE == RANK { print "Process " RANK " gets line " NR }
