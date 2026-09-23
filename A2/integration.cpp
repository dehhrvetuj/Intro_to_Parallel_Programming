#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BLOCK 0
#define DYNAMIC 1
static int METHOD = 0;

struct SharedData
{
    long long number_of_trapezoids;
    long long next_trapezoid;
    long long chunk_size;
    double width;
    pthread_mutex_t mutex;
};

struct ThreadData
{
    SharedData *shared_data;
    int thread_id;
    int number_of_threads;
    double partial_sum;
};

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

/*
 * Assign prespecified work using block distribution.
 */
void get_prespecified_work(ThreadData *data, long long *start, long long *end)
{
    int id = data->thread_id;
    int p = data->number_of_threads;
    long long n = data->shared_data->number_of_trapezoids;

    /*
     * Each thread calculates one continuous
     * group of trapezoids.
     */
    *start = n * id / p;
    *end = n * (id + 1) / p;
}

/*
 * Assign the next chunk of trapezoids.
 *
 * Return 1 if a new chunk was assigned.
 * Return 0 if no work remains.
 */
int get_next_work(SharedData *shared, long long *start, long long *end)
{
    pthread_mutex_lock(&shared->mutex);

    /*
     * Check whether all trapezoids have already
     * been assigned.
     */
    if (shared->next_trapezoid >= shared->number_of_trapezoids)
    {
        pthread_mutex_unlock(&shared->mutex);
        return 0;
    }

    *start = shared->next_trapezoid;
    *end = *start + shared->chunk_size;

    /*
     * The final chunk may be smaller than
     * the specified chunk size.
     */
    if (*end > shared->number_of_trapezoids)
    {
        *end = shared->number_of_trapezoids;
    }

    shared->next_trapezoid = *end;

    pthread_mutex_unlock(&shared->mutex);

    return 1;
}

/*
 * Entry function for each worker thread.
 */
void *do_work(void *argument)
{
    ThreadData *data = (ThreadData *)argument;
    SharedData *shared = data->shared_data;

    double total_sum = 0.0;

    long long start;
    long long end;

    if (METHOD == BLOCK)
    {
        /*
         * Block distribution assigns exactly
         * one interval to each thread.
         */
        get_prespecified_work(data, &start, &end);

        total_sum = integrate(start, end, shared->width);
    }
    else
    {
        /*
         * Dynamic distribution repeatedly assigns
         * chunks to each thread.
         */
        while (get_next_work(shared, &start, &end))
        {
            total_sum += integrate(start, end, shared->width);
        }
    }

    data->partial_sum = total_sum;

    return NULL;
}

void print_help(const char *program_name)
{
    printf("Usage: %s -t THREADS -n TRAPEZOIDS "
           "[-d block|dynamic] [-c CHUNK_SIZE]\n",
           program_name);

    printf("\nOptions:\n");
    printf("  -t THREADS      Number of threads\n");
    printf("  -n TRAPEZOIDS   Number of trapezoids\n");
    printf("  -d METHOD       Work distribution method\n");
    printf("                  block or dynamic\n");
    printf("                  Default: block\n");
    printf("  -c CHUNK_SIZE   Number of trapezoids per chunk\n");
    printf("                  Only used by dynamic scheduling\n");
    printf("                  Default: TRAPEZOIDS / THREADS\n");
    printf("  -h              Print this help message\n");
}

int main(int argc, char *argv[])
{
    int number_of_threads = 0;
    long long number_of_trapezoids = 0;

    long long chunk_size = 0;
    int chunk_size_given = 0;

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
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
        {
            number_of_threads = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
        {
            number_of_trapezoids = atoll(argv[++i]);
        }
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)
        {
            i++;

            if (strcmp(argv[i], "block") == 0)
            {
                METHOD = BLOCK;
            }
            else if (strcmp(argv[i], "dynamic") == 0)
            {
                METHOD = DYNAMIC;
            }
            else
            {
                fprintf(stderr, "Invalid distribution method: %s\n", argv[i]);
                fprintf(stderr, "The method must be block or dynamic.\n");

                return 1;
            }
        }
        else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
        {
            chunk_size = atoll(argv[++i]);
            chunk_size_given = 1;
        }
        else
        {
            fprintf(stderr, "Invalid or incomplete argument: %s\n", argv[i]);

            print_help(argv[0]);

            return 1;
        }
    }

    /*
     * Check the required input parameters.
     */
    if (number_of_threads <= 0 || number_of_trapezoids <= 0)
    {
        fprintf(stderr,
                "The number of threads and trapezoids must be positive.\n");

        print_help(argv[0]);

        return 1;
    }

    /*
     * Check a user-specified chunk size.
     */
    if (chunk_size_given && chunk_size <= 0)
    {
        fprintf(stderr, "The chunk size must be positive.\n");

        return 1;
    }

    /*
     * Calculate the default chunk size.
     */
    if (!chunk_size_given)
    {
        long long size = number_of_trapezoids / number_of_threads;

        chunk_size = size < 1 ? 1 : size;
    }

    /*
     * Allocate memory for the threads and
     * their individual result data.
     */
    pthread_t *threads = (pthread_t *)malloc(
        number_of_threads * sizeof(pthread_t));

    ThreadData *thread_data = (ThreadData *)malloc(
        number_of_threads * sizeof(ThreadData));

    if (threads == NULL || thread_data == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");

        free(threads);
        free(thread_data);

        return 1;
    }

    /*
     * Initialize shared information.
     */
    SharedData shared_data;

    shared_data.number_of_trapezoids = number_of_trapezoids;
    shared_data.next_trapezoid = 0;
    shared_data.chunk_size = chunk_size;
    shared_data.width = 1.0 / (double)number_of_trapezoids;

    int mutex_result = pthread_mutex_init(&shared_data.mutex, NULL);

    if (mutex_result != 0)
    {
        fprintf(stderr, "Failed to initialize mutex.\n");

        free(threads);
        free(thread_data);

        return 1;
    }

    /*
     * Initialize individual thread data.
     */
    for (int i = 0; i < number_of_threads; i++)
    {
        thread_data[i].shared_data = &shared_data;
        thread_data[i].thread_id = i;
        thread_data[i].number_of_threads = number_of_threads;
        thread_data[i].partial_sum = 0.0;
    }

    struct timespec start_time;
    struct timespec end_time;

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /*
     * Create worker threads.
     */
    int created_threads = 0;

    for (int i = 0; i < number_of_threads; i++)
    {
        int result = pthread_create(
            &threads[i],
            NULL,
            do_work,
            &thread_data[i]);

        if (result != 0)
        {
            fprintf(stderr, "Failed to create thread %d.\n", i);

            /*
             * Wait for threads that were successfully
             * created before releasing memory.
             */
            for (int j = 0; j < created_threads; j++)
            {
                pthread_join(threads[j], NULL);
            }

            pthread_mutex_destroy(&shared_data.mutex);

            free(threads);
            free(thread_data);

            return 1;
        }

        created_threads++;
    }

    /*
     * Wait for all worker threads.
     */
    for (int i = 0; i < number_of_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    /*
     * Add all partial results.
     */
    double integral = 0.0;

    for (int i = 0; i < number_of_threads; i++)
    {
        integral += thread_data[i].partial_sum;
    }

    double elapsed_time =
        (end_time.tv_sec - start_time.tv_sec) +
        (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    if (METHOD == BLOCK)
    {
        printf("Distribution: block\n");
    }
    else
    {
        printf("Distribution: dynamic\n");
        printf("Chunk size: %lld\n", shared_data.chunk_size);
    }

    printf("Number of threads: %d\n", number_of_threads);
    printf("Number of trapezoids: %lld\n", number_of_trapezoids);
    printf("Computed integral: %.15f\n", integral);
    printf("Runtime: %.9f seconds\n", elapsed_time);

    /*
     * Release resources.
     */
    pthread_mutex_destroy(&shared_data.mutex);

    free(threads);
    free(thread_data);

    return 0;
}
