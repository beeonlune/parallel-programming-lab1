#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define NANOS_IN_SECOND 1000000

unsigned long calculateTimeDifference(struct timeval t_start, struct timeval t_end) {
    return (t_end.tv_sec - t_start.tv_sec) * NANOS_IN_SECOND + (t_end.tv_usec - t_start.tv_usec);
}

void checkMemoryAllocation(void *pointer) {
    if (pointer == NULL) {
        fprintf(stderr, "Memory allocation failed. Exiting...\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <rows> <columns>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);

    double **matrix = (double **)calloc(rows, sizeof(double *));
    checkMemoryAllocation(matrix);

    for (int row = 0; row < rows; ++row) {
        matrix[row] = (double *)calloc(cols, sizeof(double));
        checkMemoryAllocation(matrix[row]);
    }

    struct timeval t1, t2;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            matrix[row][col] = 10 * row + col;
        }
    }

    gettimeofday(&t1, NULL);
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            matrix[row][col] = sin(2 * matrix[row][col]);
        }
    }
    gettimeofday(&t2, NULL);

#ifndef NO_OUTPUT
    FILE *output_file = fopen("computed_results.txt", "w");
    if (output_file == NULL) {
        fprintf(stderr, "Failed to open the output file.\n");
        return EXIT_FAILURE;
    }

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            fprintf(output_file, "%f ", matrix[row][col]);
        }
        fprintf(output_file, "\n");
    }

    fclose(output_file);
#endif

    printf("Execution time: %lf seconds\n", (double)calculateTimeDifference(t1, t2) / NANOS_IN_SECOND);

    for (int row = 0; row < rows; ++row) {
        free(matrix[row]);
    }
    free(matrix);

    return EXIT_SUCCESS;
}
