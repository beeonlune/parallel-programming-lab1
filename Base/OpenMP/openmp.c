#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

void verifyMemoryAllocation(void *pointer) {
    if (pointer == NULL) {
        fprintf(stderr, "Memory allocation failed. Terminating program...\n");
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
    verifyMemoryAllocation(matrix);

    for (int i = 0; i < rows; ++i) {
        matrix[i] = (double *)calloc(cols, sizeof(double));
        verifyMemoryAllocation(matrix[i]);
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            matrix[r][c] = 10 * r + c;
        }
    }

    double t_start = omp_get_wtime();

    #pragma omp parallel for collapse(2)
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            matrix[r][c] = sin(2 * matrix[r][c]);
        }
    }

    double t_end = omp_get_wtime();

#ifndef NO_OUTPUT
    FILE *output = fopen("output_results.txt", "w");
    if (output == NULL) {
        fprintf(stderr, "Failed to open output file.\n");
        return EXIT_FAILURE;
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            fprintf(output, "%f ", matrix[r][c]);
        }
        fprintf(output, "\n");
    }

    fclose(output);
#endif

    printf("Execution time: %lf seconds\n", t_end - t_start);

    for (int i = 0; i < rows; ++i) {
        free(matrix[i]);
    }
    free(matrix);

    return EXIT_SUCCESS;
}
