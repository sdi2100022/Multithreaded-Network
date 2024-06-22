/* @author : Athanasios Giavridis -- sdi2100022 */

/* Used From Project_1 */

/* The Header file of the JobUtils.c */

/* 
 * This file contains all the basic utilities 
 * as well as all the definitions of the structures 
 * and their functions used by the application
 */


/* All the needed includes of the application */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define ERROR_CODE -1

/* The index triplet used in the jobQueue structure */
typedef struct jobtuple{
    char *jobID; //jobID is a string with the form "job_XX" where XX is an ascending number with no leading zeros
    char *job; //job is a string containing a regula Unix command
    int queuePosition; //queuePosition is an integer containing the position of the queue the job was placed at
    int socket; //socket of the job 
    pid_t pid;
}jobtuple;

/* Printing a jobtuple */
void printjobtuple(void *tuple);

/* Default Constructor of a jobtuple */
void jobtuple_constructor(jobtuple *tuple,char *jobID,char *job,int socket);

/* Default Destructor of a jobtuple */
void jobtuple_destructor(jobtuple *tuple);

/* The definition of the qnode structure for the jobQueue */
typedef struct qnode {
    void *data; //data of the qnode
    struct qnode *next; //pointer to the next node
}qnode;

/* The definition of the jobQueue structure */
typedef struct jobQueue{
    qnode *front; //front node of the jobQueue
    qnode *rear; //rear node of the jobQueue
    int size; //size of the queue
}jobQueue;

/* Initialize the jobQueue */
void jobQueue_constructor(jobQueue *q);

/* Check if its empty,returns true if its empty else false */
bool empty(jobQueue *q);

/* Enqueue function,inserts data on the rear of the queue */
void enqueue(jobQueue *q,void* data);

/* Dequeue function,releases data from the front of the queue and returns it */
void *dequeue(jobQueue *q);

/* Function that traverses the jobQueue and operates on its data with helper functions */
void* traverse(jobQueue *q, void (*operate_on_data)(void *,char **result));

/* A Helper function of the traverse function,its role is to decrement each queuePosition when a dequeue will happen */
void decrement_qposition(void *data,char **result);

/* A Helper function of the traverse function,its role is to turn all the data of the jobQueue to a string*/
void concatenated_string(void *data,char **result);

/* Function that searches a node based on a value and removes it and returns its data,
 * if the jobID is not found it returns NULL, 
 * compares values with helper functions 
*/
void* removeNode(jobQueue *q,void *value,int (*compare)(void *,void *value));

/* A Helper function of removeNode to compare values using JobID */
int compareByJobID(void *data, void *value);

/* A Helper function of removeNode to compare values using pid */
int compareByPID(void *data, void *value);

/* Print function for personal use to see all the queue at once */
void printjQ(jobQueue *q,void (*printFunction)(void *));

/* Free the jobQueue*/
void jobQueue_destructor(jobQueue *q);

/* Implemented my own strdup cause the #include <string.h> was not working for it */
char* strdup(const char* s);
