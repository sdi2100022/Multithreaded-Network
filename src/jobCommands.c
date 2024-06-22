/* @author : Athanasios Giavridis -- sdi2100022 */

#include "../lib/jobCommands.h"

extern Server *network; // global struct of the server 

extern int mainsockfd;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond_empty;
extern pthread_cond_t cond_full;
extern pthread_cond_t cond_concurrency;
extern pthread_cond_t cond_allInactive;

char *issueJob(char *job,int socket){

    static int job_count = 1; // count of JobIDs

    char *result = NULL;

    char *buffer = NULL;
    int buffer_size = snprintf(NULL,0,"job_%d",job_count); //get the buffer size of the JobID string
    char* jobID = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocating JobID string
    if(jobID == NULL){
        printf("Memory Error!\n");
        printf("Allocation Failed!\n");
        return result;
    }
    snprintf(jobID,buffer_size + 1,"job_%d",job_count); // get the JobID string
    jobID[buffer_size] = '\0';

    jobtuple *tuple = (jobtuple*)malloc(sizeof(jobtuple)); // allocating jobtuple
    if(tuple == NULL){
        free(jobID);
        printf("Memory Error!\n");
        printf("Allocation Failed!\n");
        return result;
    }

    jobtuple_constructor(tuple,jobID,job,socket); // allocating and initializing the tuple

    pthread_mutex_lock(&mutex); // get the mutex

    while(network->jobBuffer->size >= network->bufferSize){ // if the size of the jobBuffer exceeds the prerequisite bufferSize
        pthread_cond_wait(&cond_full,&mutex); // wait till a job is removed so there is space to insert 
    }

    enqueue(network->jobBuffer,tuple); // queue the job in the "queued" queue of the server;

    buffer_size = snprintf(NULL,0,"JOB <%s,%s> SUBMITTED\n",tuple->jobID, tuple->job);  //get the buffer size of the tuple's values
    buffer = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocating the string
    if(buffer == NULL){
        free(jobID);
        free(tuple);
        printf("Memory Error!\n");
        printf("Allocation Failed!\n");
        return result;
    }

    snprintf(buffer,buffer_size,"JOB <%s,%s> SUBMITTED\n",tuple->jobID, tuple->job);// get the string size
    int indicator = snprintf(NULL,0,"%d",buffer_size); // get the byte size of the buffer
    result = (char*)malloc(sizeof(char)*(1 + indicator + buffer_size + 1)); // allocate space for the buffer
    snprintf(result,1 + indicator + buffer_size,"%d%d%s",indicator,buffer_size,buffer); // put the message with the indicator inside the buffer
    free(buffer);
    result[1 + indicator + buffer_size] = '\0';

    job_count++; // increase the job count

    pthread_cond_signal(&cond_empty); // wake up any active workers waiting for the jobBuffer to have a job inside it
     
    pthread_mutex_unlock(&mutex); // release the mutex
    
    return result; // return the message
}

char *setConcurrency(int N){

    pthread_mutex_lock(&mutex);   
    network->concurrency = N;

    int buffer_size = snprintf(NULL,0,"CONCURRENCY SET AT %d\n",N); // get the size of the string
    char *buffer = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocate space for the buffer
    snprintf(buffer,buffer_size + 1,"CONCURRENCY SET AT %d\n",N); // put message in the buffer
    buffer[buffer_size] = '\0';

    pthread_cond_broadcast(&cond_concurrency); // wake up all worker threads that the concurrency has changed

    int indicator = snprintf(NULL,0,"%d",buffer_size); // get the size of the buffer size
    char* result = (char*)malloc(sizeof(char)*(1 + indicator + buffer_size + 1)); // allocate space for the buffer
    snprintf(result,1 + indicator + buffer_size + 1,"%d%d%s",indicator,buffer_size,buffer); // put the message with the indicator in the buffer
    free(buffer);
    result[1 + indicator + buffer_size] = '\0';

    pthread_mutex_unlock(&mutex); // release the mutex
    
    return result; // return the message
}

char *stop(char *jobID){
    
    int buffer_size = 0; // size of the result string
    char *result = NULL;
    char *buffer = NULL;
    jobtuple *tuple = NULL; // stored tuple that matches the jobID

    pthread_mutex_lock(&mutex);

    tuple = (jobtuple*)removeNode(network->jobBuffer,jobID,compareByJobID); // search the queued jobQueue

    if(tuple != NULL){ // if found in queued 

        buffer_size = snprintf(NULL,0,"JOB <%s> REMOVED\n",tuple->jobID);  // get the size of the string
        buffer = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocate space for the buffer
        snprintf(buffer,buffer_size + 1,"JOB <%s> REMOVED\n",tuple->jobID); // put message in the buffer
        pthread_cond_signal(&cond_full); // since a job was removed wake a controller thread waiting to input its job inside the jobBuffer

        
    }else{ // if not found in queued

        buffer_size = snprintf(NULL,0,"JOB <%s> NOTFOUND\n",jobID); // get the size of the string
        buffer = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocate space for the buffer
        snprintf(buffer,buffer_size + 1,"JOB <%s> NOTFOUND\n",jobID); // put the message in the buffer
    }

    int indicator = snprintf(NULL,0,"%d",buffer_size); // get the size of the buffer size
    result = (char*)malloc(sizeof(char)*(1 + indicator + buffer_size + 1)); // allocate space for the buffer
    snprintf(result,2 + indicator + buffer_size,"%d%d%s",indicator,buffer_size,buffer); // put the message with the indicator in the buffer
    free(buffer); // deallocate it
    result[1 + indicator + buffer_size] = '\0';

    pthread_mutex_unlock(&mutex); // release the mutex
    
    if(tuple != NULL){ // if a job was removed
        write(tuple->socket,result,1 + indicator + buffer_size); // send to the issueJob commander of that job a message so it won't hang
        shutdown(tuple->socket,SHUT_WR); // close the socket
        jobtuple_destructor(tuple); // deallocate the tuple
    }

    return result; // return the message
}

char *poll(){

    char *result = NULL;
    char *buffer = NULL;

    pthread_mutex_lock(&mutex);

    buffer = (char*)traverse(network->jobBuffer,concatenated_string); // get all jobs in jobBuffer

    if(buffer == NULL || buffer[0] == '\0'){ // if the jobBuffer is empty or something did not work return empty tuple <>

        int buffer_size = snprintf(NULL,0,"<>\n"); // get the size of the string
        buffer = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocate space for the buffer
        snprintf(buffer,buffer_size + 1,"<>\n"); // put message in the buffer
    }

    int buffer_size = strlen(buffer); // get the buffer size

    int indicator = snprintf(NULL,0,"%d",buffer_size); // get the indicator for the buffer size
    result = (char*)malloc(sizeof(char)*(1 + indicator + buffer_size + 1)); // allocate space for the buffer
    snprintf(result,2 + indicator + buffer_size,"%d%d%s",indicator,buffer_size,buffer); // put the message with the indicator in the buffer
    free(buffer); // deallocate it
    result[1 + indicator + buffer_size] = '\0';

    pthread_mutex_unlock(&mutex); // release the mutex


    return result; // return the message
}

char *server_exit(){

    char *result = NULL;

    pthread_mutex_lock(&mutex); // get the mutex

    network->EXIT = true; // change the flag of EXIT to true

    while(empty(network->jobBuffer) == false){ // remove all the tuples from the queue
        jobtuple *tuple = (jobtuple*)dequeue(network->jobBuffer); // remove the first tuple of the queue
        int buffer_size = snprintf(NULL,0,"SERVER TERMINATED BEFORE EXECUTION\n"); // get the size of the string
        char *buffer = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocate space for the buffer
        snprintf(buffer,buffer_size + 1,"SERVER TERMINATED BEFORE EXECUTION\n"); // put the message in the buffer
        buffer[buffer_size] = '\0';

        int indicator = snprintf(NULL,0,"%d",buffer_size); // get the indicator for the buffer size
        result = (char*)malloc(sizeof(char)*(1 + indicator + buffer_size + 1)); // allocate space for the buffer
        snprintf(result,2 + indicator + buffer_size,"%d%d%s",indicator,buffer_size,buffer); // put the message with the indicator in the buffer
        free(buffer); // deallocate it
        result[1 + indicator + buffer_size] = '\0';

        write(tuple->socket,result,strlen(result)); // write to the socket of the issueJobs that are awaiting for the output

        shutdown(tuple->socket,SHUT_WR); // close the socket from the write permission
        jobtuple_destructor(tuple); // deallocate the tuple
        free(result); // deallocate the message
    }

    pthread_cond_broadcast(&cond_concurrency); // wake up any inactive worker stack in the concurrency waiting
    pthread_cond_broadcast(&cond_empty); // wake up any worker stack in the empty queue waiting

    while(network->activeWorkers > 0){  // as long as there are active workers
        pthread_cond_wait(&cond_allInactive, &mutex); // wait for the last worker to deactivate
    }

    pthread_mutex_unlock(&mutex); // release the mutex

    // destroy the condition variables 
    pthread_cond_destroy(&cond_allInactive); 
    pthread_cond_destroy(&cond_concurrency);    
    pthread_cond_destroy(&cond_empty);    
    pthread_cond_destroy(&cond_full);

    pthread_mutex_destroy(&mutex); // destroy the mutex

    int buffer_size = snprintf(NULL,0,"SERVER TERMINATED\n"); // get the size of the string
    char *buffer = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocate space for the buffer
    snprintf(buffer,buffer_size + 1,"SERVER TERMINATED\n"); // put the message in the buffer
    buffer[buffer_size] = '\0';

    int indicator = snprintf(NULL,0,"%d",buffer_size); // get the indicator for the buffer size
    result = (char*)malloc(sizeof(char)*(1 + indicator + buffer_size + 1)); // allocate space for the buffer
    snprintf(result,2 + indicator + buffer_size,"%d%d%s",indicator,buffer_size,buffer); // put the message with the indicator in the buffer
    free(buffer); //deallocate the buffer
    result[1 + indicator + buffer_size] = '\0';

    return result; // return the message
}