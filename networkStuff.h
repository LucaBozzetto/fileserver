#ifndef _NETWORKSTUFF
#define _NETWORKSTUFF

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <signal.h>

typedef enum resp{OK, HEADER_ERROR, HEADER_RENEGOTIATE, MESSAGE_INCOMPLETE, ABORT_OPERATION, INVALID_RESPONSE=-1} Response;

void *get_in_addr(struct sockaddr *sa);

Response strToResponse(char *s);

#endif
