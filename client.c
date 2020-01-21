#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <math.h>
#include "networkStuff.h"

#define MAXRCVLEN 8000 //this is only used when receiving protocol messages from the server.
#define MAX_FILEPATH_LENGTH 20
#define CHUNK_SIZE sizeof(unsigned long long) // #define CHUNK_SIZE 18446744073709551615 //Unsigned long long %llu *byte int maximum chunksize
#define SERVERPORT "12004"
#define MAXTENTATIVES 3

unsigned long long chunkSize = CHUNK_SIZE;

bool sendFile(FILE *fp, int sockfd, int fileLength){
	char c;
	int i=0;
	int numBytes=0;
	char send_message[chunkSize+1];
	char receive_message[MAXRCVLEN + 1];
	bool sent = false;
	int len;

	#ifdef DEBUG
	int sleepSection = fileLength/10;
	int sleepTick = 1;
	int totalSent=0;
	#endif

	while((c=fgetc(fp)) && c != EOF){
		send_message[i++] = c;
		if(i == chunkSize){
			send_message[i] = 0;

			sent = false;
			for(int j=0; j < MAXTENTATIVES && !sent; j++){
				if ((numBytes = send(sockfd, send_message, chunkSize, 0)) == -1) {
        			perror("client: send");
        			exit(1);
    			}

				len = recv(sockfd, receive_message, MAXRCVLEN, 0);
				receive_message[len] = '\0';

				if (len < 0){
					return false;
				}

				// printf("%s",send_message);
				Response msg_response = strToResponse(receive_message);
				if(msg_response == OK){
					#ifdef DEBUG
					totalSent += i;
					#endif
					i = 0;
					sent = true;
				}else if(msg_response == ABORT_OPERATION){
					return false;
				}else if(msg_response == MESSAGE_INCOMPLETE){
					// printf("Incomplete!!\n");
				}
			}

			#ifdef DEBUG
			if(totalSent >= sleepSection*sleepTick){
				sleep(2);
				sleepTick++;
			}
			#endif

			if(!sent)
				return false;
		}
	}

	if(i < chunkSize ){
		send_message[i] = 0;
		sent = false;
			for(int j=0; j < MAXTENTATIVES && !sent; j++){
				if ((numBytes = send(sockfd, send_message, strlen(send_message), 0)) == -1) {
        			perror("client: send");
        			exit(1);
    			}

				int len = recv(sockfd, receive_message, MAXRCVLEN, 0);
				receive_message[len] = '\0';

				if (len < 0)
					return false;

				// printf("%s",send_message);
				Response msg_response = strToResponse(receive_message);
				if(msg_response == OK){
					i = 0;
					sent = true;
				}else if(msg_response == ABORT_OPERATION){
					return false;
				}
			}

			if(!sent)
				return false;
	}
	
	return true;
}

int main(int argc, char *argv[]){
	char buffer[MAXRCVLEN + 1]; /* +1 so we can add null terminator */
	int rv, sockfd, numBytes, len;
	struct addrinfo hints, *servinfo, *destAddr;
	char *address = NULL;
	char *filePath = NULL;
	char *header;
	bool add,file;
	add = file = false;
	FILE *fp;
	
	address = malloc(sizeof(char) * 100);
	filePath = malloc(sizeof(char) * (MAX_FILEPATH_LENGTH+1));

	memset(address,0,100);
	memset(filePath,0,MAX_FILEPATH_LENGTH);

	if (!(argc < 1 && argc > 7)){
		for (int i=1; i<argc; i++) {
      		if (strcmp(argv[i], "-a") == 0 && argc > i+1) {
				i++;
         		sscanf(argv[i], "%s", address);
				add = true;
      		}else if (strcmp(argv[i], "-f") == 0 && argc > i+1) {
				i++;
         		sscanf(argv[i], "%s", filePath);
				file = true;
      		}else if (strcmp(argv[i], "-c") == 0 && argc > i+1) {
				i++;
         		sscanf(argv[i], "%llu", &chunkSize);
         		if(chunkSize <= 0){
         			printf("Error: invalid chunksize");
         			exit(1);
         		}
      		}
		}
	}else{
        printf("Usage: %s mst have 1,2 or 3 arguments\n", argv[0]);
		printf("\"%s -a server-IP-address\" server address\n", argv[0]);
        printf("\"%s -c chunk-size\" set the size of the messages sent between client and server\n", argv[0]);
		printf("\"%s -ffilepath\" sends file content the provided host. This argument is always REQUIRED\n", argv[0]);
        return 0;
    }

	if(!file){
		fprintf(stderr,"you must select a file\n");
		exit(EXIT_FAILURE);
	}
	if(!add){
		strcpy(address,"127.0.0.1");
	}
	// printf("%s\n",address);

	//Initialize infor for getaddr
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    //Resolve host name
    if ((rv = getaddrinfo(address, SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and make a socket
    for(destAddr = servinfo; destAddr != NULL; destAddr = destAddr->ai_next) {
        if ((sockfd = socket(destAddr->ai_family, destAddr->ai_socktype, destAddr->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        break;
    }
    
    if (destAddr == NULL) {
        fprintf(stderr, "client: failed to create socket\n");
        return 2;
    }


	fp = fopen(filePath,"rb");
	if(fp){
		unsigned long long int fileLength = 0;
		// int fileLength = 0;
		unsigned long long headerLength = 1; // may have to change to llu in order to contain fileLength and more

		//get file size
		fseek(fp, 0L, SEEK_END);
		fileLength = ftell(fp);
		rewind(fp);

		headerLength = strlen(filePath);
		headerLength += floor(log10(strlen(filePath))) + 1; //get filePath length in chars
		headerLength += floor(log10(fileLength)) + 1;
		headerLength += floor(log10(chunkSize)) + 1;
		headerLength += 5;

		header = malloc(sizeof(char) * headerLength);
		sprintf(header, "%s %lu %llu %llu\n", filePath, strlen(filePath), fileLength, chunkSize);

		connect(sockfd, destAddr->ai_addr, destAddr->ai_addrlen);
		if(sockfd < 0){
			perror("client: connect");
			exit(1);
		}

		//Send the message to the server.
    	if ((numBytes = send(sockfd, header, strlen(header), 0)) == -1) {
        	perror("client: send");
        	exit(1);
    	}


		//check for server Response
		len = recv(sockfd, buffer, MAXRCVLEN, 0);
		if(len < 0){
			perror("Client: ");
			exit(1);
		}
		buffer[len] = '\0';

		Response header_response = strToResponse(buffer);
		if(header_response == HEADER_RENEGOTIATE){
			if ((numBytes = send(sockfd, "0", strlen("0"), 0)) == -1) {
				perror("client: send");
				exit(1);
			}
			len = recv(sockfd,buffer, MAXRCVLEN, 0);
			if(len < 0){
				perror("Client:");
				exit(1);
			}
			buffer[len] = '\0';

			if(len < 0){
				perror("Header renegotiation failed\n");
				exit(1);
			}

			// printf("Renegotiating chunk size\n");
			chunkSize = atoi(buffer);
			// printf ("done, %d\n", chunkSize);

			header_response = OK;
		}

		if(header_response == OK){
			// start sending the file
			printf("Transfering the file...\n");
			// printf ("%d\n", chunkSize);
			if(!sendFile(fp,sockfd,fileLength)){
				fprintf(stderr, "Error while sending the file\n");
				fclose(fp);
				close(sockfd);
				exit(0);
			}
			printf("File transfered\n");
		}
		fclose(fp);
	}else{
		fclose(fp);
		fprintf(stderr, "Error opening the file\n");
	}
 
	close(sockfd);
	return EXIT_SUCCESS;
}
