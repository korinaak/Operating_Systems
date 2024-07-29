#include <ctype.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define h_addr h_addr_list[0]

bool streq(char* str1, char* str2) { return !strcmp(str1, str2); }

void usage() {
    printf("usage: ./bin/lab4 [--host HOST] [--port PORT] [--debug]\n");
}

void display_sent_debug(bool debug, char* input) {
    if (debug) {
        printf("[DEBUG] sent '%s'\n", input);
    }
}

void display_read_debug(bool debug, char* response) {
    if (debug) {
        printf("[DEBUG] read '%s'\n", response);
    }
}

bool is_digit_sequence(char* str) {
    if (*str == '\0') {
        return false;
    }

    while (*str != '\0') {
        if (!isdigit(*str) && *str != '\n' && *str != ' ') {
            return false;
        }
        str++;
    }
    return true;
}

void parse_response(char* response, char* event_type, char* brightness,
                    char* temperature, char* time) {
    event_type[0] = response[0];
    event_type[1] = '\0';

    for (int i = 2; i < 5; i++) {
        brightness[i - 2] = response[i];
    }
    brightness[3] = '\0';

    for (int i = 6; i < 10; i++) {
        temperature[i - 6] = response[i];
    }
    temperature[4] = '\0';

    for (int i = 11; i < 21; i++) {
        time[i - 11] = response[i];
    }
    time[10] = '\0';
}

void display_event_type(char* event_type) {
    switch (atoi(event_type)) {
        case 0:
            printf("boot (0)\n");
            break;
        case 1:
            printf("setup (1)\n");
            break;
        case 2:
            printf("interval (2)\n");
            break;
        case 3:
            printf("button (3)\n");
            break;
        case 4:
            printf("motion (4)\n");
            break;
        default:
            printf("invalid event type (1 - 4)\n");
            exit(EXIT_FAILURE);
    }
}

void display_brightness(char* brightness) {
    printf("Light level is: %d\n", atoi(brightness));
}

void display_temperature(char* temperature) {
    printf("Temperature is: %f\n", atof(temperature) / 100.0);
}

void display_time(char* time) {
    time_t t = atoi(time);
    printf("Timestamp is: %s\n", ctime(&t));
}

void clear_response(char* response, int response_size) {
    memset(response, 0, response_size);
}

int main(int argc, char** argv) {
    char* host = "iot.dslab.pub.ds.open-cloud.xyz";
    char* port = "18080";
    bool debug = false;
    int sock_fd;
    struct sockaddr_in sin;
    struct sockaddr_in server;
    struct hostent* server_host;

    switch (argc) {
        case 1:
            break;
        case 2:
            if (streq(argv[1], "--debug")) {
                debug = true;
            } else {
                usage();
                exit(EXIT_FAILURE);
            }
            break;
        case 3:
            if (streq(argv[1], "--host")) {
                host = argv[2];
            } else if (streq(argv[1], "--port")) {
                port = argv[2];
            } else {
                usage();
                exit(EXIT_FAILURE);
            }
            break;
        case 4:
            if (streq(argv[1], "--host")) {
                host = argv[2];
            } else if (streq(argv[1], "--port")) {
                port = argv[2];
            } else {
                usage();
                exit(EXIT_FAILURE);
            }

            if (streq(argv[3], "--debug")) {
                debug = true;
            } else {
                usage();
                exit(EXIT_FAILURE);
            }
            break;
        case 5:
            if (streq(argv[1], "--host") && streq(argv[3], "--port")) {
                host = argv[2];
                port = argv[4];
            } else {
                usage();
                exit(EXIT_FAILURE);
            }
            break;
        case 6:
            if (streq(argv[1], "--host") && streq(argv[3], "--port")) {
                host = argv[2];
                port = argv[4];
            } else {
                usage();
                exit(EXIT_FAILURE);
            }

            if (streq(argv[5], "--debug")) {
                debug = true;
            } else {
                usage();
                exit(EXIT_FAILURE);
            }
            break;
        default:
            usage();
            exit(EXIT_FAILURE);
    }
    printf("host = %s\n", host);
    printf("port = %s\n", port);
    printf("debug = %d\n", debug);

    if (!is_digit_sequence(port)) {
        perror("port number must be an integer");
        exit(EXIT_FAILURE);
    }

    uint16_t port_number = atoi(port);

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock_fd, &sin, sizeof(sin)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    };

    server.sin_family = AF_INET;
    server.sin_port = htons(port_number);
    server_host = gethostbyname(host);
    bcopy((char*)server_host->h_addr, (char*)&server.sin_addr,
          server_host->h_length);

    if (connect(sock_fd, &server, sizeof(server)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    while (true) {
        fd_set inset;
        FD_ZERO(&inset);
        FD_SET(STDIN_FILENO, &inset);
        FD_SET(sock_fd, &inset);
        int maxfd = (sock_fd < STDIN_FILENO) ? STDIN_FILENO : sock_fd;

        int ready_fds = select(maxfd + 1, &inset, NULL, NULL, NULL);
        if (ready_fds <= 0) {
            fprintf(stderr, "Error while calling select\n");
        }

        if (FD_ISSET(STDIN_FILENO, &inset)) {
            char terminal_input[1000];
            int n_read;

            if ((n_read = read(STDIN_FILENO, terminal_input, 1000)) == -1) {
                fprintf(stderr, "Error while calling read\n");
                exit(EXIT_FAILURE);
            }

            terminal_input[n_read] = '\0';

            if (n_read > 0 && terminal_input[n_read - 1] == '\n') {
                terminal_input[n_read - 1] = '\0';
            }

            if (n_read >= 4 && streq(terminal_input, "exit")) {
                display_sent_debug(debug, terminal_input);
                if (shutdown(sock_fd, 2) == -1) {
                    perror("shutdown socket");
                    exit(EXIT_FAILURE);
                }
                close(sock_fd);
                exit(EXIT_SUCCESS);
            } else if (n_read >= 4 && streq(terminal_input, "help")) {
                display_sent_debug(debug, terminal_input);
                usage();
                printf("help: display help message\n");
                printf("get: request data\n");
                printf("exit: terminate the program\n");
                printf("N name surname reason: request permission to go out\n");
            } else if (n_read >= 1) {
                display_sent_debug(debug, terminal_input);
                int request;
                if ((request = write(sock_fd, terminal_input, n_read)) == -1) {
                    perror("error while sending request");
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (FD_ISSET(sock_fd, &inset)) {
            char response[1000];
            int response_bytes = read(sock_fd, response, 1000);
            response[response_bytes] = '\0';

            if (response_bytes >= 1 && is_digit_sequence(response)) {
                display_read_debug(debug, response);
                printf("---------------------------\n");
                printf("Latest event:\n");

                char event_type[100];
                char brightness[100];
                char temperature[100];
                char time[100];

                parse_response(response, event_type, brightness, temperature,
                               time);
                display_event_type(event_type);
                display_temperature(temperature);
                display_brightness(brightness);
                display_time(time);
                clear_response(response, sizeof(response));
            } else if (streq(response, "try again")) {
                display_read_debug(debug, response);
                clear_response(response, sizeof(response));
            } else if (response_bytes >= 1 &&
                       (strncmp(response, "ACK", strlen("ACK") - 1) == 0)) {
                display_read_debug(debug, response);
                printf("Response: %s\n", response);
                clear_response(response, sizeof(response));
            } else if (response_bytes >= 1) {
                display_read_debug(debug, response);
                printf("Send verification code: %s\n", response);
                clear_response(response, sizeof(response));
            }
        }
    }
    exit(EXIT_SUCCESS);
}