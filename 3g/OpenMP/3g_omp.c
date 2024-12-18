#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

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
    double **b = (double **)calloc(ISIZE, sizeof(double*));
    handleAllocationError(a);
    handleAllocationError(b);

    for (int i = 0; i < ISIZE; ++i) {
        a[i] = (double *)calloc(JSIZE, sizeof(double));
        b[i] = (double *)calloc(JSIZE, sizeof(double));
        handleAllocationError(a[i]);
        handleAllocationError(b[i]);
    }

    // Параллельная инициализация матриц
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < ISIZE; i++) {
        for (int j = 0; j < JSIZE; j++) {
            a[i][j] = 10 * i + j;
            b[i][j] = 0;
        }
    }

    double start = omp_get_wtime();

    // Параллельное вычисление первой части: a[i][j] = sin(0.005 * a[i][j])
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < ISIZE; i++) {
        for (int j = 0; j < JSIZE; j++) {
            a[i][j] = sin(0.005 * a[i][j]);
        }
    }

    // Параллельное вычисление второй части: b[i][j] = a[i-5][j+2] * 1.5
    #pragma omp parallel for schedule(static)
    for (int i = 5; i < ISIZE; i++) {
        for (int j = 0; j < JSIZE - 2; j++) {
            b[i][j] = a[i - 5][j + 2] * 1.5;
        }
    }

    double stop = omp_get_wtime();
    printf("Time spent: %lf sec\n", stop - start);

    FILE *ff = fopen("result.txt", "w");
    if (ff == NULL) {
        fprintf(stderr, "Failed to open file for writing. Exiting...\n");
        for (int i = 0; i < ISIZE; ++i) {
            free(a[i]);
            free(b[i]);
        }
        free(a);
        free(b);
        return EXIT_FAILURE;
    }

    // Запись в файл (не распараллеливаем для упрощения синхронизации)
    for (int i = 0; i < ISIZE; i++) {
        for (int j = 0; j < JSIZE; j++) {
            fprintf(ff, "%f ", b[i][j]);
        }
        fprintf(ff, "\n");
    }
    fclose(ff);

    for (int i = 0; i < ISIZE; ++i) {
        free(a[i]);
        free(b[i]);
    }
    free(a);
    free(b);

    return EXIT_SUCCESS;
}
