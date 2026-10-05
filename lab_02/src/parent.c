#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(void) {

    char *filename = NULL;
    size_t filename_capacity = 0;

    printf("Enter output filename: ");
    ssize_t filename_len = getline(&filename, &filename_capacity, stdin);

    if (filename_len == -1) {
        fprintf(stderr, "Failed to read filename\n");
        free(filename);
        return 1;
    }
    if (filename_len > 0 && filename[filename_len - 1] == '\n') {
        filename[filename_len - 1] = '\0';
    }

    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1) {
        perror("pipe1");
        free(filename);
        return 1;
    }

    if (pipe(pipe2) == -1) {
        perror("pipe2");
        close(pipe1[0]);
        close(pipe1[1]);
        free(filename);
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        free(filename);
        return 1;
    }

    int result = 0;

    if (pid == 0) {
        // child
        close(pipe1[1]);
        close(pipe2[0]);

        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            perror("dup2");
            close(pipe1[0]);
            close(pipe2[1]);
            free(filename);
            return 1;
        }

        close(pipe1[0]);

        char pipe2_fd_str[32];
        snprintf(pipe2_fd_str, sizeof(pipe2_fd_str), "%d", pipe2[1]);

        execl("./child", "./child", filename, pipe2_fd_str, (char*)NULL);
        perror("execl");
        close(pipe2[1]);
        free(filename);
        return 1;

    }else {
        // parent
        printf("PARENT PID=%d, CHILD PID=%d\n", getpid(), pid);
        close(pipe1[0]);
        close(pipe2[1]);

        char *buffer = NULL;
        size_t capacity = 0;
        ssize_t len;

        while ((len = getline(&buffer, &capacity, stdin)) != -1) {

            ssize_t written = write(pipe1[1],buffer, (size_t)len);

            if (written == -1) {
                perror("write string in pipe");
                free(buffer);
                free(filename);
                close(pipe1[1]);
                close(pipe2[0]);

                waitpid(pid, NULL, 0);
                return 1;
            }

            if (written != len) {
                fprintf(stderr, "Incomplete write to pipe\n");
                free(buffer);
                free(filename);
                close(pipe1[1]);
                close(pipe2[0]);

                waitpid(pid, NULL, 0);
                return 1;
            }

            char status;
            ssize_t bytes_read = read(pipe2[0], &status, 1);

            if (bytes_read == -1) {
                perror("read status");
                free(buffer);
                free(filename);
                close(pipe1[1]);
                close(pipe2[0]);

                waitpid(pid, NULL, 0);
                return 1;
            }

            if (bytes_read == 0) {
                fprintf(stderr, "Child closed pipe unexpectedly\n");
                result = 1;
                break;
            }

            if (status == 'Z') {
                printf("Division by zero.\n");
                result = 1;
                break;
            }

            if (status == 'I') {
                printf("Invalid input line\n");
                continue;
            }

            if (status == 'O') {
                printf("Command processed successfully\n");
            }


        }

        free(buffer);
        close(pipe1[1]);
        close(pipe2[0]);


        if (waitpid(pid, NULL, 0) == -1) {
            perror("waitpid");
            free(filename);
            return 1;
        }

        free(filename);
    }


    return result;
}