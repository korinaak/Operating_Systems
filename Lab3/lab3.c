#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

bool is_integer(char *str) {
    if (*str == '\0') {
        return false;
    }

    while (*str != '\0') {
        if (!isdigit(*str)) {
            return false;
        }
        str++;
    }
    return true;
}

int main(int argc, char *argv[]) {
    bool round_robin = true;
    int total_children;

    if (argc != 2 && argc != 3) {
        printf("Usage: bin/lab3 <nChildren> [--random] [--round-robin]\n");
        exit(EXIT_FAILURE);
    }

    if (!is_integer(argv[1])) {
        printf("Usage: bin/lab3 <nChildren> [--random] [--round-robin]\n");
        exit(EXIT_FAILURE);
    }

    if (argc == 3 && (strcmp(argv[2], "--random") != 0) &&
        (strcmp(argv[2], "--round-robin") != 0)) {
        printf("Usage: bin/lab3 <nChildren> [--random] [--round-robin]\n");
        exit(EXIT_FAILURE);
    }

    if (argc == 3) {
        round_robin = strcmp(argv[2], "--random");
    }

    total_children = atoi(argv[1]);

    printf("total_children = %d\n", total_children);
    printf("round_robin = %d\n", round_robin);

    pid_t children_pid[total_children];
    int parent_to_child_fd[total_children][2];
    int child_to_parent_fd[total_children][2];

    for (int i = 0; i < total_children; i++) {
        if (pipe(parent_to_child_fd[i]) == -1) {
            fprintf(stderr,
                    "Something went wrong while creating the %d-th pipe "
                    "(parent to child).\n",
                    i);
            exit(EXIT_FAILURE);
        }

        if (pipe(child_to_parent_fd[i]) == -1) {
            fprintf(stderr,
                    "Something went wrong while creating the %d-th pipe (child "
                    "to parent).\n",
                    i);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < total_children; i++) {
        pid_t child = fork();
        if (child == -1) {
            fprintf(stderr,
                    "Something went wrong while creating the %d-th child.\n",
                    i);
            exit(EXIT_FAILURE);
        }

        if (child == 0) {
            close(child_to_parent_fd[i][0]);
            close(parent_to_child_fd[i][1]);
            for (int j = 0; j < total_children; j++) {
                if (j != i) {
                    close(parent_to_child_fd[j][0]);
                    close(parent_to_child_fd[j][1]);
                    close(child_to_parent_fd[j][0]);
                    close(child_to_parent_fd[j][1]);
                }
            }

            int value;

            while (true) {
                if (read(parent_to_child_fd[i][0], &value, sizeof(int)) == -1) {
                    fprintf(stderr,
                            "Something went wrong while reading from pipe");
                    exit(EXIT_FAILURE);
                }
                printf("[Child %d] [%d] Child received %d!\n", i, getpid(),
                       value);
                value++;
                sleep(5);
                if (write(child_to_parent_fd[i][1], &value, sizeof(int)) ==
                    -1) {
                    fprintf(stderr,
                            "Something went wrong while writing to pipe");
                    exit(EXIT_FAILURE);
                }
                printf(
                    "[Child %d] [%d] Child Finished hard work, writing "
                    "back "
                    "%d\n",
                    i, getpid(), value);
            }
            close(child_to_parent_fd[i][1]);
            close(parent_to_child_fd[i][0]);
            exit(EXIT_SUCCESS);
        }

        children_pid[i] = child;
    }

    int current_child = 0;
    int status;
    for (int i = 0; i < total_children; i++) {
        close(child_to_parent_fd[i][1]);
        close(parent_to_child_fd[i][0]);
    }

    while (true) {
        fd_set inset;
        int maxfd = STDIN_FILENO;

        FD_ZERO(&inset);
        FD_SET(STDIN_FILENO, &inset);

        for (int i = 0; i < total_children; i++) {
            FD_SET(child_to_parent_fd[i][0], &inset);
            if (maxfd < child_to_parent_fd[i][0]) {
                maxfd = child_to_parent_fd[i][0];
            }
        }

        int ready_fds = select(maxfd + 1, &inset, NULL, NULL, NULL);
        if (ready_fds <= 0) {
            fprintf(stderr, " Error while calling select\n");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &inset)) {
            char buffer[101];
            int n_read = read(STDIN_FILENO, buffer, 100);
            if (n_read == -1) {
                fprintf(stderr, "Error while calling read\n");
                exit(EXIT_FAILURE);
            }

            buffer[n_read] = '\0';

            if (n_read > 0 && buffer[n_read - 1] == '\n') {
                buffer[n_read - 1] = '\0';
            }

            if (n_read >= 1 && is_integer(buffer)) {
                current_child = round_robin ? current_child : rand();
                current_child %= total_children;
                int data = atoi(buffer);

                if (write(parent_to_child_fd[current_child][1], &data,
                          sizeof(int)) == -1) {
                    fprintf(stderr,
                            "Something went wrong while writing to pipe");
                    exit(EXIT_FAILURE);
                }
                printf("[Parent] Assigned %d to child %d\n", data,
                       current_child);

                if (round_robin) {
                    current_child++;
                }
            } else if (n_read >= 4 && strcmp(buffer, "exit") == 0) {
                for (int i = 0; i < total_children; i++) {
                    if (waitpid(children_pid[i], NULL, WNOHANG) == 0) {
                        printf("Waiting for %d\n", i);
                        if (kill(children_pid[i], SIGTERM) == -1) {
                            fprintf(stderr,
                                    "Error while terminating %d-th child "
                                    "proccess\n",
                                    i);
                            exit(EXIT_FAILURE);
                        }
                        wait(&status);
                    }
                }

                for (int i = 0; i < total_children; i++) {
                    close(parent_to_child_fd[i][1]);
                    close(child_to_parent_fd[i][0]);
                }

                printf("All children terminated!\n");
                exit(EXIT_SUCCESS);
            } else {
                printf("Type a number to send job to a child!\n");
            }
        }

        for (int i = 0; i < total_children; i++) {
            if (FD_ISSET(child_to_parent_fd[i][0], &inset)) {
                int data, n_read;

                n_read = read(child_to_parent_fd[i][0], &data, sizeof(int));
                if (n_read == -1) {
                    fprintf(
                        stderr,
                        "Error while reading data from child %d to parent\n",
                        i);
                    exit(EXIT_FAILURE);
                }

                printf("[Parent] Received result from child %d --> %d\n", i,
                       data);
            }
        }
    }
    return 0;
}