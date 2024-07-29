#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int n;
pid_t *children;
pid_t parent_pid;
bool terminating = 0;
char *initial_states;

void sig_handler(int signal) {
    if (signal == SIGTERM) {
        terminating = 1;
        for (int i = 0; i < n; i++) {
            printf("[PARENT/PID=%d] Waiting for %d children to exit\n",
                   parent_pid, n - i);

            if (kill(children[i], signal) == -1) {
                fprintf(stderr, "Something went wrong with child %d: ", i);
            }

            int status;
            waitpid(children[i], &status, 0);
            printf(
                "[PARENT/PID=%d] Child with PID=%d terminated successfully "
                "with exit status code 0!\n",
                parent_pid, children[i], WEXITSTATUS(status));
        }
        printf("[PARENT/PID=%d] All children exited, terminating as well\n",
               parent_pid);
        exit(0);
    }
    for (int i = 0; i < n; i++) {
        kill(children[i], signal);
    }
}

void sigchld_handler(int signal) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (pid > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            for (int i = 0; i < n; i++) {
                if (children[i] == pid) {
                    if (!terminating) {
                        char gate_id[100];
                        sprintf(gate_id, "%d", i);
                        printf("[PARENT/PID=%d] Child %d with PID=%d exited\n",
                               parent_pid, i, pid);
                        if ((children[i] = fork()) == 0) {
                            char *args[] = {"./child", &initial_states[i],
                                            gate_id, NULL};
                            execv(args[0], args);
                        } else {
                            printf(
                                "[PARENT/PID=%d] Created new child for gate %d "
                                "(PID %d) and initial state '%c'\n",
                                parent_pid, i, children[i], initial_states[i]);
                        }
                    }
                    break;
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <gates_state>\n", "gates");
        return 1;
    }
    parent_pid = getpid();
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGCHLD, sigchld_handler);
    n = strlen(argv[1]);
    initial_states = argv[1];
    children = (pid_t *)malloc(n * sizeof(pid_t));
    for (int i = 0; i < n; i++) {
        if ((children[i] = fork()) == 0) {
            char gate_id[100];
            sprintf(gate_id, "%d", i);
            char *args[] = {"./child", &argv[1][i], &gate_id[0], NULL};
            execv(args[0], args);
        }
        printf(
            "[PARENT/PID=%d] Created child %d (PID=%d) and initial state "
            "'%c'\n",
            parent_pid, i, children[i], argv[1][i]);
    }
    while (1) {
        pause();
    }
    free(children);
    return 0;
}
