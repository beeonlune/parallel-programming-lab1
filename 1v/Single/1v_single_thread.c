#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void handleAllocationError(void* ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Memory allocation failed. Exiting...\n");
        abort();
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <rows> <columns>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const int ISIZE = atoi(argv[1]);
    const int JSIZE = atoi(argv[2]);

    double **a = (double **)calloc(ISIZE, sizeof(double*));
    handleAllocationError(a);

    for (int i = 0; i < ISIZE; ++i) {
        a[i] = (double *)calloc(JSIZE, sizeof(double));
        handleAllocationError(a[i]);
    }

    // Инициализация массива
    for (int i = 0; i < ISIZE; i++) {
        for (int j = 0; j < JSIZE; j++) {
            a[i][j] = 10 * i + j;
        }
    }

    double start = omp_get_wtime();

    // Вычисление: a[i][j] = sin(5 * a[i-2][j+3])
    for (int i = 2; i < ISIZE; i++) {
        for (int j = 0; j < JSIZE - 3; j++) {
            a[i][j] = sin(5 * a[i - 2][j + 3]);
        }
    }

    double stop = omp_get_wtime();
    printf("Time spent: %lf sec\n", stop - start);

    FILE *ff = fopen("result.txt", "w");
    if (ff == NULL) {
        fprintf(stderr, "Failed to open file for writing. Exiting...\n");
        for (int i = 0; i < ISIZE; ++i) {
            free(a[i]);
        }
        free(a);
        return EXIT_FAILURE;
    }

    // Запись в файл
    for (int i = 0; i < ISIZE; i++) {
        for (int j = 0; j < JSIZE; j++) {
            fprintf(ff, "%f ", a[i][j]);
        }
        fprintf(ff, "\n");
    }
    fclose(ff);

    for (int i = 0; i < ISIZE; ++i) {
        free(a[i]);
    }
    free(a);

    return EXIT_SUCCESS;
}
