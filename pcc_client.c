#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void outputToServer(int fsocket, char* send_to_server_buffer){
    int all_output;
    int curr_output;
    int buffer_size;

    all_output = 0;
    buffer_size = sizeof(send_to_server_buffer);
    while(all_output < buffer_size){
        curr_output = write(fsocket, send_to_server_buffer + all_output, buffer_size);
        all_output += curr_output;
        if(curr_output <= 0){
            perror("Failed to communicate with server");
            exit(1);
        }
    }
}

unsigned int inputFromServer(int fsocket, int input_len){
    unsigned int input_from_server;
    int all_input;
    int curr_input;

    all_input = 0;
    while(all_input < input_len){
        curr_input = read(fsocket, &input_from_server, input_len);
        all_input += curr_input;
        if(curr_input <= 0){
            perror("Failed to communicate with server");
            exit(1);
        }
    }
    return input_from_server;
}


int main(int argc, char *argv[]){
    struct in_addr IP;
    struct sockaddr_in serv_addr;
    int reading;
    unsigned int port_in_use, N_from_server;
    char *message_buff, *N_buff;
    int fsocket;
    uint32_t N_to_send, printable_chars_cnt;
    long int file_size;
    FILE *specified_file;

    if(argc != 4){
        perror("Incorrect number of arguments");
        exit(1);
    }

    port_in_use = atoi(argv[2]);

    if(inet_pton(AF_INET, argv[1], &IP) != 1){
        perror("Failed to retrieve IP");
        exit(1);
    }

    specified_file = fopen(argv[3], "rb");
    if(specified_file == NULL){
        perror("Failed to open input file");
        exit(1);
    }

    fseek(specified_file, 0, SEEK_END);
    file_size = ftell(specified_file); /* retrieve the file size */

    message_buff = calloc(1000000, sizeof(char)); /* allocated 1MB for buffer */
    if(message_buff == NULL){
        perror("Failed to allocate buffer");
        exit(1);
    }

    /* socket set-up */
    fsocket = socket(AF_INET, SOCK_STREAM, 0);
    if(fsocket < 0){
        perror("Setting up socket failed");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port_in_use = htons(port_in_use);
    serv_addr.sin_addr = IP;

    if(connect(fsocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        perror("Failed to connect to server");
        exit(1);
    }

    N_to_send = htonl(file_size);
    N_buff = (char*) &N_to_send;

    outputToServer(fsocket, N_buff); /* send message size to server prior to content */
    fseek(specified_file, 0, SEEK_SET);

    while (reading = fread(message_buff, 1, 1000000, specified_file) > 0){ /* send message content up to 1MB at once */
        outputToServer(fsocket, message_buff);
    }
    
    if(reading < 0){
        perror("Failed to read from input file");
        exit(1);
    }
    fclose(specified_file);

    N_from_server = inputFromServer(fsocket, 4); /* less than 1MB, retrieve the printable chars counter from server */
    printable_chars_cnt = ntohl(N_from_server); 
    printf("# of printable characters: %u\n", printable_chars_cnt);
    return 0;
}