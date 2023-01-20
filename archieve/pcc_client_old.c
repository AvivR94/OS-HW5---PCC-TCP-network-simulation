#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

/*void outputToServer(int fsocket, char* send_to_server_buffer, int buff_size){
    int curr_output;

    curr_output = write(fsocket, &send_to_server_buffer, buff_size);
    if(curr_output <= 0){
        perror("Failed to communicate with server");
        exit(1);
    }
}

int inputFromServer(int fsocket){
    int curr_input;
    uint32_t input_from_server;
    int printable_chars_cnt;
    char* N_buff;

    N_buff = (char*) &input_from_server;
    curr_input = read(fsocket, N_buff, 4);
    if(curr_input <= 0){
        perror("Failed to communicate with server");
        exit(1);
    }
    printable_chars_cnt = ntohl(input_from_server); 
return printable_chars_cnt;
}*/

int main(int argc, char *argv[]){
    struct in_addr IP;
    struct sockaddr_in serv_addr;
    int reading;
    int curr_input;
    unsigned int port_in_use;
    int fsocket;
    uint32_t N_to_send;
    uint32_t N_from_server;
    int file_size;
    int printable_chars_cnt;
    FILE *specified_file;
    int curr_sent;
    char message_buff[1000000];

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
    fseek(specified_file, 0, SEEK_SET);

    /* socket set-up */
    fsocket = socket(AF_INET, SOCK_STREAM, 0);
    if(fsocket < 0){
        perror("Setting up socket failed");
        exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_in_use);
    serv_addr.sin_addr = IP;

    if(connect(fsocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        perror("Failed to connect to server");
        exit(1);
    }

    N_to_send = htonl(file_size);
    curr_sent = write(fsocket, &N_to_send, sizeof(N_to_send));
    if (curr_sent != sizeof(N_to_send)){
        perror("Failed to send file size to server");
        exit(1);
    }

    while (1){ /* send message content up to 1MB at once */

        if ((reading = fread(message_buff, 1, sizeof(message_buff), specified_file)) > 0){
            curr_sent = write(fsocket, message_buff, reading);
            if (curr_sent != reading){
                perror("Failed to send content from file to server");
                exit(1);
            }
        }
        else
            break;
        
    }

    if(reading < 0){
        perror("Failed to read from input file");
        exit(1);
    }
    fclose(specified_file);
    printf("hi9\n");
    
    curr_input = read(fsocket, &N_from_server, 4); /* less than 1MB, retrieve the printable chars counter from server */
    if (curr_input <= 0){
        perror("Failed to communicate with server");
        exit(1);
    }
    printable_chars_cnt = ntohl(N_from_server);
    printf("hi10\n");
    printf("# of printable characters: %u\n", printable_chars_cnt);
    return 0;
}