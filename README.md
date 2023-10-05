# parallel-awk
Parallel Awk

Around 2002, the farm of Apache web servers at GreatSchools.net sent all their access log messages to a single log file (using
mod_log_spread, based on the Spread Toolkit). Unfortunately, the awk script that was analyzed a day's worth of log files was on a
course where it would soon take more than a day to run the analysis.

Parallel Awk is a fork of Aho/Kernighan/Wienberger's "One True Awk" source code, modified to link and run under MPI, and with a few 
small additions to the Awk language. The end result allowed a set of web servers (under light load during the nighttime hours) to be
coralled into a compute cluster that could analyze the file in parallel.

