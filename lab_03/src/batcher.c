#include "batcher.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
        int* arr;
        int left;
        int size;
        int n;
        int threads;
        int result;
} SortTask;

static void compare_swap(int* arr, int i, int j, int n) {
        if (j >= n) {
                return;
        }

        if (arr[i] > arr[j]) {
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
        }
}

static void odd_even_merge(int* arr, int left, int size, int step, int n) {
        int next_step = step * 2;

        if (next_step < size) {
                odd_even_merge(arr, left, size, next_step, n);
                odd_even_merge(arr, left+step, size, next_step, n);

                for (int i = left + step; i + step < left + size; i += next_step) {
                        compare_swap(arr, i, i + step, n);
                }
        } else {
                compare_swap(arr, left, left + step, n);
        }
}

static void odd_even_merge_sort(int* arr, int left, int size, int n) {
        if (size <= 1 || left >= n) {
                return;
        }

        int half = size / 2;
        odd_even_merge_sort(arr, left, half, n);
        odd_even_merge_sort(arr, left + half, half, n);

        odd_even_merge(arr, left, size, 1, n);
}

static void* worker(void *arg) {
        SortTask* task = (SortTask *)arg;

        task->result = parallel_sort(task->arr, task->left, task->size, task->n, task->threads);

        return NULL;
}


int parallel_sort(int *arr, int left, int size, int n, int threads) {
        if (size <= 1 || left >= n || n - left <= 1) {
                return 0;
        }

        if (threads <= 1) {
                odd_even_merge_sort(arr, left, size, n);
                return 0;
        }

        int half = size / 2;

        int left_threads = threads / 2;
        int right_threads = threads - left_threads;

        pthread_t thread;

        SortTask task = {.arr = arr, .left = left, .size = half, .n = n, .threads = left_threads, .result = 0};

        int result = pthread_create(&thread, NULL, worker, &task);

        if (result != 0) {
                fprintf(stderr, "pthread_create failed: %s. Sorting sequentially.\n", strerror(result));
                odd_even_merge_sort(arr, left, size, n);
                return 0;
        }

        int right_result = parallel_sort(arr, left + half, half, n, right_threads);

        result = pthread_join(thread, NULL);

        if (result != 0) {
                fprintf(stderr, "pthread_join failed: %d\n", result);
                exit(EXIT_FAILURE);
        }

        if (task.result != 0 || right_result != 0) {
                return 1;
        }

        odd_even_merge(arr, left, size, 1, n);

        return 0;
}
