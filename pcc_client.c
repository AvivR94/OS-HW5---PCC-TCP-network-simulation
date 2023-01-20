#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

int errorOccured(char* to_print){
	perror(to_print);
	exit(1);
}

void sendContentToServer(FILE* specified_file, int fconnection){
	char output_content_buffer[1000000]; /* up to 1MB */
	int reading;
	int data_sent;

	while(1){
        reading = fread(output_content_buffer, 1, sizeof(output_content_buffer), specified_file); 
        if (reading > 0){
            data_sent = write(fconnection, output_content_buffer, reading);
            if (data_sent != reading)
                errorOccured("Failed to send content from file to server");
        }
        else
            break;
    }
}

int main(int argc, char *argv[]){
	struct sockaddr_in serv_addr; 
	struct in_addr IP;
	FILE* specified_file;
	int fconnection = -1;
	char* output_N_buffer;
    char* input_N_buffer;
	int data_sent;
	int recieved_input;
    int port_in_use;
    int printable_char_cnt;
	uint32_t file_size, output_to_send, input_from_server;

	if (argc != 4)
        errorOccured("Incorrect number of arguments");

    port_in_use = atoi(argv[2]);
	if ((specified_file = fopen(argv[3],"rb")) == NULL)
        errorOccured("Failed to open input file");

	fseek(specified_file, 0, SEEK_END);
	file_size = ftell(specified_file);
	fseek(specified_file, 0, SEEK_SET);

	/* socket set-up */
	if ((fconnection = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		errorOccured("Setting up socket failed");

	/* IP set-up */
	if(inet_pton(AF_INET, argv[1], &IP) != 1){
        errorOccured("Failed to retrieve IP");
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_in_use);
    serv_addr.sin_addr = IP;

	/* init connection */
	if (connect(fconnection, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		errorOccured("Failed to connect to server");

	/* send N, the file size to the server */
	output_to_send = (htonl(file_size));
	output_N_buffer = (char*)&output_to_send;
	data_sent = write(fconnection, output_N_buffer, sizeof(output_to_send));
	if(data_sent < 0)
		errorOccured("Failed to send file size to server");
        	
	/* send the content of the file to the server, buffer is up to 1MB */
    sendContentToServer(specified_file, fconnection);
	fclose(specified_file);

	/* recieve the number of printable chars in the file content from the server */
	recieved_input = 0;
    input_N_buffer = (char*)&input_from_server;
    recieved_input = read(fconnection, input_N_buffer, 4);
    if(recieved_input < 0){
        errorOccured("Failed to communicate with server");
	}

	close(fconnection);
	printable_char_cnt = ntohl(input_from_server);
	printf("# of printable characters: %u\n", printable_char_cnt);
	exit(0);
}