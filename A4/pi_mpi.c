#include <string.h>
#include <stdio.h>
#include <mpi.h>

#define MAX_NAME_SIZE 42
#define MAX_MSG_SIZE 100

double f(double x)
{
    return 4.0 / (1.0 + x * x);
}

/*
 * Calculate the integral over the trapezoids
 * in the interval [start, end).
 *
 * This function only performs numerical integration.
 * It does not distribute work between threads.
 */
double integrate(long long start, long long end, double width)
{
    double local_sum = 0.0;

    for (long long i = start; i < end; i++)
    {
        double x_left = i * width;
        double x_right = (i + 1) * width;

        double area = (f(x_left) + f(x_right)) * width / 2.0;

        local_sum += area;
    }

    return local_sum;
}

double do_work(int rank, int size, long long number_of_trapezoids, double width)
{
	long long start = number_of_trapezoids * rank / size;
	  
    long long end = number_of_trapezoids * (rank + 1) / size;
	
	return integrate(start, end, width);
}

void print_help(const char *program_name)
{
    printf("Usage: %s -n TRAPEZOIDS\n", program_name);

    printf("\nOptions:\n");
    printf("  -n TRAPEZOIDS   Number of trapezoids\n");
    printf("  -h              Print this help message\n");
}

int main(int argc, char *argv[])
{
    long long number_of_trapezoids = 0;
	double width = 0.0;
	double sum = 0.0, mysum = 0.0;

    /*
     * Read command-line arguments.
     */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            print_help(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
        {
            number_of_trapezoids = atoll(argv[++i]);
			width = 1.0 / (double)number_of_trapezoids;
        }
        else
        {
            fprintf(stderr, "Invalid or incomplete argument: %s\n", argv[i]);

            print_help(argv[0]);

            return 1;
        }
    }


    char msg[MAX_MSG_SIZE], name[MAX_NAME_SIZE];
    int rank, size, p, len;

    /* no MPI calls before this point */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
      printf("Hello from process %d of %d\n", rank, size);
      for (p = 1; p < size; p++) {
	MPI_Recv(msg, MAX_MSG_SIZE, MPI_CHAR, p, 0, MPI_COMM_WORLD,
		 MPI_STATUS_IGNORE);
	printf("%s\n", msg);
      }
    } else {
      MPI_Get_processor_name(name, &len);
      sprintf(msg, "Message from process %d at %s", rank, name);
      MPI_Send(msg, strlen(msg)+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }
	
	mysum = do_work(rank, size, number_of_trapezoids, width);
	
	MPI_Reduce(&mysum, &sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	
	if (rank == 0) {
      printf("The integration result is %f\n", sum);
	｝

	
    MPI_Finalize();
	
	
    /* no MPI calls after this point */
    return 0;
}
