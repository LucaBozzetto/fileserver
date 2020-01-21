#include "networkStuff.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

Response strToResponse(char *s){
    if (!s){
        return INVALID_RESPONSE;
    }

	int num = atoi(s);
    
	if ( num < 0 || num > 4)
		return INVALID_RESPONSE;
	
	return num;
}

