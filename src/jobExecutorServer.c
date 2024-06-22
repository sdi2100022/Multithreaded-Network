/* @author : Athanasios Giavridis -- sdi2100022 */

#include "../lib/jobThreads.h"

#define SO_REUSEPORT 15

Server *network; // global struct of the server 

int mainsockfd; // the main socket file descriptor
pthread_mutex_t mutex; // the mutex of the threads for the "shared" memory
pthread_cond_t cond_empty; // condition global variable for when the queue is empty
pthread_cond_t cond_full; // condition global variable for when the queue is full
pthread_cond_t cond_concurrency; // condition global variable for when concurrency changed
pthread_cond_t cond_allInactive; // condition global variable for when all workers have exited

int main(int argc,char** argv){

    if(argc < 4){ // check for errors
        printf("ERROR! ");
        printf("Incorrect Syntax!\nCorrect Syntax Is:\n");
        printf("%s [portnum] [bufferSize] [threadPoolSize]\n",argv[0]);

        return ERROR_CODE;
    }

    network = (Server*)malloc(sizeof(Server)); // allocate space for the global server
    if(network == NULL){ 
        printf("Memory Error!\n");
        printf("Allocation Failed!\n");
        return ERROR_CODE;
    }
    network->concurrency = 1; // set the concurrency to its starting value 1
    network->jobBuffer = (jobQueue*)malloc(sizeof(jobQueue)); // allocate space for the jobBuffer of the server
    if(network->jobBuffer == NULL){
        printf("Memory Error!\n");
        printf("Allocation Failed!\n");
        free(network);
        return ERROR_CODE;
    }
    jobQueue_constructor(network->jobBuffer); // call the jobQueue constructor to initialize the jobBuffer
    network->activeWorkers = 0; // set the active workers to 0
    network->EXIT = false; // set the Exit flag to false

    int portnum = atoi(argv[1]); // get portnum from arguments
    int bufferSize = atoi(argv[2]); // get the bufferSize from the arguments
    if(bufferSize <= 0){ //if the bufferSize is less than or equal to zero its an error
        printf("ERROR! ");
        printf("Invalid bufferSize\n");

        jobQueue_destructor(network->jobBuffer);
        free(network->jobBuffer);
        free(network);

        return ERROR_CODE;
    }
    network->bufferSize = bufferSize; // set the bufferSize of the server
    int threadPoolSize = atoi(argv[3]); // get threadPoolSize

    // initialize the mutex and the condition variables
    pthread_mutex_init(&mutex, NULL); 
    pthread_cond_init(&cond_empty, NULL); 
    pthread_cond_init(&cond_full, NULL);
    pthread_cond_init(&cond_concurrency, NULL);
    pthread_cond_init(&cond_allInactive, NULL);

    int newsockfd; // socket of the commander
    struct sockaddr_in serv_addr; // struct for the server address
    int serv_len = sizeof(serv_addr); // get the bytes of the structure
    int opt = 1; // initialize the opt

    if ((mainsockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // create the socket file descriptor of the main thread
        printf("ERROR! ");
        printf("Could Not Open Socket");

        jobQueue_destructor(network->jobBuffer);
        free(network->jobBuffer);
        free(network);

        return ERROR_CODE;
    }

    if (setsockopt(mainsockfd,SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT ,&opt,sizeof(opt)) < 0){ // set the socket options
        printf("ERROR! ");
        printf("Could Not Set Socket Options\n");

        jobQueue_destructor(network->jobBuffer);
        free(network->jobBuffer);
        free(network);

        return ERROR_CODE;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portnum);

    if (bind(mainsockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ // bind the socket to the server
        printf("ERROR! ");
        printf("Could Not Bind\n");

        jobQueue_destructor(network->jobBuffer);
        free(network->jobBuffer);
        free(network);

        return ERROR_CODE;
    }

    listen(mainsockfd, 100); // listen to a max of 100 requests before it starts refusing, [this number was chosen at random it can be changed without affecting the app]

    pthread_t workers; // the workers for our workers threads
    for (int i = 0; i < threadPoolSize; i++) {  
        pthread_create(&workers, NULL, worker_thread, NULL); // create a worker thread
        pthread_detach(workers); // when it returns detach it
    }

    while (1) { // create the main loop for accepting jobCommander requests

        if ((newsockfd = accept(mainsockfd,(struct sockaddr *)&serv_addr,(socklen_t*)&serv_len)) < 0){ // accept a new request to our server
                printf("ERROR! ");
                printf("Could Not Accept\n");

                jobQueue_destructor(network->jobBuffer);
                free(network->jobBuffer);
                free(network);

                return ERROR_CODE;
        }


        int *arg = malloc(sizeof(*arg)); // allocate memory for the controller thread's argument
        if(arg == NULL){
            printf("Memory Error!\n");
            printf("Allocation Failed!\n");

            jobQueue_destructor(network->jobBuffer);
            free(network->jobBuffer);
            free(network);

            return ERROR_CODE;
        }
        *arg = newsockfd; // initialize our argument with the socket file descriptor of our requester
        pthread_t controller; // controller thread
        pthread_create(&controller, NULL, controller_thread, arg); // create our controller thread
        pthread_detach(controller); // when it returns detach it from our main thread
    }

    return 0; // the program should never reach here but exit with the exit(0) from the controller thread
}