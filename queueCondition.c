#include "networkStuff.h"
#include "queueCondition.h"

//Create a queue and initilize its mutex and condition variable
TransferQueue* createTransferQueue()
{
    TransferQueue* q = (TransferQueue*)malloc(sizeof(TransferQueue));
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    
    //Initialize the condition variable
    pthread_cond_init(&q->cond, NULL);
    return q;
}

// add a new transfer - append it onto the queue
void enqueue(TransferQueue* q, char *fileName,  pthread_t tid)
{
    TransferNode* node = (TransferNode*)malloc(sizeof(TransferNode));
    node->msg.tid = tid;

    node->msg.fileName = malloc(sizeof(char) * (strlen(fileName)+1));
    strcpy(node->msg.fileName,fileName);

    node->next = NULL;

    // critical section
    pthread_mutex_lock(&q->mutex);
    if (q->tail != NULL) {
        q->tail->next = node;       // append after tail
        q->tail = node;
    } else {
        q->tail = q->head = node;   // first node
    }
    //Signal the consumer thread woiting on this condition variable
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    // fprintf(stderr, "Worker enqueues the Transfer, signals cond variable, unlocks mutex, and goes to sleep\n");
    // sleep(2);

}

//remove a transfer - remove it from the queue
//I might want to change from tid search based to something more specific (tid+fileName for example)
int dequeue(TransferQueue* q, pthread_t tid){
    int success = 0;
    
    // critical section
    pthread_mutex_lock(&q->mutex);
    
    TransferNode *iter = q->head;
    TransferNode *prev = NULL;
    while(iter != NULL){
        if(iter->msg.tid == tid){

            if(iter == q->head)
                q->head = iter->next;
            if(iter == q->tail)
                q->tail = prev;

            if(prev != NULL)
                prev->next = iter->next;

            free(iter->msg.fileName);
            free(iter);
            success = 1;
            break;
        }
        prev = iter;
        iter = iter->next;
    }
    
    //Signal element removed
    pthread_cond_signal(&q->cond);
    //Release lock
    pthread_mutex_unlock(&q->mutex);

    return success;
}

// allocates the string with all the active transfers.
void printActiveTransfers(TransferQueue* q){
    
    // critical section
    pthread_mutex_lock(&q->mutex);
    
    //Wait for a signal telling us that there's something on the queue
    //If we get woken up but the queue is still null, we go back to sleep
    /* ASK IF I CAN COMMENT THIS
    while(q->head == NULL){
        fprintf(stderr,"Transfer queue is empty and printing goes to sleep (pthread_cond_wait)\n");
        pthread_cond_wait(&q->cond, &q->mutex);
        fprintf(stderr,"printActiveTransfers is woken up - someone signalled the condition variable\n");
    } */
    
    TransferNode *iter = q->head;

    if(iter == NULL){
        printf("There are not active transfers at the moment\n");
    }

    int i=1;
    while(iter != NULL){
        printf("Transfer #%d file: %s running on [%lu]\n",i,iter->msg.fileName,(unsigned long)iter->msg.tid);
        iter = iter->next;
        i++;
    }

    //Release lock
    pthread_mutex_unlock(&q->mutex);
}

int getActiveThreads(TransferQueue* q){
    
    // critical section
    pthread_mutex_lock(&q->mutex);
    
    TransferNode *iter = q->head;
    int j=0;
    while(iter != NULL){
        iter = iter->next;
        j++;
    }

    //Release lock
    pthread_mutex_unlock(&q->mutex);

    return j;
}
//*****************************************************************************

//*************************** Thread function stuff ***************************
// void* workerFunc(void* arg);

// #define NUM_WORKERS 2

//Each thread needs multiple arguments, so we create a dedicated struct
// typedef struct {
//     int workerId;
//     TransferQueue* q;
// } ThreadArgs;
//*****************************************************************************

/*
int main(void)
{
    pthread_t tid[NUM_WORKERS];
    ThreadArgs args[NUM_WORKERS];
    int i;
    TransferQueue* q = createTransferQueue();

    // create worker threads
    for (i = 0; i < NUM_WORKERS; i++) {
        args[i].workerId = i + 1;
        args[i].q = q;
        pthread_create(&tid[i], NULL, workerFunc, &args[i]);
    }
    
    for (;;) {
        Transfer msg;
        if (getTransfer(q, &msg)) {
            // fprintf(stderr,"getTransfer removes Transfer %d from worker %d\n", msg.value, msg.sender);
        }
    }
    
    // wait for worker threads to terminate
    for (i = 0; i < NUM_WORKERS; i++) {
        pthread_join(tid[i], NULL);
    }
    
    return 0;
}

void* workerFunc(void* arg)
{
    ThreadArgs* args = (ThreadArgs*)arg;
    int i;

    for (i = 0; i < 10; i++) {
        sendTransfer(args->q, args->workerId, i);
    }
    
    return NULL;
}
*/
