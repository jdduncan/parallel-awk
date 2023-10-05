

BEGIN       { if (!PARALLEL) { 
                  SIZE = 1 ; RANK = 0 ; 
                  print "Not parallel!" 
               } 
            }


NR % SIZE == RANK  && /googlebot/   { i++ }

END { print "Process " RANK " Googles: " i } 