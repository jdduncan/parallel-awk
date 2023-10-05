

BEGIN { 
   # when you reduce a string, the value reduced is its length.
   j="That night, there was a gathering by the great oak of all good creatures."
   k=reduce(or(j))
   print k
} 



