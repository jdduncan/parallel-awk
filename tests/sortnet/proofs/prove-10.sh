PATH=../../..:$PATH
export PATH

# Test the syntax of the sorter 
awk -f ../verify-sorter.awk ../10line.sorter

# Build the input files
echo Building input files
awk -v n=10 -f ../prover.awk

# Run the sorter on them
echo Running the sorting network
mpiexec -n 10 time ../../../mpawk -f ../sorter.awk ../10line.sorter

echo Testing output

# Paste the output together
paste -d ' ' out.* > combined.out

# And Test it
mpiexec -n 2 mpawk -f ../is_sorted.awk combined.out

