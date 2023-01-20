
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


unsigned int occurences[95]; // data struct to count the printable's.
unsigned port;
int open_connections = 0;
int exit_after_process = 0;


void server_shut_down() {
    int i;

    for (i = 0; i < 95; i++) {
        printf("char '%c' : %u times\n", i + 32, occurences[i]);
    }
    exit(0);
}

void sig_handler() {
    if (open_connections < 1) {
        server_shut_down();
    } else {
        exit_after_process = 1;
    }
}

int count_chars(char *msg, int N) {
    int N_to_send = 0;
    int i;

    for (i = 0; i < N; i++) {
        if (msg[i] <= 126 && msg[i] >= 32) {
            occurences[msg[i] - 32]++;
            N_to_send++;
        }
    }
    return N_to_send;
}


int send_to_client(int conn_file, char *N_send_buffer, int N_buffer_size) {
    int all_sent = 0;
    int now_sent;

    while (all_sent < N_buffer_size) {
        now_sent = write(conn_file, N_send_buffer + all_sent, N_buffer_size);
        all_sent += now_sent;

        if (now_sent < 0) {
            if (errno != ETIMEDOUT && errno != ECONNRESET &&
                errno != EPIPE) { // if error isn't one of these error's exit, else continue to next accept.
                return 1; //FATAL ERROR.
            } else {
                return 2; // NON-FATAL ERROR.
            }
        } else if (now_sent == 0) { // no more to send.
            if (all_sent != N_buffer_size) { // didn't send everything.
                return 2; // NON-FATAL ERROR.
            }
        }
    }
    return 0; // SUCCESS.
}


int get_from_client(int conn_file, char *msg_buffer, int msg_size) {
    int all_received = 0;
    int now_received = 1;

    while (now_received > 0) {
        now_received = read(conn_file, msg_buffer + all_received, msg_size - all_received);
        all_received += now_received;

        if (now_received < 0) {
            if (errno != ETIMEDOUT && errno != ECONNRESET &&
                errno != EPIPE) { // if error isn't one of these error's exit, else continue to next accept.
                return 1; //FATAL ERROR.
            } else {
                return 2; // NON-FATAL ERROR.
            }
        } else if (now_received == 0) { // no more to receive (got EOF).
            if (all_received != msg_size) {// didn't receive all the msg.
                return 2; // NON-FATAL ERROR.
            }
        }
    }
    return 0; // SUCCESS.
}


int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);

    int listen_file = -1;
    int conn_file = -1;
    int return_value = 1;
    int N_counted, response;

    int32_t N, N_send;

    char *N_buffer, *msg_buffer, *N_send_buffer;

    if (argc != 2) {
        fprintf(stderr, "%s\n", strerror(EINVAL));
        exit(1);
    }

    port = atoi(argv[1]);

    // set up signal handler:
    struct sigaction sigint;
    memset(&sigint, 0, sizeof(sigint));
    sigint.sa_handler = &sig_handler;
    sigint.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sigint, NULL) != 0) {
        fprintf(stderr, "Signal handler creation failed!");
        exit(1);
    }

    // set up socket:
    listen_file = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(listen_file, SOL_SOCKET, SO_REUSEADDR, &return_value, sizeof(int)) < 0) {
        fprintf(stderr, "Setsocket failed!");
        exit(1);
    }

    memset(&serv_addr, 0, addrsize);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    // bind the socket:
    if (bind(listen_file, (struct sockaddr *) &serv_addr, addrsize) != 0) {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        exit(1);
    }

    // start listen with queue size 10:
    if (listen(listen_file, 10) != 0) {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        exit(1);
    }

    // TCP connections accept loop:
    while (1) {
        if (exit_after_process) { // exit after done process if got sigint in mid of process.
            server_shut_down();
        }

        // accept connection
        conn_file = accept(listen_file, NULL, NULL);
        if (conn_file < 0) {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            if (errno != ETIMEDOUT && errno != ECONNRESET &&
                errno != EPIPE) { // if error isn't one of these error's exit, else continue to next accept.
                exit(1);
            } else {
                continue;
            }
        }
        open_connections = 1;

        // read the N from client and convert N to int:
        N_buffer = (char *) &N;

        response = get_from_client(conn_file, N_buffer, 4);

        if (response != 0) {
            if (response == 1) {
                fprintf(stderr, "Failed to get N from client! Fatal Error: %s\n", strerror(errno));
                exit(1);
            } else {
                fprintf(stderr, "Failed to get N from client! non-fatal Error\n");
                close(conn_file);
                conn_file = -1;
                continue;
            }
        }

        N = ntohl(N);


        // allocating memory for msg_buffer:
        msg_buffer = (char *) calloc(N, sizeof(char));

        // read msg from client:
        response = get_from_client(conn_file, msg_buffer, N);

        if (response != 0) {
            if (response == 1) {
                fprintf(stderr, "Failed to get N from client! Fatal Error: %s\n", strerror(errno));
                exit(1);
            } else {
                fprintf(stderr, "Failed to get N from client! non-fatal Error\n");
                close(conn_file);
                conn_file = -1;
                free(msg_buffer);
                continue;
            }
        }


        // count the amount of printable chars and update data struct.
        N_counted = count_chars(msg_buffer, N);
        N_send = htonl(N_counted);
        N_send_buffer = (char *) &N_send;
        free(msg_buffer);


        // send the amount of printable chars in msg to client
        response = send_to_client(conn_file, N_send_buffer, sizeof(N_send));

        if (response != 0) {
            if (response == 1) {
                fprintf(stderr, "Failed to send N to client! Fatal Error: %s\n", strerror(errno));
                exit(1);
            } else {
                fprintf(stderr, "Failed to send N to client! non-fatal Error\n");
                close(conn_file);
                conn_file = -1;
                continue;
            }
        }

        // succeed to receive msg, count printable, and send printable number to client.
        // prepare for next client.
        close(conn_file);
        conn_file = -1;

        open_connections = 0;
    }
    return 0;
}