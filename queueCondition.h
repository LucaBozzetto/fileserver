#ifndef _NETWORKSTUFF
#include #include "networkStuff.h"
#endif

#ifndef _QUEUECONDITION
#define _QUEUECONDITION


//****************************** Transfer queue stuff ******************************
typedef struct {
    char *fileName;
    pthread_t tid; //the tid associated;
	// int transferID;
} Transfer;

//Transfer node
typedef struct transfer_node {
    Transfer msg;
    struct transfer_node* next;
} TransferNode;

//Transfer queue - a singly linked list
//Remove from head, add to tail
typedef struct {
    TransferNode* head;
    TransferNode* tail;
    // int transferID;

    pthread_mutex_t mutex;
    
    //Add a condition variable
    pthread_cond_t cond;
} TransferQueue;

//Create a queue and initilize its mutex and condition variable
TransferQueue* createTransferQueue();

// add a new transfer - append it onto the queue
void enqueue(TransferQueue* q, char *fileName,  pthread_t tid);

//remove a transfer - remove it from the queue
int dequeue(TransferQueue* q, pthread_t tid);

// allocates the string with all the active transfers.
void printActiveTransfers(TransferQueue* q);

int getActiveThreads(TransferQueue* q);

#endif