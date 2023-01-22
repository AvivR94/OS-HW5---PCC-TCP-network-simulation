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

uint32_t pcc_total[95];
struct sockaddr_in serv_addr;
struct sigaction sigint;
int fconnection = 0; /* 0 means no connections, positive num means connected to client/s */
int finish_and_terminate = 0; /* 0 means go on, 1 means terminate the program after handling last connection */

/*----------------
  helper functions */

/* straight to exit = 1 means terminate the program stat, else inspect the case of error */
void errorOccured(char* to_print, int straight_to_exit){
    perror(to_print);
    if ((straight_to_exit == 1) || (errno != ETIMEDOUT && errno != ECONNRESET && errno != EPIPE))
        exit(1);
}

void terminateServer(){
    int i;
    
    for(i = 0; i < 95; i++){
        printf("char '%c' : %u times\n", i+32, pcc_total[i]);
    }
    exit(0);
}

void signal_handler_func(){
    if(fconnection <= 0)
        terminateServer();
    else
        finish_and_terminate = 1; /* if there's an open connection, finish the process and then terminate */
}

void signalHandlerSetUp(){
	sigint.sa_handler = &signal_handler_func;
	sigemptyset(&sigint.sa_mask);
	sigint.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sigint, 0) != 0)
        errorOccured("Creation of signal handler failed", 1);
}

void bindAndListen(int flisten){
    int bind_val;
    int listen_val;

    bind_val = bind(flisten, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (bind_val != 0)
        errorOccured("Binding the socket failed", 1);
    
    listen_val = listen(flisten, 10); /* open to listen up to 10 clients */
    if (listen_val != 0)
        errorOccured("Listen to connections failed", 1);
}

int serverSetUp(int port_in_use){
    int flisten = -1;
    int option_val = 1;

    signalHandlerSetUp();

    flisten = socket(AF_INET, SOCK_STREAM, 0);
    if (flisten < 0)
        errorOccured("Setting up the socket failed", 1);

    if(setsockopt(flisten, SOL_SOCKET, SO_REUSEADDR, &option_val, sizeof(int)) < 0)
        errorOccured("Setting up the socket failed", 1);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(port_in_use); 

    bindAndListen(flisten);

    return flisten;
}

int countPrintableChars(char* buffer, int received_input, uint32_t* pcc_temp){
    int i;
    int iter_chars_counted;

    iter_chars_counted = 0;
    for(i = 0; i < received_input; i++){
            if(buffer[i] >= 32 && buffer[i] <= 126){
                iter_chars_counted++;
                pcc_temp[(buffer[i]-32)]++; /* keep track of printable chars of the client for server's statistics */
            }
        }
    return iter_chars_counted;
}

/* receive message content from client */
uint32_t receiveContentFromClient(int message_len, uint32_t* pcc_temp){
    uint32_t chars_counted;
    int received_input;
    char input_content_buffer[1000000]; /* up to 1MB  */

    received_input = 0;
    chars_counted = 0;
    while (message_len > 0){
        received_input = read(fconnection, input_content_buffer, sizeof(input_content_buffer));
        message_len -= received_input;
        if (received_input < 0){  
            errorOccured("Reading from client failed", 0);
            close(fconnection);
            fconnection = 0;
            return 0; /* return to the while loop of the server with closed connection to go on to the next connection */
        }
        /* count the printable chars from all content to send it back eventually to the client */
        chars_counted += countPrintableChars(input_content_buffer, received_input, pcc_temp);
    }
    
    return chars_counted;
}

/* if the connection with client was successful, update global pcc_total */
void updatePCCTotal(uint32_t* pcc_temp){
    int i;

    for(i = 0; i < 95; i++)
        pcc_total[i] += pcc_temp[i]; /* keep track of global pcc_total for server statistics */
}

/* end of helper functions
   ----------------------- */

int main(int argc, char *argv[]){
    int flisten = -1;
    int port_in_use;
    int sent_output;
    int received_input;
	char* input_N_buffer;
    char* output_C_buffer;
    uint32_t chars_counted_of_client;
    uint32_t file_size;
    uint32_t* pcc_temp;

    if (argc != 2)
        errorOccured("Incorrect number of arguments", 1);

    port_in_use = atoi(argv[1]);

    flisten = serverSetUp(port_in_use);

    /* run as long as the server is up */
    while (1){
        if(finish_and_terminate == 1) /* signal handler invoked */
            terminateServer();

        fconnection = accept(flisten, NULL, NULL);
        if (fconnection <= 0){
            errorOccured("Accept connection failed", 0);
            continue; /* if the program didnt terminate, go on to the next connection */
        }
        
        /* receive message length (its file size) from the client */
        input_N_buffer = (char*)&file_size;
		received_input = read(fconnection, input_N_buffer, 4); /* less than 1MB */
	    if(received_input < 0){
            errorOccured("Reading from client failed", 0);
            close(fconnection);
            fconnection = 0;
            continue;
        }
        pcc_temp = (uint32_t*) calloc(95, sizeof(uint32_t));
        /* receive message content from client and get the printable chars count to send to the client back */
        chars_counted_of_client = htonl(receiveContentFromClient(ntohl(file_size), pcc_temp));
        if (fconnection <= 0){ /* if reading from the client failed during the helper func, continue to the next client - connection closed */
            free(pcc_temp);
            continue;
        }
        /* send to the client the number of printable chars from its message content */
        output_C_buffer =(char*)&chars_counted_of_client;
	    sent_output = write(fconnection, output_C_buffer, 4); /* up to 1MB */
	    if(sent_output != 4){ 
            errorOccured("Sending to client failed", 0);
            close(fconnection);
            fconnection = 0;
            free(pcc_temp);
            continue;
        }
        
        updatePCCTotal(pcc_temp); /* add the printable chars of the client to the global pcc for server's statistics */
        free(pcc_temp);
        close(fconnection);
        fconnection = 0;
    }
}