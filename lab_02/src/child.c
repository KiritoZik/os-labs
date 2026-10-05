#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Expected 2 arguments: filename and pipe fd\n");
        return 1;
    }

    printf("CHILD PID=%d, PPID=%d\n", getpid(), getppid());

    char*  filename = argv[1];

    int pipe_fd = atoi(argv[2]);

    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (file_fd == -1) {
        perror("open");
        return 1;
    }

    char* buffer = NULL;
    size_t capacity = 0;

    while (getline(&buffer, &capacity, stdin) != -1) {
        char* ptr = buffer;
        char* endptr;

        long value = strtol(ptr, &endptr, 10);

        if (endptr == ptr) {
            char status = 'I';
            if (write(pipe_fd, &status, 1) != 1) {
                perror("write pipe status");
                free(buffer);
                close(file_fd);
                close(pipe_fd);
                return 1;
            }
            continue;
        }

        if (value < INT_MIN || value > INT_MAX) {
            char status = 'I';
            if (write(pipe_fd, &status, 1) != 1) {
                perror("write pipe status");
                free(buffer);
                close(file_fd);
                close(pipe_fd);
                return 1;
            }
            continue;
        }
        int result = (int)value;

        ptr = endptr;

        int invalid = 0;

        while (1) {
            long divisor_value = strtol(ptr, &endptr, 10);

            if (endptr == ptr) {
                break;
            }

            if (divisor_value < INT_MIN || divisor_value > INT_MAX) {
                char status = 'I';
                if (write(pipe_fd, &status, 1) != 1) {
                    perror("write pipe status");
                    free(buffer);
                    close(file_fd);
                    close(pipe_fd);
                    return 1;
                }
                invalid = 1;
                break;
            }

            int divisor = (int)divisor_value;

            if (divisor == 0) {
                char status = 'Z';
                if (write(pipe_fd, &status, 1) != 1) {
                    perror("write pipe status");
                    free(buffer);
                    close(file_fd);
                    close(pipe_fd);
                    return 1;
                }
                free(buffer);
                close(file_fd);
                close(pipe_fd);
                return 1;
            }

            result = result / divisor;
            ptr = endptr;

        }

        if (invalid) {
            continue;
        }

        char output[64];

        int len = snprintf(output, sizeof(output), "%d\n", result);

        if (write(file_fd, output, len) == -1) {
            perror("write");
            free(buffer);
            close(file_fd);
            close(pipe_fd);
            return 1;
        }
        char status = 'O';
        if (write(pipe_fd, &status, 1) != 1) {
            perror("write pipe status");
            free(buffer);
            close(file_fd);
            close(pipe_fd);
            return 1;
        }
    }


    free(buffer);
    close(file_fd);
    close(pipe_fd);

    return 0;
}