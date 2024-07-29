#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int gate_id;
int running_time = 0;
char state;

void print_gate_state() {
    printf("[GATE=%d/PID=%d/TIME=%ds] The gates are %s!\n", gate_id, getpid(),
           running_time, state == 't' ? "open" : "closed");
}

void sig_handler(int signal) {
    switch (signal) {
        case SIGUSR1:
            state = state == 't' ? 'f' : 't';
            break;
        case SIGUSR2:
            print_gate_state();
            break;
        case SIGTERM:
            exit(state == 't');
            break;
    }
}

void alarm_handler(int signal) {
    print_gate_state();
    running_time += 15;
    alarm(15);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Child process requires gate state as an argument\n");
        return 1;
    }

    state = argv[1][0];
    gate_id = atoi(argv[2]);

    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGTERM, sig_handler);

    signal(SIGALRM, alarm_handler);
    print_gate_state();
    running_time += 15;
    alarm(15);

    while (1) {
        pause();
    }

    return 0;
}
