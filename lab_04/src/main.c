#include "calculator.h"

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define LINE_SIZE 4096
#define SHARED_FILE "/tmp/lab04_shared.bin"

typedef struct {
    sem_t data_ready;
    sem_t result_ready;
    int done;
    char status;
    char line[LINE_SIZE];
} SharedData;

static int wait_sem(sem_t *sem) {
    while (sem_wait(sem) == -1) {
        if (errno != EINTR) return -1;
    }
    return 0;
}

static int wait_reply(sem_t *sem, pid_t pid, int *child_reaped) {
    while (1) {
        struct timespec limit;
        if (clock_gettime(CLOCK_REALTIME, &limit) == -1) return -1;
        limit.tv_sec++;

        if (sem_timedwait(sem, &limit) == 0) return 0;
        if (errno == EINTR) continue;
        if (errno != ETIMEDOUT) return -1;

        pid_t checked = waitpid(pid, NULL, WNOHANG);
        if (checked == -1) return -1;
        if (checked == pid) {
            *child_reaped = 1;
            fprintf(stderr, "Child exited without a reply\n");
            return -1;
        }
    }
}

static int child_work(SharedData *shared, int output_fd) {
    printf("CHILD PID=%ld, PPID=%ld\n", (long)getpid(), (long)getppid());
    fflush(stdout);
    while (1) {
        if (wait_sem(&shared->data_ready) == -1) {
            perror("sem_wait");
            return 1;
        }
        if (shared->done) break;
        int value;
        shared->status = calculate_line(shared->line, &value);
        if (shared->status == 'O' && dprintf(output_fd, "%d\n", value) < 0) {
            perror("dprintf");
            shared->status = 'E';
        }
        if (sem_post(&shared->result_ready) == -1) {
            perror("sem_post");
            return 1;
        }
        if (shared->status == 'E') return 1;
    }
    return 0;
}

int main(void) {
    char *filename = NULL;
    size_t capacity = 0;
    printf("Enter output filename: ");
    fflush(stdout);
    ssize_t len = getline(&filename, &capacity, stdin);

    if (len == -1) {
        fprintf(stderr, "Cannot read filename\n");
        free(filename);
        return 1;
    }

    if (len > 0 && filename[len - 1] == '\n') filename[--len] = '\0';
    if (len == 0 || strcmp(filename, SHARED_FILE) == 0) {
        fprintf(stderr, "Invalid output filename\n");
        free(filename);
        return 1;
    }

    int output_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    free(filename);

    if (output_fd == -1) { perror("open output"); return 1; }

    int fd = open(SHARED_FILE, O_RDWR | O_CREAT | O_TRUNC, 0600);

    if (fd == -1) { perror("open shared file"); close(output_fd); return 1; }

    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        close(fd);
        unlink(SHARED_FILE);
        close(output_fd);
        return 1;
    }

    SharedData *shared = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (shared == MAP_FAILED) {
        perror("mmap");
        unlink(SHARED_FILE);
        close(output_fd);
        return 1;
    }

    shared->done = 0;
    shared->status = 'O';
    if (sem_init(&shared->data_ready, 1, 0) == -1) {
        perror("sem_init");
        munmap(shared, sizeof(*shared));
        unlink(SHARED_FILE);
        close(output_fd);
        return 1;
    }

    if (sem_init(&shared->result_ready, 1, 0) == -1) {
        perror("sem_init");
        sem_destroy(&shared->data_ready);
        munmap(shared, sizeof(*shared));
        unlink(SHARED_FILE);
        close(output_fd);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        sem_destroy(&shared->data_ready);
        sem_destroy(&shared->result_ready);
        munmap(shared, sizeof(*shared));
        unlink(SHARED_FILE);
        close(output_fd);
        return 1;
    }

    if (pid == 0) {
        int result = child_work(shared, output_fd);
        close(output_fd);
        munmap(shared, sizeof(*shared));
        _exit(result);
    }

    close(output_fd);
    printf("PARENT PID=%ld, CHILD PID=%ld\n", (long)getpid(), (long)pid);
    fflush(stdout);
    char *line = NULL;
    capacity = 0;

    int failed = 0;
    int child_reaped = 0;
    while ((len = getline(&line, &capacity, stdin)) != -1) {
        if (len >= LINE_SIZE) {
            fprintf(stderr, "Line too long\n");
            continue;
        }
        memcpy(shared->line, line, (size_t)len);
        shared->line[len] = '\0';
        if (sem_post(&shared->data_ready) == -1) {
            perror("sem_post");
            failed = 1;
            break;
        }
        if (wait_reply(&shared->result_ready, pid, &child_reaped) == -1) {
            if (!child_reaped) perror("wait_reply");
            failed = 1;
            break;
        }
        if (shared->status == 'I') {
            printf("Invalid input\n");
        } else if (shared->status == 'Z') {
            printf("Division by zero\n");
            failed = 1;
            break;
        } else if (shared->status == 'E') {
            fprintf(stderr, "Cannot write result\n");
            failed = 1;
            break;
        }
    }
    free(line);
    shared->done = 1;

    if (!child_reaped && sem_post(&shared->data_ready) == -1) {
        perror("sem_post");
        failed = 1;
    }

    if (!child_reaped) {
        pid_t waited;
        do {
            waited = waitpid(pid, NULL, 0);
        } while (waited == -1 && errno == EINTR);

        if (waited == -1) { perror("waitpid"); failed = 1; }
    }
    if (sem_destroy(&shared->data_ready) == -1) { perror("sem_destroy"); failed = 1; }
    if (sem_destroy(&shared->result_ready) == -1) { perror("sem_destroy"); failed = 1; }
    if (munmap(shared, sizeof(*shared)) == -1) { perror("munmap"); failed = 1; }
    if (unlink(SHARED_FILE) == -1) { perror("unlink"); failed = 1; }
    return failed;
}
