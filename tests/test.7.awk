function sum(x) {
   return x + 1
}

END { 

   print "a " reduce(sum(sqrt(9)))   
   print "b " sqrt(reduce(sum(2)))  
   print "c " sum(sqrt(4))
   print "d " reduce(sum(sum(sqrt(4) + 1)))


}

