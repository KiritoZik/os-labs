#include "batcher.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int parse_positive_int(const char *str, int *value) {
    char *end;

    errno = 0;
    long number = strtol(str, &end, 10);

    if (errno != 0 || str == end || *end != '\0' ||
        number <= 0 || number > INT_MAX) {
        return 1;
    }

    *value = (int)number;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5 || strcmp(argv[1], "-n") != 0 ||
        strcmp(argv[3], "-t") != 0) {
        fprintf(stderr, "Usage: %s -n SIZE -t THREADS\n", argv[0]);
        return 1;
    }

    int n;
    int threads;

    if (parse_positive_int(argv[2], &n) != 0 || parse_positive_int(argv[4], &threads) != 0) {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    int network_size = 1;

    while (network_size < n) {
        if (network_size > INT_MAX / 2) {
            fprintf(stderr, "Array size is too large\n");
            return 1;
        }

        network_size *= 2;
    }

    int* arr = malloc((size_t)n * sizeof(*arr));

    if (arr == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    srand(42);

    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 200001 - 100000;
    }

    if (n < 30) {
        printf("arr: ");
        for(int i = 0; i < n; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    }

    struct timespec start;
    struct timespec end;

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime");
        free(arr);
        return 1;
    }

    int result = parallel_sort(arr, 0, network_size, n, threads);

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("clock_gettime");
        free(arr);
        return 1;
    }

    if (result != 0) {
        fprintf(stderr, "Sorting failed\n");
        free(arr);
        return 1;
    }

    double elapsed = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;

    for (int i = 1; i < n; i++) {
        if (arr[i - 1] > arr[i]) {
            fprintf(stderr, "Sorting is incorrect!\n");
            free(arr);
            return 1;
        }
    }

    printf("Elements: %d\n", n);
    printf("Network size: %d\n", network_size);
    printf("Threads: %d\n", threads);
    printf("Time: %.6f seconds\n", elapsed);
    printf("Sorting: OK\n");

    if (n < 30) {
        printf("Sorted arr: ");
        for(int i = 0; i < n; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    }

    free(arr);

    return 0;
}