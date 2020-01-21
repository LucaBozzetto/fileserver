#include "networkStuff.h"
#include "queueCondition.h"

// #define MAXRCVLEN 18446744073709551615 //Unsigned long long %llu *byte int maximum chunksize
#define MAXRCVLEN 8000
#define MAX_FILEPATH_LENGTH 200
#define SERVERPORT "12004"
#define ACCEPTED_CONNECTIONS 10

// bool parseHeader(char *message, char *filePath, int filePathStrLength, int *fileLength, int *chunkSize){
// 	sscanf(message, "%s %d %d %d",filePath, filePathStrLength, fileLength, chunkSize);
// 	return true;
// }

struct addrinfo *listenAddr;
bool shutdownServer;
TransferQueue* q;
bool completeTransfers;
pthread_mutex_t flags;

void printMenu(){
	printf("List of server commands:\n");
	printf("	1 - Display active transfers.\n");
	printf("	0 - Shutdown the server.\n");
	printf("\nSelect your command:");
}

void *UIThreadCode(void *data){
	char choice;
	char term;
	int sockfd = *((int *)data);
	free(data);

	printMenu();

	do{
		// scanf("%d",&choice);
		while ((choice = getchar()) != '\n' && choice != EOF)
		// printf("\n%c\n",choice);m
		if(choice == '0'){
			pthread_mutex_lock(&flags);
			printf("\n*******************\n");
			printf("Shutting down...\n");
			printf("*******************\n\n");
			shutdownServer = true;

			pthread_mutex_lock(&q->mutex);
			TransferNode *aux = q->head;
			pthread_mutex_unlock(&q->mutex);


			if(aux != NULL){
				printf("There are pending transfers.\nDo you wish to wait until the transfers are completed?(y/n)");
				fflush(stdout);
				scanf("%c",&term);
				scanf("%c",&term);
				if(term == 'y'){
					printf("\nWaiting for %d transfers to complete:\n",getActiveThreads(q));
					printActiveTransfers(q);
					completeTransfers = true;
					pthread_mutex_unlock(&flags);
					// pthread_exit(NULL);
				}
			}
			shutdown(sockfd,SHUT_RDWR);
			close(sockfd);
			pthread_mutex_unlock(&flags);
			pthread_exit(NULL);
			// return EXIT_SUCCESS;
			// shutdown(sockfd,SHUT_RDWR);
			// close(sockfd);
			// perror("shutdown failed");
			// printf("Shutting down...\n");
		}else if(choice == '1'){
			//print active transfers
			printf("Transfers:\n");
			printActiveTransfers(q);
			printf("\nSelect your command:");
		}else{
			printf("\nInvalid command!\n\n");
			printMenu();
		}
	}while(choice != 0);
	pthread_exit(NULL);
}

void  *TransferThreadCode(void *data){
	unsigned long long len = 0;
	unsigned long long filePathStrLength, fileLength, chunkSize;

	unsigned long long numBytes = 0;
	unsigned long long totRead = 0;
	int consocket = *((int *)data);

	int destinationFile;
	FILE* filePointer;
	char *template;
	char *uploads = NULL;


	pthread_t tid = pthread_self();

	char buffer[MAXRCVLEN +1]; /* +1 so we can add null terminator */
	char *filePath = NULL;

	bool header = false;
	bool inserted = false;
	// bool terminated = false;

	filePath = malloc(sizeof(char) * (MAX_FILEPATH_LENGTH+1));

	free(data);

	while(true){// i might want to find an alternative solution!!
		if(!header)
			len = recv(consocket, buffer, MAXRCVLEN, 0);
		else{
			len = 0;
			char auxBuffer[MAXRCVLEN + 1];
			int readLen=0;
			memset(auxBuffer,0,MAXRCVLEN);
			memset(buffer,0,MAXRCVLEN);
			while(len < chunkSize && (len+totRead != fileLength)){
				readLen = recv(consocket, auxBuffer, chunkSize-len, 0);
				auxBuffer[readLen] = '\0';

				strcat(buffer,auxBuffer);
				len += readLen;

				buffer[len] = '\0';
			}
		}
		// printf("\nlen %d \n",len);
		// printf("%s\n",buffer);
		// fflush(stdout);
		buffer[len] = '\0';

		if(len > 0){
			if(!header){
				sscanf(buffer, "%s %llu %llu %llu",filePath, &filePathStrLength, &fileLength, &chunkSize);
				if(chunkSize > MAXRCVLEN){
					//renegotiate chunksize
					if ((numBytes = send(consocket, "2", strlen("2"), 0)) == -1) {
						perror("server: send");
						close(consocket);
						pthread_mutex_lock(&flags);
						if(!completeTransfers)
							dequeue(q,tid);
						pthread_mutex_unlock(&flags);
						fclose(filePointer);
						free(filePath);
						if(uploads != NULL){
							remove(uploads);
							free(uploads);
						}
						pthread_exit((void*)false);
					}

					len = recv(consocket, buffer, MAXRCVLEN, 0);
					buffer[len] = '\0';

					if(strToResponse(buffer) == OK){
						char number[999]; // CHANGE WITH CORRECT SIZE
						sprintf(number, "%d", MAXRCVLEN);
						if ((numBytes = send(consocket, number, strlen(number), 0)) == -1) {
							perror("server: send");
							close(consocket);
							pthread_mutex_lock(&flags);
							if(!completeTransfers)
								dequeue(q,tid);
							pthread_mutex_unlock(&flags);
							fclose(filePointer);
							free(filePath);
							if(uploads != NULL){
								remove(uploads);
								free(uploads);
							}
							pthread_exit((void*)false);
						}
						chunkSize = MAXRCVLEN;	
					}
				}

				if ((numBytes = send(consocket, "0", strlen("0"), 0)) == -1) {
					perror("server: send");
					close(consocket);
					pthread_mutex_lock(&flags);
					if(!completeTransfers)
						dequeue(q,tid);
					pthread_mutex_unlock(&flags);
					fclose(filePointer);
					free(filePath);
					if(uploads != NULL){
						remove(uploads);
						free(uploads);
					}
					pthread_exit((void*)false);
				}

				// printf("Started Receiving file %s of length %d with chunksize %d\n",filePath, fileLength, chunkSize);
				// printf("The file received is:\n\n");
				header = true;

				//create an unique file and open it with write privileges
				template = malloc(sizeof(char) * (filePathStrLength+1+6)); //+1 null and +6 for XXXX in template
				strcpy(template,"XXXXXX");
				strcat(template,filePath);

				uploads = malloc(sizeof(char)*(strlen(template)+9));
				sprintf(uploads,"uploads/%s",template);

				destinationFile = mkstemps(uploads,filePathStrLength);
				if(destinationFile < 0){
					perror("Impossible to create the file!");
				}
				filePointer = fdopen(destinationFile, "w");
				free(template);
			}else{
				if(!inserted){
					// printf("e mo crasho\n");
					enqueue(q, filePath, tid);
					inserted = true;
				}

				//check if the message received was complete
				/*if(len < chunkSize && (len+totRead != fileLength)){
					//send message incomplete
					// printf("%d\n",chunkSize);
					if ((numBytes = send(consocket, "3", strlen("3"), 0)) == -1) {
						perror("server: send");
						close(consocket);
						dequeue(q,tid);
						fclose(filePointer);
						if(uploads != NULL){
							remove(uploads);
						}
						pthread_exit((void*)false);
					}
					continue;
				}else{ */
				
				// printf("%s",buffer);
				fputs(buffer, filePointer);
				totRead += len;

				if ((numBytes = send(consocket, "0", strlen("0"), 0)) == -1) {
					perror("server: send");
					close(consocket);
					pthread_mutex_lock(&flags);
					if(!completeTransfers)
						dequeue(q,tid);
					pthread_mutex_unlock(&flags);
					free(filePath);
					fclose(filePointer);
					if(uploads != NULL){
						remove(uploads);
						free(uploads);
					}
					pthread_exit((void*)false);
				}



			}
		}else{
			//error
		}


		if(totRead == fileLength){
			//flush the out to show the message in case the process get terminated
			// printf("File transfer completed succesfully, waiting for another connection...\n");
			fflush(stdout);
			close(consocket);
			pthread_mutex_lock(&flags);
			if(!completeTransfers)
				dequeue(q,tid);
			pthread_mutex_unlock(&flags);
			// dequeue(q,tid);a
			fclose(filePointer);
			free(uploads);
			free(filePath);
			pthread_exit((void*)true);// RETURN TRUE
		}else if(totRead > fileLength){
			fprintf(stderr, "Error, file received was corrupted (filesize %llu larger than expected)\n", totRead);
			close(consocket);
			// exit(EXIT_FAILURE);
			pthread_mutex_lock(&flags);
			if(!completeTransfers)
				dequeue(q,tid);
			pthread_mutex_unlock(&flags);
			fclose(filePointer);
			free(filePath);
			if(uploads != NULL){
				remove(uploads);
				free(uploads);
			}
			pthread_exit((void*)false);//RETURN FALSE
		}
	}
}
 
int main(int argc, char *argv[]){
	int rv;
	int sockfd;
	struct addrinfo hints, *servinfo;
	struct sockaddr_in dest; // socket info about the machine connecting to us
	socklen_t socksize = sizeof(struct sockaddr_in);
	char *address = NULL;

	bool add,file;
	add = file = false;

	int opt=1;
	int consocket;
	pthread_t tid,tid2;

	address = malloc(sizeof(char) * 100);

	shutdownServer = false;
	completeTransfers = false;

	pthread_mutex_init(&flags, NULL);
	q = createTransferQueue();


	if (!(argc < 1 || argc > 3)){
		for (int i=1; i<argc; i++) {
      		if (strcmp(argv[i], "-a") == 0) {
         		sscanf(argv[i+1], "%s", address);
				 add = true;
      		}
		}
	}else{
        printf("Usage: %s mst have 0 or 1 argument\n", argv[0]);
        printf("\"%s -a listen-IP-address\" listen on provided address\n", argv[0]);
		printf("\"%s\" listen on localhost\n", argv[0]);
        return 0;
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
        exit(EXIT_FAILURE);
    }
    
    // loop through all the results and make a socket
    for(listenAddr = servinfo; listenAddr != NULL; listenAddr = listenAddr->ai_next) {
        if ((sockfd = socket(listenAddr->ai_family, listenAddr->ai_socktype, listenAddr->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        break;
    }	
	
	// Forcefully attaching socket to the address 
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt))){ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
    
    if (listenAddr == NULL) {
        fprintf(stderr, "server: failed to create socket\n");
		freeaddrinfo(listenAddr);
		close(sockfd);
		exit(EXIT_FAILURE);
    }

	if(bind(sockfd, listenAddr->ai_addr, listenAddr->ai_addrlen) == -1){
		fprintf(stderr, "server: failed to bind socket\n");
		freeaddrinfo(listenAddr);
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	listen(sockfd, ACCEPTED_CONNECTIONS);

	//spawn UIThread
	int *mainSockArg = malloc(sizeof(int));
	*mainSockArg = sockfd;
	pthread_create(&tid, NULL, UIThreadCode, (void*)mainSockArg);
	pthread_detach(tid);


	// printf("Server waiting for incoming connections...\n");
	pthread_mutex_lock(&flags);
	while(!shutdownServer){
		pthread_mutex_unlock(&flags);
		// Create a socket to communicate with the client that just connected
		consocket = accept(sockfd,(struct sockaddr *)&dest, &socksize);
		pthread_mutex_lock(&flags);
		if(consocket < 0 && !shutdownServer){
			fprintf(stderr,"server: Error accepting incoming connection");
			freeaddrinfo(listenAddr);
			close(sockfd);
			exit(EXIT_FAILURE);
		}else if(consocket < 0 && shutdownServer){
			break;
		}
		pthread_mutex_unlock(&flags);

		pthread_mutex_lock(&flags);	
		if(shutdownServer)
			break;
		pthread_mutex_unlock(&flags);

		//spawn transfer socket and get back to listening
		int *socketArg = malloc(sizeof(int));
		*socketArg = consocket;
		pthread_create(&tid2, NULL, TransferThreadCode, (void*)socketArg);

		pthread_mutex_lock(&flags);
	}	
	pthread_mutex_unlock(&flags);

	pthread_mutex_lock(&flags);
	if(completeTransfers){
		pthread_mutex_unlock(&flags);
		pthread_mutex_lock(&q->mutex);
		// printf("acquisito");

		TransferNode *iter = q->head;
		TransferNode *aux = NULL;
		while(iter != NULL){
			aux = iter->next;
			pthread_mutex_unlock(&q->mutex);
			pthread_join(iter->msg.tid,NULL);
			dequeue(q,iter->msg.tid);
			pthread_mutex_lock(&q->mutex);

			iter = aux;;
		}
		// printf("rilasciato\n");
		pthread_mutex_unlock(&q->mutex);
		printf("\nAll transfers completed\n");
	}
	pthread_mutex_unlock(&flags);
	
	

	

	// int k=0;
	// pthread_t res;
	// res = getActiveThreads(q,&k);
	// while(k != -1){
	// 	pthread_join(res,NULL);
	// 	k++;
	// 	res = getActiveThreads(q,&k);
	// }
	
	free(address);
	close(sockfd);
	return EXIT_SUCCESS;
}
