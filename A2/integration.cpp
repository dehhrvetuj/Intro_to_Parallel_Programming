#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct ThreadData {
    int thread_id;
    int number_of_threads;
    long long number_of_trapezoids;
    double partial_sum;
};

double f(double x)
{
    return 4.0 / (1.0 + x * x);
}

void *integrate(void *argument)
{
    ThreadData *data = (ThreadData *)argument;

    int id = data->thread_id;
    int p = data->number_of_threads;
    long long n = data->number_of_trapezoids;

    double width = 1.0 / (double)n;

    /*
     * Block distribution:
     * each thread calculates a continuous group of trapezoids.
     */
    long long start = n * id / p;
    long long end = n * (id + 1) / p;

    double local_sum = 0.0;

    for (long long i = start; i < end; i++) {
        double x_left = i * width;
        double x_right = (i + 1) * width;

        double area =
            (f(x_left) + f(x_right)) * width / 2.0;

        local_sum += area;
    }

    data->partial_sum = local_sum;

    return NULL;
}

void print_help(const char *program_name)
{
    printf("Usage: %s -t THREADS -n TRAPEZOIDS\n",
           program_name);

    printf("\nOptions:\n");
    printf("  -t THREADS      Number of threads\n");
    printf("  -n TRAPEZOIDS   Number of trapezoids\n");
    printf("  -h              Print this help message\n");
}

int main(int argc, char *argv[])
{
    int number_of_threads = 0;
    long long number_of_trapezoids = 0;

    /*
     * Read command-line arguments.
     */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-t") == 0 &&
                   i + 1 < argc) {
            number_of_threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-n") == 0 &&
                   i + 1 < argc) {
            number_of_trapezoids = atoll(argv[++i]);
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            print_help(argv[0]);
            return 1;
        }
    }

    if (number_of_threads <= 0 ||
        number_of_trapezoids <= 0) {
        fprintf(stderr,
                "The number of threads and trapezoids "
                "must be positive.\n");

        print_help(argv[0]);
        return 1;
    }

    pthread_t *threads = (pthread_t *)malloc(
        number_of_threads * sizeof(pthread_t));

    ThreadData *thread_data = (ThreadData *)malloc(
        number_of_threads * sizeof(ThreadData));

    if (threads == NULL || thread_data == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");

        free(threads);
        free(thread_data);

        return 1;
    }

    /*
     * Initialize thread arguments before timing.
     */
    for (int i = 0; i < number_of_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].number_of_threads =
            number_of_threads;
        thread_data[i].number_of_trapezoids =
            number_of_trapezoids;
        thread_data[i].partial_sum = 0.0;
    }

    struct timespec start_time;
    struct timespec end_time;

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /*
     * Create worker threads.
     */
    for (int i = 0; i < number_of_threads; i++) {
        int result = pthread_create(
            &threads[i],
            NULL,
            integrate,
            &thread_data[i]);

        if (result != 0) {
            fprintf(stderr,
                    "Failed to create thread %d.\n", i);

            free(threads);
            free(thread_data);

            return 1;
        }
    }

    /*
     * Wait for all worker threads.
     */
    for (int i = 0; i < number_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    /*
     * Combine the partial results.
     */
    double integral = 0.0;

    for (int i = 0; i < number_of_threads; i++) {
        integral += thread_data[i].partial_sum;
    }

    double elapsed_time =
        (end_time.tv_sec - start_time.tv_sec) +
        (end_time.tv_nsec - start_time.tv_nsec)
            / 1000000000.0;

    printf("Number of threads: %d\n",
           number_of_threads);

    printf("Number of trapezoids: %lld\n",
           number_of_trapezoids);

    printf("Computed integral: %.15f\n",
           integral);

    printf("Runtime: %.9f seconds\n",
           elapsed_time);

    free(threads);
    free(thread_data);

    return 0;
}
