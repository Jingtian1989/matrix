#	matrix

Matrix is a matrix multiply demostration demostrate the effect of cache conflict on SMP.


##	Commands
	
	-m 0(cache_conflict) | 1(no_cache_conflict)
	-t thread_count
	-s matrix_size
	-d (dumping matrixs)


##	How To Avoid Cache Conflict

	Multiple threads run on different cpus(set by cpu affinity) execute matrix mul-
	tiply, when they are designed to write the adjacent memory address in matrix C, 
	the memory items may be just right mapped to the same cache line, there will be
	some waste of time to keep cache consistence inside of the cpu.

	So it's better to partition the threads to write different cache line avoiding 
	the cache consistence waste.


##	Testing Result
	
	I run the program on a AMD 4-physical core with -t 4 (4 threads each bonding to
	a different cpu), -s 1024 (1024 square matrix), the -m 1 (no_cache_conflict) ha-
	d a 50% performance promoting than -m 0 (cache conflict).


##	How To Use It

	gcc matrix.c -pthread