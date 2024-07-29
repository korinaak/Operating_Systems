#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MODE 0644
#define OFLAG O_CREAT | O_APPEND | O_WRONLY

int update_file(char *filename, char *message) {
    int fdescriptor = open(filename, OFLAG, MODE);

    if (fdescriptor == -1) {
        fprintf(stderr, "Error opening file %s", filename);
        return -1;
    }

    /* 
    if (write(fdescriptor, message, strlen(message)) != strlen(message)) {
    	// Write the leftover bytes
    }
    */

    close(fdescriptor);
    return 0;
}

bool file_exists(char *filename) {
    struct stat buffer;
    return stat(filename, &buffer) == 0 ? true : false;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./a.out filename\n");
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[1], "--help")) {
        printf("Usage: ./a.out filename\n");
        exit(EXIT_SUCCESS);
    }

    if (file_exists(argv[1])) {
        fprintf(stderr, "Error: output.txt already exists\n");
        exit(EXIT_FAILURE);
    }

    int status;
    char child_message[100];
    char parent_message[100];
    char *filename = argv[1];

    sprintf(parent_message, "[PARENT] getpid() = %d, getppid() = %d\n",
            getpid(), getppid());

    pid_t child = fork();

    if (child < 0) {
        fprintf(stderr, "Error creating a child process\n");
        exit(EXIT_FAILURE);
    }

    if (!child) {
        sprintf(child_message, "[CHILD] getpid() = %d, getppid() = %d\n",
                getpid(), getppid());
        strcat(child_message, parent_message);

        if (update_file(filename, child_message) < 0) {
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    } else {
        wait(&status);
        exit(EXIT_SUCCESS);
    }

    exit(EXIT_SUCCESS);
}
