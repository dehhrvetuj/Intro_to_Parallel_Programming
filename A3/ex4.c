#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

/* Allocate an n x n matrix */
double **allocate_matrix(int n)
{
    double **A = malloc(n * sizeof(double *));

    for (int i = 0; i < n; i++)
        A[i] = malloc(n * sizeof(double));

    return A;
}


/* Free matrix */
void free_matrix(double **A, int n)
{
    for (int i = 0; i < n; i++)
        free(A[i]);

    free(A);
}


/* Initialize an upper triangular system */
void initialize(double **A, double *b, int n)
{
    /*
     * We choose the exact solution:
     *
     * x[0] = x[1] = ... = 1
     *
     * Therefore b[i] is the sum of row i.
     */

    for (int i = 0; i < n; i++) {

        b[i] = 0.0;

        for (int j = 0; j < n; j++) {

            if (j < i)
                A[i][j] = 0.0;

            else if (j == i)
                A[i][j] = 2.0;

            else
                A[i][j] = 0.000001;

            b[i] += A[i][j];
        }
    }
}


/* Row-oriented backward substitution */
void row_oriented(double **A, double *b, double *x, int n)
{
    for (int row = n - 1; row >= 0; row--) {

        double sum = 0.0;

        #pragma omp parallel for reduction(+:sum) schedule(runtime)
        for (int col = row + 1; col < n; col++) {

            sum += A[row][col] * x[col];
        }

        x[row] = (b[row] - sum) / A[row][row];
    }
}


/* Column-oriented backward substitution */
void column_oriented(double **A, double *b, double *x, int n)
{
    /* x initially contains b */
    for (int row = 0; row < n; row++)
        x[row] = b[row];

    for (int col = n - 1; col >= 0; col--) {

        x[col] /= A[col][col];

        #pragma omp parallel for schedule(runtime)
        for (int row = 0; row < col; row++) {

            x[row] -= A[row][col] * x[col];
        }
    }
}


/* Check the result */
void check_result(double *x, int n)
{
    double max_error = 0.0;

    for (int i = 0; i < n; i++) {

        double error = x[i] - 1.0;

        if (error < 0)
            error = -error;

        if (error > max_error)
            max_error = error;
    }

    printf("Maximum error: %e\n", max_error);
}


int main(int argc, char *argv[])
{
    if (argc != 3) {

        printf("Usage:\n");
        printf("  %s N row\n", argv[0]);
        printf("  %s N column\n", argv[0]);

        return 1;
    }


    int n = atoi(argv[1]);

    double **A = allocate_matrix(n);
    double *b = malloc(n * sizeof(double));
    double *x = malloc(n * sizeof(double));


    /* Initialize system */
    initialize(A, b, n);


    /* Start timing */
    double start = omp_get_wtime();


    if (strcmp(argv[2], "row") == 0) {

        row_oriented(A, b, x, n);

    }
    else if (strcmp(argv[2], "column") == 0) {

        column_oriented(A, b, x, n);

    }
    else {

        printf("Second argument must be row or column\n");
        return 1;
    }


    /* Stop timing */
    double end = omp_get_wtime();


    printf("N: %d\n", n);
    printf("Threads: %d\n", omp_get_max_threads());
    printf("Time: %lf seconds\n", end - start);

    check_result(x, n);


    free_matrix(A, n);
    free(b);
    free(x);

    return 0;
}
