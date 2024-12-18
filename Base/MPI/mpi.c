#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <mpi.h>

// Константы для удобства управления процессами
const int ROOT_PROCESS = 0;   // Главный процесс, который собирает данные
const int TURN_TAG = 100;     // Тег для передачи очереди записи в файл
const int BUFFER_READY = 200; // Тег для синхронизации процессов

// Структура для представления диапазона строк/столбцов матрицы
typedef struct {
    int start; // Начальный индекс (включительно)
    int stop;  // Конечный индекс (не включается)
} Range;

// Проверка успешности выделения памяти
// Если указатель NULL, программа завершает работу с ошибкой
void allocateMemoryCheck(void *pointer) {
    if (pointer == NULL) {
        fprintf(stderr, "Memory allocation failed. Aborting program...\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
}

// Функция для вычсления диапазона (начала и конца) строк матрицы для каждого процесса
// rank - текущий процесс
// totalProcesses - общее число процессов
// maxSize - общее количество строк матрицы
Range calculateRange(int rank, int totalProcesses, int maxSize) {
    Range segment;
    // Равномерное распределение строк между процессами
    segment.start = (maxSize / totalProcesses) * rank;
    // Последний процесс берет оставшиеся строки, если они есть
    segment.stop = (rank == totalProcesses - 1) ? maxSize : (maxSize / totalProcesses) * (rank + 1);
    return segment;
}

// Ожидание своей очереди для записи данных в файл
// rank - текущий процесс
void waitForTurnToWrite(int rank) {
    if (rank != ROOT_PROCESS) {
        int dummy;
        // Ожидание сообщения от предыдущего процесса
        MPI_Recv(&dummy, 1, MPI_INT, rank - 1, TURN_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

// Передача права на запись следующему процессу
// rank - текущий процесс
// totalProcesses - общее кол-во процессов
void allowNextProcessToWrite(int rank, int totalProcesses) {
    if (rank != totalProcesses - 1) {
        int dummy = 0;
        // Отправка сигнала следующему процессу
        MPI_Send(&dummy, 1, MPI_INT, rank + 1, TURN_TAG, MPI_COMM_WORLD);
    }
}

// Функция для записи части матрицы в файл
// rank - текущий процесс
// totalProcesses - общее количество процессов
// data - часть матрицы, принадлежащая процессу
// xRange - диапазон строк, обрабатываемый процессом
// yRange - диапазон столбцов (общий для всех процессов)
void saveMatrixToFile(int rank, int totalProcesses, double **data, Range xRange, Range yRange) {
    // Определяем режим записи: первый процесс открывает файл на запись, остальные дописывают
    const char *fileMode = (rank == ROOT_PROCESS) ? "w" : "a";
    FILE *outputFile;

    // Ожидаем своей очереди
    waitForTurnToWrite(rank);

    // Открываем файл в соответствующем режиме
    outputFile = fopen("output_results.txt", fileMode);
    if (!outputFile) {
        fprintf(stderr, "Failed to open file. Aborting...\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Записываем свою часть матрицы в файл
    for (int i = xRange.start; i < xRange.stop; ++i) {
        for (int j = yRange.start; j < yRange.stop; ++j) {
            fprintf(outputFile, "%f ", data[i - xRange.start][j - yRange.start]);
        }
        fprintf(outputFile, "\n");
    }

    fclose(outputFile);

    // Передаём право следующему процессу
    allowNextProcessToWrite(rank, totalProcesses);
}

// Основная функция вычислений
// Выполняет вычисления для своей части матрицы и записывает время выполнения
void executeComputation(int rank, int totalProcesses, double **matrix, Range xRange, Range yRange) {
    // Синхронизация всех процессов перед началом вычислений
    MPI_Barrier(MPI_COMM_WORLD);

    // Измеряем время начала вычислений (только на главном процессе)
    double startTime = 0.0, endTime = 0.0;
    if (rank == ROOT_PROCESS) {
        startTime = MPI_Wtime();
    }

    // Основной вычислительный процесс
    // Вычисляем sin(2 * value) для каждого элемента своей части матрицы
    for (int i = xRange.start; i < xRange.stop; ++i) {
        for (int j = yRange.start; j < yRange.stop; ++j) {
            matrix[i - xRange.start][j - yRange.start] = sin(2 * matrix[i - xRange.start][j - yRange.start]);
        }
    }

    // Синхронизация всех процессов после завершения вычислений
    MPI_Barrier(MPI_COMM_WORLD);

    // Измеряем время завершения вычислений (только на главном процессе)
    if (rank == ROOT_PROCESS) {
        endTime = MPI_Wtime();
        printf("Total computation time: %lf seconds\n", endTime - startTime);
    }

#ifndef DISABLE_OUTPUT
    // Сохраняем результаты в файл
    saveMatrixToFile(rank, totalProcesses, matrix, xRange, yRange);
#endif
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    // Проверяем корректность аргументов командной строки
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <rows> <columns>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Размеры матрицы
    int rows = atoi(argv[1]); // Количество строк
    int cols = atoi(argv[2]); // Количество столбцов

    // Определяем номер текущего процесса и общее количество процессов
    int rank, totalProcesses;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);

    // Рассчитываем диапазон строк для текущего процесса
    Range xRange = calculateRange(rank, totalProcesses, rows);
    // Диапазон столбцов остаётся общим для всех процессов
    Range yRange = {0, cols};

    // Выделяем память для части матрицы
    double **matrix = (double **)calloc(xRange.stop - xRange.start, sizeof(double *));
    allocateMemoryCheck(matrix);
    for (int i = 0; i < xRange.stop - xRange.start; ++i) {
        matrix[i] = (double *)calloc(cols, sizeof(double));
        allocateMemoryCheck(matrix[i]);
    }

    // Инициализируем матрицу начальными значениями
    for (int i = xRange.start; i < xRange.stop; ++i) {
        for (int j = yRange.start; j < yRange.stop; ++j) {
            matrix[i - xRange.start][j - yRange.start] = 10 * i + j;
        }
    }

    // Выполняем вычисления
    executeComputation(rank, totalProcesses, matrix, xRange, yRange);

    // Освобождаем выделенную память
    for (int i = 0; i < xRange.stop - xRange.start; ++i) {
        free(matrix[i]);
    }
    free(matrix);

    MPI_Finalize();
    return 0;
}
