
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

int curr_connections = 0; /* 0 means no connections, 1 means connected to client/s */
int finish_and_terminate = 0; /* 0 means go on, 1 means do operation */
unsigned int printables[95];
/* counts the printable chars of the message received from client 
int charsCounter(char* message, int chars_count){
    int cnt;
    int i;

    cnt = 0;
    for (i = 0; i < chars_count; i++){
        if (message[i] >= 32 && message[i] <= 126){
            printables[message[i]-32]++;
            cnt++;
        }
    }
    return cnt;
}
*/
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

/* writing the output to the client 
int outputToClient(int fconnection, char* send_to_client_buff, int buff_size){
    int curr_output;

    curr_output = write(fconnection, &send_to_client_buff, buff_size);
    if (curr_output < 0){
        if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE) 
            return 1; invokes continue 
        else
            return -1;  invokes exiting the program 
    }
    return 0;
}

int inputFromClient(int fconnection, char* message_buff){
    int curr_input;

    curr_input = read(fconnection, &message_buff, sizeof(message_buff));
    if (curr_input < 0){
        if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
            return 1;  invokes continue                              
        else
            return -1;  invokes exiting the program 
    }
    return 0;
}

 helper func for main, used for reaction to return value of connections with client
   returns 1 if there's a need to continue in the while loop of the connection, and 0 otherwise
   exits the program if there's a fatal error
int checkRetVal(int ret_val, int fconnection){
    if (ret_val != 0){
        perror("Failed to communicate with client");
        if (ret_val == -1)
            exit(1);
        else{
            close(fconnection);
            fconnection = -1;
            return 1; means invoke continue in while loop
        }
    }
    return 0; means do nothing 
}*/

int main(int argc, char *argv[]){
    unsigned int port_in_use;
    struct sockaddr_in serv_addr;
    int i;
    int fconnection = -1;
    int flisten = -1;
    int option_val = 1;
    int chars_counted;
    int all_input;
    int curr_input;
    int message_len;
    int reading;
    int curr_output;
    unsigned int N, C_send_to_client;
    char message_buff[1000000];
    printf("hi1");
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
    printf("hi2");
    /* socket set-up */
    if ((flisten = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Setting up the socket failed");
        exit(1);
    }
    if (setsockopt(flisten, SOL_SOCKET, SO_REUSEADDR, &option_val, sizeof(int)) < 0){
        perror("Setting up the socket failed");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port_in_use);
    printf("hi3");
    if (bind(flisten, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) != 0){
        perror("Binding the socket failed");
        exit(1);
    }
    printf("hi4");
    if (listen(flisten, 10) != 0){
        perror("Listen to connections failed");
        exit(1);
    }
    printf("hi5");
    /* acceptance and action as long as program is running */
    while (1){
        if (finish_and_terminate == 1)
            terminateServer();
        printf("hi6");
        fconnection = accept(flisten, NULL, NULL);
        if (fconnection < 0){
            perror("Accept connection failed");
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
                continue;
            else
                exit(1);
        }
        curr_connections = 1; /* server is connected to a client */

        printf("hi7");
        reading = 1;
        while(reading > 0){
            reading = read(fconnection, &N, sizeof(N)); /* reading message len (N) from client - integer 32-bit, less than 1MB */
            printf("reading is: %d",reading);}
        if (reading < 0){
            perror("Reading from client failed");
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
                continue;
            else
                exit(1);
        }
        message_len = ntohl(N);
        printf("hi8, message len is:%d\n",message_len);
        all_input = 0;
        chars_counted = 0;
        printf("chars count is: %d\n",chars_counted);
        while (all_input < message_len){ /* running until got all the message - up to 1MB a time */
            curr_input = read(fconnection, &message_buff, sizeof(message_buff));
            if (curr_input <= 0){
                perror("Reading from client failed");
                if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
                    continue;
                else
                    exit(1);
            }
            all_input += curr_input;
            for (i = 0; i < curr_input; i++){
                if (message_buff[i] >= 32 && message_buff[i] <= 126){
                    chars_counted++;
                    printables[message_buff[i]-32]++;
                }
            }
            printf("chars count is: %d\n",chars_counted);
        }
        printf("hi9");
        printf("chars count is: %d\n",chars_counted);
        C_send_to_client = htonl(chars_counted); 
        printf("C_SEND IS: %d\n",C_send_to_client);
        curr_output = write(fconnection, &C_send_to_client, sizeof(C_send_to_client)); /* sending the counted printables number (C) to client */
        if (curr_output < 0){
            perror("Reading from client failed");
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
                continue;
            else
                exit(1);
        }
        printf("hi10");
        close(fconnection); /* finished with the client, may move on to the next one */ 
        fconnection = -1;
        curr_connections = 0;
        printf("hi11");
    }
    printf("hi12");
    return 0;
}