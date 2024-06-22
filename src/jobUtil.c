/* @author : Athanasios Giavridis -- sdi2100022 */

/* Used From Project_1 */

/* The Implementations of the functions of jobUtils.h */

#include "../lib/jobUtil.h"

void printjobtuple(void *tuple){

    jobtuple *data = (jobtuple*)tuple;

    printf("<%s,%s,%d>",data->jobID,data->job,data->queuePosition);
    return;
}

void jobtuple_constructor(jobtuple *tuple, char *jobID, char *job,int socket){

    if (tuple == NULL) {
        printf("Invalid tuple pointer!\n");
        return;
    }

    if (jobID != NULL) {
        tuple->jobID = (char*)malloc((strlen(jobID) + 1) * sizeof(char));
        if (tuple->jobID == NULL) {
            printf("Memory Error!\n");
            printf("Allocation Failed!\n");
            return;
        }
        strcpy(tuple->jobID, jobID);
    } else {
        tuple->jobID = NULL;
    }

    if (job != NULL) {
        tuple->job = (char*)malloc((strlen(job) + 1) * sizeof(char));
        if (tuple->job == NULL) {
            if (tuple->jobID != NULL) {
                free(tuple->jobID);
            }
            printf("Memory Error!\n");
            printf("Allocation Failed!\n");
            return;
        }
        strcpy(tuple->job, job);
    } else {
        tuple->job = NULL;
    }
    tuple->socket = socket; // insert socket from arguments
    tuple->queuePosition = 0; // queuePosition by default is 0, gets the value when queueing
    tuple->pid = -1; // pid by default is -1, gets the value when executing

    return;
}

void jobtuple_destructor(jobtuple *tuple){

    if (tuple == NULL) return; // check if tuple is NULL

    if (tuple->jobID != NULL) { // free the jobID
        free(tuple->jobID);
        tuple->jobID = NULL;
    }
    if (tuple->job != NULL) { // free the job
        free(tuple->job);
        tuple->job = NULL;
    }

    free(tuple); // free the tuple 
    return;
}

/* jobQueue Struct Functions */
void jobQueue_constructor(jobQueue *q){

    q->front = NULL;
    q->rear = NULL;
    q->size = 0;

    return;
}


bool empty(jobQueue *q){

    return (q->front == NULL);
}


void enqueue(jobQueue *q,void *data){

    qnode *new_node = malloc(sizeof(qnode)); //create the new node

    if (new_node == NULL){
        printf("Memory Error!\n");
        printf("Allocation Failed!\n");
        return;
    }

    new_node->data = data;
    new_node->next = NULL;

    if (empty(q)) { // if jobQueue is empty
        q->front = q->rear = new_node; // set both front and rear as the new node
        
        jobtuple *tuple = (jobtuple*)q->rear->data;
        if(tuple->queuePosition != -1){ // only if we are not at the waiting queue
            tuple->queuePosition = 0; 
        }
    }else {
        
        jobtuple *rear_tuple = (jobtuple*)q->rear->data;
        jobtuple *new_tuple = (jobtuple*)new_node->data;

        if(new_tuple->queuePosition != -1){// only if we are not at the waiting queue
            new_tuple->queuePosition = rear_tuple->queuePosition + 1; // get queuePosition from last queued data
        }
        q->rear->next = new_node; // else put the new node at the rear of the queue
        q->rear = new_node;
    }

    q->size++;

    return;
}


void *dequeue(jobQueue *q){

    void *data = NULL;

    if(empty(q)){ //if jobQueue is empty 
        return NULL; //return empty 
    }

    traverse(q,decrement_qposition); // decrease all queuePositions by 1

    qnode *temp = q->front;
    data = temp->data;
    q->front = q->front->next;
    
    q->size--;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);

    return data;
}

void* traverse(jobQueue *q, void (*operate_on_data)(void *,char **result)){

    qnode *current = q->front;

    char *result = malloc(1 * sizeof(char));
    if (result == NULL) {
        printf("Memory Error!\n");
        printf("Allocation Failed!\n");
        return NULL;
    }
    result[0] = '\0';

    while (current != NULL) {
        operate_on_data(current->data,&result);
        current = current->next;
    }

    return result;
}

void decrement_qposition(void *data,char **result){

    jobtuple *tuple = (jobtuple*)data;
    if(tuple->queuePosition < -1){ // do not let queuePosition drop under -1
        return;
    }
    tuple->queuePosition--;
}

void concatenated_string(void *data, char **result) {
    jobtuple *tuple = (jobtuple*)data;
    int buffer_size = snprintf(NULL, 0, "<%s,%s>\n", tuple->jobID, tuple->job);
    char *buffer = (char*)malloc((buffer_size + 1) * sizeof(char));

    if (buffer == NULL) {
        printf("Memory Error!\n");
        printf("Allocation Failed!\n");
        return;
    }

    snprintf(buffer, buffer_size + 1, "<%s,%s>\n", tuple->jobID, tuple->job);

    int current_len = strlen(*result);
    *result = realloc(*result, (current_len + buffer_size + 1) * sizeof(char));
    if (*result == NULL) {
        free(buffer);
        printf("Memory Error!\n");
        printf("Allocation Failed!\n");
        return;
    }

    strcat(*result, buffer);
    free(buffer);
}

void* removeNode(jobQueue *q,void *value,int (*compare)(void *,void *value)){
    
    qnode *current = q->front;
    qnode *prev = NULL;

    while (current != NULL) { // traverse the jobQueue to find the node with the given value
        jobtuple *tuple = (jobtuple*)current->data;

        if (compare(tuple,value) == 0) { // if node found, remove it
            if (prev == NULL) {
                q->front = current->next;// node to be removed is the front node
            } else {
                prev->next = current->next;
            }

            
            if (q->rear == current) { // if rear was removed
                q->rear = prev; // set prev as rear
            }

            
            qnode *temp = current->next;
            while (temp != NULL) {
                jobtuple *temp_tuple = (jobtuple*)temp->data;
                
                if(temp_tuple->queuePosition > -1){
                    temp_tuple->queuePosition--;// decrease the queuePositions after the removed node
                }

                temp = temp->next;
            }

            void *data = current->data; // store the data of the removed node

            free(current);
           
            q->size--; // decrease the size of the jobQueue

            return data; // return the data of the removed node
        }

        prev = current;
        current = current->next;
    }
    return NULL; //if not found return NULL
}

int compareByJobID(void *data, void *value) {

    jobtuple *tuple = (jobtuple *)data;
    char *jobID = (char *)value;

    return strcmp(tuple->jobID, jobID);

}

int compareByPID(void *data, void *value) {

    jobtuple *tuple = (jobtuple *)data;
    pid_t pid = *(pid_t*)value;

    return (tuple->pid - pid);
}


void printjQ(jobQueue *q,void (*printFunction)(void *)){

    qnode *current = q->front;
    while (current != NULL) {
        printFunction(current->data);
        current = current->next;
    }
    printf("\n");
}


void jobQueue_destructor(jobQueue *q){

    qnode *current = q->front;
    while (current != NULL) {
        qnode *temp = current;
        current = current->next;
        
        jobtuple *tuple = (jobtuple*)temp->data;
        if (tuple != NULL) {
            if (tuple->jobID != NULL) {
                free(tuple->jobID);
                tuple->jobID = NULL;
            }
            if (tuple->job != NULL) {
                free(tuple->job);
                tuple->job = NULL;
            }
            free(tuple); // free the jobtuple structure itself
        }
        
        free(temp); // free the qnode
    }
    q->front = NULL;
    q->rear = NULL;

    return;
}


char* strdup (const char* s){
  size_t slen = strlen(s);
  char* result = malloc(slen + 1);
  if(result == NULL)
  {
    return NULL;
  }

  memcpy(result, s, slen+1);
  return result;
}