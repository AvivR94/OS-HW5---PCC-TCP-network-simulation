
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

unsigned int port_in_use;
unsigned int printables[95];
int curr_connections; 
int finish_and_terminate; 

curr_connections = 0; /* 0 means no connections, 1 means connected to client/s */
finish_and_terminate = 0; /* 0 means go on, 1 means do operation */
/* counts the printable chars of the message received from client */
int charsCounter(char * message, int message_len){
    int cnt;
    int i;

    cnt = 0;
    for (i = 0; i < message_len; i++){
        if (message[i] >= 32 && message[i] <= 126){
            cnt++;
            printables[msg[i]-32]++;
        }
    }
    return cnt;
}

/* prints the requested output and then closes the program */
void terminateServer(){
    int i;

    for (i = 0; i < 95; i++)
        printf("char '%c' : %u times\n", i + 32, printables[i]);
    exit(0);
}

/* if a signal is invoked */
void signal_handler_func(){
    if (curr_connections < 1)
        terminateServer();
    else
        finish_and_terminate = 1;
}

/* writing the output to the client */
int outputToClient(int fconnection, char* send_to_client_buffer){
    int all_output
    int curr_output;
    int buffer_size;

    all_output = 0;
    buffer_size = sizeof(send_to_client_buffer);
    while (all_output < buffer_size){
        curr_output = write(fconnection, send_to_client_buffer + all_output, buffer_size);
        all_output += curr_output;
        if (curr_output < 0){
            if (errno == ETIMEDOUT && errno == ECONNRESET && errno == EPIPE) 
                return 1; /* invokes continue */
            else
                return -1; /* invokes exiting the program */
        }
    }
    return 0;
}

int inputFromClient(int fconnection, char* message_buff, int buff_size){
    int curr_input;
    int all_input;
    int buff_size;

    all_input = 0;
    buff_size = sizeof(message_buff);
    while (buff_size > 0){
        curr_input = read(fconnection, message_buff + all_input, buff_size);
        all_input += curr_input;
        if (curr_input < 0){
            if (errno == ETIMEDOUT && errno == ECONNRESET && errno == EPIPE)
                return 1; /* invokes continue */                                   
            else
                return -1; /* invokes exiting the program */
        }
    }
    return 0;
}

/* helper func for main, used for reaction to return value of connections with client
   returns 1 if there's a need to continue in the while loop of the connection, and 0 otherwise
   exits the program if there's a fatal error */
int checkRetVal(int ret_val, int fconnection){
    if (ret_val != 0){
        perror("Failed to communicate with client");
        if (ret_val == -1)
            exit(1);
        else{
            close(fconnection);
            fconnection = -1;
            return 1; /* means invoke continue in while loop */
        }
    }
    return 0; /* means do nothing */
}

int main(int argc, char *argv[]){
    struct sockaddr_in serv_addr;
    int fconnection;
    int flisten;
    int option_val;
    int chars_counted;
    int ret_val;
    int32_t N, C_send_to_client, message_len;
    char *N_buffer, *message_buff, *send_to_client_buffer;

    if (argc != 2){
        perror("Incorrect number of arguments");
        exit(1);
    }
    port_in_use = atoi(argv[1]);

    /* signal handler set-up */
    struct sigaction sigint;
    memset(&sigint, 0, sizeof(sigint));
    sigint.sa_handler = &signal_handler_func;
    sigint.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sigint, NULL) != 0){
        perror("Creation of signal handler failed");
        exit(1);
    }

    /* socket set-up */
    socklen_t addrsize = sizeof(struct sockaddr_in);
    flisten = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(flisten, SOL_SOCKET, SO_REUSEADDR, &option_val, sizeof(int)) < 0){
        perror("Setting up the socket failed");
        exit(1);
    }

    memset(&serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port_in_use = htons(port_in_use);

    if (bind(flisten, (struct sockaddr*) &serv_addr, addrsize) != 0){
        perror("Binding the socket failed");
        exit(1);
    }

    if (listen(flisten, 10) != 0){
        perror("Listen to connections failed");
        exit(1);
    }

    fconnection = -1;
    /* acceptance and action as long as program is running */
    while (1){
        if (finish_and_terminate == 1)
            terminateServer();

        fconnection = accept(flisten, NULL, NULL);
        if (fconnection < 0){
            perror("Accept connection failed");
            if (errno == ETIMEDOUT && errno == ECONNRESET && errno == EPIPE)
                continue;
            else
                exit(1);
        }
        curr_connections = 1; /* server is connected to a client */
        N_buffer = (char*)&N;
        ret_val = inputFromClient(fconnection, N_buffer, 4); /* reading message len (N) from client */
        if (checkRetVal(ret_val, fconnection) == 1)
            continue;
        message_len = ntohl(N);

        /* allocation for message buffer up to 1MB */
        message_buff = (char*) calloc(1000000, sizeof(char));
        ret_val = inputFromClient(fconnection, message_buff, message_len); /* reading message from client */
        if (checkRetVal(ret_val, fconnection) == 1)
            continue;
        
        chars_counted = charsCounter(message_buff, message_len);
        C_send_to_client = htonl(chars_counted); 
        send_to_client_buffer = (char*)&C_send_to_client; /* C is 32-bit int, so garuanteed less than 1MB */
        ret_val = outputToClient(fconnection, send_to_client_buffer); /* sending the counted printables number (C) to client */
        if (checkRetVal(ret_val, fconnection) == 1)
            continue;
        close(fconnection); /* finished with the client, may move on to the next one */ 
        fconnection = -1;
        curr_connections = 0;
    }
    return 0;
}