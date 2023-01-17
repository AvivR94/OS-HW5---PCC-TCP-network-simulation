#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void send_to_server(uint32_t buffer_size, int socket_file, char* out_buffer){
    int all_sent = 0;
    int now_sent;

    while(all_sent < buffer_size){
        now_sent = write(socket_file, out_buffer + all_sent, buffer_size - all_sent);
        all_sent += now_sent;

        if(now_sent <= 0){
            fprintf(stderr, "Failed to write data to server!\n");
            exit(1);
        }
    }
}

unsigned int get_from_server(int response_len, int socket_file){
    unsigned int response;

    int all_received = 0;
    int now_received;

    while(all_received < response_len){
        now_received = read(socket_file, &response, response_len - all_received);
        all_received += now_received;

        if(now_received <= 0){
            fprintf(stderr, "Failed to get N to server!\n");
            exit(1);
        }
    }
    return response;
}


int main(int argc, char *argv[]){
    struct in_addr IP;
    struct sockaddr_in serv_addr;
    unsigned int port;
    uint32_t N, N_to_send, server_response;
    char *out_buffer, *N_buff;
    int socket_file = -1;
    unsigned int N_from_server;


    FILE *input_file;

    if(argc != 4){
        fprintf(stderr, "%s\n", strerror(EINVAL));
        exit(1);
    }

    port = atoi(argv[2]);

    if(inet_pton(AF_INET, argv[1], &IP) != 1){
        perror("Failed to get IP!");
        exit(1);
    }

    // open file
    input_file = fopen(argv[3], "rb");
    if(input_file == NULL){
        perror("Can't open input file");
        exit(1);
    }

    // get file size in bytes:
    fseek(input_file, 0, SEEK_END);
    N = ftell(input_file);

    // setting send buffer:
    out_buffer = calloc(1000000, sizeof(char));
    if(out_buffer == NULL){
        perror("Failed to malloc!");
        exit(1);
    }

    // read from file and close file:
    fseek(input_file, 0, SEEK_SET);

    // ?
   if(fread(out_buffer, 1, 1000000, input_file) != 0){
        fprintf(stderr, "Failed to read from file!\n");
        exit(1);
    }
    // --------

    fclose(input_file);

    // set up socket:
    socket_file = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_file < 0){
        perror("Failed to creat socket!");
        exit(1);
    }


    // set up server and connect:
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = IP;

    if(connect(socket_file, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        perror("Failed to connect!");
        exit(1);
    }

    // write N to server:
    N_to_send = htonl(N);
    N_buff = (char *) &N_to_send;

    send_to_server(4, socket_file, N_buff);

    // write data to server:
    send_to_server(N, socket_file, out_buffer);

    // get response from server:
    N_from_server = get_from_server(4, socket_file);

    server_response = ntohl(N_from_server);
    if(server_response < 0){
        perror("Failed to nthol!");
        exit(1);
    }

    printf("# of printable characters: %u\n", server_response);
    free(out_buffer);
    return 0;
}