#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int fconnection = -1; /* -1 means no connections, 1 means connected to client/s */
int finish_and_terminate = 0; /* 0 means go on, 1 means do operation */
uint32_t printables[95];

void terminateServer(){
    int i;
    
    for(i = 0; i < 95; i++){
        printf("char '%c' : %u times\n", i+32, printables[i]);
    }
    exit(0);
}

void signal_handler_func(){
    if(fconnection < 0)
        terminateServer();
    else
        finish_and_terminate = 1; /* if there's an open connection, finish the process and then terminate*/
}

/* straight to exit = 1 means terminate the program stat, else inspect the case of error */
int errorOccured(char* to_print, int straight_to_exit){
    perror(to_print);
    if (straight_to_exit == 1)
        exit(1);
    if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)
        return 1;
    else
        exit(1);
    return 0; /* shouldnt get here, just because int output func */
}

/* recieve message content from client */
uint32_t recieveContentFromClient(int message_len){
    uint32_t chars_counted;
    int recieved_input;
    char input_content_buffer[1000000]; /* up to 1MB  */
    int i; 

    recieved_input = 0;
    chars_counted = 0;
    while (message_len > 0){
        recieved_input = read(fconnection, input_content_buffer, sizeof(input_content_buffer));
        message_len -= recieved_input;
        if (recieved_input < 0){  
            if (errorOccured("Reading from client failed", 0) == 1){
                close(fconnection);
                fconnection = -1;
                return 0;
            }
        }
        /* count the printable chars to send back eventually to the client, and also keep the count overall in pritables DB*/
        for(i = 0; i < recieved_input; i++){
            if(input_content_buffer[i] >= 32 && input_content_buffer[i] <= 126){
                chars_counted++;
                printables[(input_content_buffer[i]-32)]++;
            }
        }
    }
    return chars_counted;
}

int main(int argc, char *argv[]){
    int option_val = 1;
    int flisten = -1;
    int port_in_use;
    int sent_output;
    int recieved_input;
	char* input_N_buffer;
    char* output_C_buffer;
    uint32_t file_size, chars_counted_to_send;
    struct sockaddr_in serv_addr;

    if (argc != 2)
        errorOccured("Incorrect number of arguments", 1);

    port_in_use = atoi(argv[1]);

    /* signal handler set-up */
    struct sigaction sigint;
	sigint.sa_handler = &signal_handler_func;
	sigemptyset(&sigint.sa_mask);
	sigint.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sigint, 0) != 0) 
        errorOccured("Creation of signal handler failed", 1);

    /* socket set-up */
    if ((flisten = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errorOccured("Setting up the socket failed", 1);

    if(setsockopt(flisten, SOL_SOCKET, SO_REUSEADDR, &option_val, sizeof(int)) < 0)
        errorOccured("Setting up the socket failed", 1);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(port_in_use); 

    /* bind and listen init */
    if (bind(flisten, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
        errorOccured("Binding the socket failed", 1);
        
    if (listen(flisten, 10) != 0)
        errorOccured("Listen to connections failed", 1);

    /* run as long as the server is up */
    while (1){
        if(finish_and_terminate == 1) /* signal handler invoked */
            terminateServer();

        fconnection = accept(flisten, NULL, NULL);
        if (fconnection < 0)
            errorOccured("Accept connection failed", 0);
        
        /* recieve message length (its file size) from the client */
        input_N_buffer = (char*)&file_size;
		recieved_input = read(fconnection, input_N_buffer, 4); /* less than 1MB */
	    if(recieved_input < 0){
            if (errorOccured("Reading from client failed", 0) == 1){
                close(fconnection);
                fconnection = -1;
                continue;
            }
        }

        /* recieve message content from client and get the printables count to send to the client back */
        chars_counted_to_send = htonl(recieveContentFromClient(ntohl(file_size)));
        if (fconnection == -1) /* if reading from the client failed during the helper func, continue to the next client - connection closed */
            continue;
        
        /* send to the client the number of printable chars from its message content */
        output_C_buffer =(char*)&chars_counted_to_send;
	    sent_output = write(fconnection, output_C_buffer, 4);
	    if(sent_output != 4){ 
            if (errorOccured("Sending to client failed", 0) == 1){
                close(fconnection);
                fconnection = -1;
                continue;
            }
        }

        close(fconnection);
        fconnection = -1;
    }
}