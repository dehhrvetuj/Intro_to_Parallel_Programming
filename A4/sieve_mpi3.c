#include <inttypes.h>
#include <math.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void mark_multiples_segmented(uint64_t k, uint64_t start, uint64_t end,
                                     int *list)
{
    uint64_t first_multiple = ((start + k - 1) / k) * k;
    if (first_multiple < k * k)
        first_multiple = k * k;

    for (uint64_t i = first_multiple; i <= end; i += k)
        list[i] = 0;
}

int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);

    int rank, size;
	uint64_t max = 0;
	
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	if (argc != 2) {
		fprintf(stderr,
				"Usage: %s N \n"
				"  Prints all primes <= N.  Example: %s 1000000\n",
				argv[0], argv[0]);
		 MPI_Abort(MPI_COMM_WORLD, 1);
	}

	char *endp;
	max = strtoull(argv[1], &endp, 10);
	if (argv[1][0] == '-'  || *endp != '\0' || max >= SIZE_MAX / sizeof(int) || max < 2) {
		fprintf(stderr, "Invalid N: %s\n", argv[1]);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	double t_start = MPI_Wtime();
	
	
    int *list = malloc(sizeof(int) * (max + 1));
    if (list == NULL) {
        perror("malloc");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    list[0] = 0;
    if (max >= 1)
        list[1] = 0;

    for (uint64_t i = 2; i <= max; i++)
        list[i] = 1;

    uint64_t sqrt_max = (uint64_t)sqrt((double)max);
	

	/* Sequential part conducted by process 0 */
    if (rank == 0) 
	{
		for (uint64_t k = 2; k * k <= sqrt_max; k++) {
			if (list[k] == 1) {
				for (uint64_t i = k * k; i <= sqrt_max; i += k)
					list[i] = 0;
			}
		}	
	}
	
	/* All processes participate in Broadcast but only process 0 provides information */
	MPI_Bcast(list, sqrt_max + 1, MPI_INT, 0, MPI_COMM_WORLD);
	
	
    uint64_t start_range = sqrt_max + 1;
    uint64_t end_range = max;
	
	uint64_t total_elements = end_range - start_range + 1;
	uint64_t lo = start_range + total_elements * rank / size;
	uint64_t hi = start_range + total_elements * (rank + 1) / size;

    if (start_range <= end_range) 
	{

		if (lo < hi) 
		{
			uint64_t end = hi - 1;
			for (uint64_t k = 2; k <= sqrt_max; k++) 
			{
				if (list[k] == 1)
					mark_multiples_segmented(k, lo, end, list);
			}
		}
		 
    }
	
	int *result = NULL;
	
	if (rank == 0) 
	{
		result = malloc(sizeof(int) * (max + 1));
		
		if (result == NULL) 
		{
			perror("malloc");
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
    }
	
	MPI_Reduce(list, result, (int)(max + 1), MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

		
	MPI_Barrier(MPI_COMM_WORLD);
	double t_end = MPI_Wtime();

    if (rank == 0) 
	{
        fprintf(stderr, "Elapsed time: %.6f seconds\n", t_end - t_start);

        for (uint64_t i = 2; i <= max; i++) 
		{
            if (list[i])
                printf("%" PRIu64 "\n", i);
        }
    }

    free(list);
	
	MPI_Finalize();
	
    return 0;
}
