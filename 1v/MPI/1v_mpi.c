#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define ISIZE 1000
#define JSIZE 1000

int main(int argc, char **argv) {
    double a[ISIZE][JSIZE];
    int i, j, rank, size, rows, start, end;
    FILE *ff;

    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Определяем количество строк, обрабатываемых каждым процессом
    rows = ISIZE / size;
    start = rank * rows;
    end = start + rows;

    // Инициализация массива на процессе 0
    if (rank == 0) {
        for (i = 0; i < ISIZE; i++) {
            for (j = 0; j < JSIZE; j++) {
                a[i][j] = 10 * i + j;
            }
        }
    }

    // Широковещательная рассылка массива `a` от процесса 0 всем остальным
    MPI_Bcast(a, ISIZE * JSIZE, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Выполнение вычислений в пределах своей части массива
    for (i = start; i < end; i++) {
        if (i >= 2) { 
            for (j = 0; j < JSIZE - 3; j++) {
                a[i][j] = sin(5 * a[i - 2][j + 3]);
            }
        }
    }

    // Сбор всех обработанных частей массива на процесс 0
    MPI_Gather(a[start], rows * JSIZE, MPI_DOUBLE,
               a[start], rows * JSIZE, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Процесс 0 записывает результат в файл
    if (rank == 0) {
        ff = fopen("1v_mpi_result.txt", "w");
        if (ff == NULL) {
            fprintf(stderr, "Error: Unable to open file for writing.\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        for (i = 0; i < ISIZE; i++) {
            for (j = 0; j < JSIZE; j++) {
                fprintf(ff, "%f ", a[i][j]);
            }
            fprintf(ff, "\n");
        }
        fclose(ff);
    }

    MPI_Finalize();

    return 0;
}
