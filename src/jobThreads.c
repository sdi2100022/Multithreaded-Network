/* @author : Athanasios Giavridis -- sdi2100022 */

#include "../lib/jobThreads.h"

extern Server *network; // global struct of the server 

extern int mainsockfd;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond_empty;
extern pthread_cond_t cond_full;
extern pthread_cond_t cond_concurrency;
extern pthread_cond_t cond_allInactive;

void *controller_thread(void *arg){

    int sockfd = *(int*)arg; // get sockfd from arg
    free(arg); // deallocate it

    char *buffer_len = (char*)(malloc(sizeof(char)*2)); // allocate 2 bytes
    recv(sockfd,buffer_len,sizeof(char),0); // read from socket the indicator of the buffer_size
    buffer_len[1] = '\0';

    int buffer_size = atoi(buffer_len); // turn it to an integer
    free(buffer_len); // deallocate it

    char *buffer = (char*)malloc(sizeof(char)*(buffer_size + 1)); // allocate space for buffer_size 
    recv(sockfd,buffer,sizeof(char)*buffer_size,0); // read from socket the buffer_size
    buffer[buffer_size] = '\0'; 

    int token_size = atoi(buffer); // turn it to an integer
    free(buffer); // deallocate it

    char *token = (char*)malloc(sizeof(char)*(token_size + 1)); // allocate space for the command of size buffer_size
    recv(sockfd,token,sizeof(char)*token_size,0); // read the command from socket 
    token[token_size] = '\0';

    char *tok_com = strtok(token, " "); // tokenize the command in a command[issueJob,setConcurrency,stop,poll,exit] and the job string that some commands are followed by

    char *command = strdup(tok_com);
    char *job;
    tok_com = strtok(NULL,""); 
    if(tok_com != NULL){
        job = strdup(tok_com);
    }else{
        job = NULL;
    }

    char* message = NULL;

    if(network->EXIT == false){ // handle commands only when server is not in a NON-EXIT state
        if (strcmp(command,"issueJob") == 0) { // check if its the issueJob command
            message = issueJob(job,sockfd);
        } else if (strcmp(command, "setConcurrency") == 0){ // check if its the setConcurrency command
            int N = atoi(job);
            message = setConcurrency(N);
        } else if (strcmp(command, "stop") == 0){ // check if its the stop command
            message = stop(job);
        } else if (strcmp(command, "poll") == 0){ // check if its the poll command
            message = poll();
        } else if (strcmp(command, "exit") == 0){ // check if its the exit command
            message = server_exit();
        }
    }

    if(message != NULL){ // write message to jobCommander
        write(sockfd,message,strlen(message));
        free(message);
    }else{ // write an error message to jobCommander so it will not hang
        char *error = (char*)malloc(sizeof(char)*40);
        snprintf(error,40,"236ERROR!\nSERVER IS IN AN EXIT STATE\n");
        error[39] = '\0';

        write(sockfd,error,strlen(error));
        free(error);

        // Free data
        free(token); 
        free(command);
        if(job != NULL){
            free(job);
        }

        return NULL;
    }

    if(strcmp(command,"issueJob") != 0){ // do not shutdown the socket if its an issueJob command cause it will read a second time
        shutdown(sockfd,SHUT_WR); // shutdown the socket for write permissions
    }

    // Free data
    free(token); 
    free(command);
    if(job != NULL){
        free(job);
    }

    if(network->EXIT == true){ // Exit the main thread if exit command is
        jobQueue_destructor(network->jobBuffer);
        free(network->jobBuffer);
        free(network);
        close(mainsockfd);
        exit(0); 
    }

    return NULL;
}

void *worker_thread(void *arg){

    while (1){

        pthread_mutex_lock(&mutex); // get the mutex
        network->activeWorkers++; // increase the active workers 

        while(network->activeWorkers > network->concurrency && network->EXIT == false){ // check if it can work based on concurrency and if the server is stil active
            network->activeWorkers--; // decrease the active workers as its not running anymore
            pthread_cond_wait(&cond_concurrency,&mutex); // free the mutex from its hand and wait till the condition var is changed by a signal
            network->activeWorkers++; // increase the active workers again
        }

        while(empty(network->jobBuffer) == true && network->EXIT == false){ // check if the jobBuffer is empty and if the server is stil active
            pthread_cond_wait(&cond_empty,&mutex); // free the mutex and wait till there is at least one job in the jobBuffer
        }

        if(network->EXIT == true){ // if the server is to be exited
            network->activeWorkers--; // decrease active workers
            if(network->activeWorkers == 0){ // if its the last worker thread 
                pthread_cond_signal(&cond_allInactive); // send signal to controller thread currently waiting on exit 
            }
            pthread_mutex_unlock(&mutex); // free the mutex
            return NULL; //exit and detach
        }

        jobtuple *tuple = (jobtuple*)dequeue(network->jobBuffer); // get the first jobtuple in queue
        pthread_mutex_unlock(&mutex); // free the mutex

        char *token; // tokenize the command in a command[issueJob,setConcurrency,stop,poll,exit] and the job string 

        char *command_copy = strdup(tuple->job);
        int size = 0;

        token = strtok(command_copy," "); // calculate size of the argv list
        while (token != NULL) {
            size++;
            token = strtok(NULL," ");
        }
        free(command_copy);

        char **argv = (char**)malloc((size + 1)*sizeof(char*)); // allocate memory for the argv list
        int argc = 0;

        command_copy = strdup(tuple->job);
        if(command_copy == NULL){
            free(argv);
            continue;
        }

        token = strtok(command_copy," ");
        while (token != NULL) {
            argv[argc] = strdup(token); // create the argv list
            if(argv[argc] == NULL){
                for (int j = 0; j < argc; j++) {
                    free(argv[j]);
                }
                free(argv);
                free(command_copy);
                continue;
            }
            argc++;
            token = strtok(NULL," ");
        }
        argv[argc] = NULL;
        free(command_copy);

        pid_t pid = fork();
        if(pid == -1){
            perror("fork"); //if there is an error on the fork
            for (int j = 0; j < argc; j++) { 
                free(argv[j]);
            }
            free(argv);
        }else if(pid == 0){ // if it is the child process

            int buffer_size = snprintf(NULL,0,"%d.output",getpid());  // get the length of the filename
            char *filename = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocate space for it 
            snprintf(filename,buffer_size + 1,"%d.output",getpid()); // write the filename inside the buffer
            filename[buffer_size] = '\0';

            int fd = open(filename,O_CREAT | O_RDWR | O_TRUNC,0666); // create and open the file with read and write permissions
            free(filename); // deallocate the filename

            int header_size = snprintf(NULL,0,"-----%s output start-----\n",tuple->jobID); // get length of header
            char *header = (char*)malloc((header_size + 1)*sizeof(char)); // allocate memory for header
            snprintf(header,header_size + 1,"-----%s output start-----\n",tuple->jobID); // write the header inside the buffer

            write(fd,header,header_size); // write the header inside the file
            free(header); // deallocate it

            dup2(fd,STDOUT_FILENO); // redirect the output to the file

            execvp(argv[0], argv); // execute the command

            perror("execvp");
            exit(EXIT_FAILURE);

        }else{
            waitpid(pid, NULL, 0);

            int buffer_size = snprintf(NULL,0,"%d.output",pid); // get the length of the filename
            char *filename = (char*)malloc((buffer_size + 1)*sizeof(char)); // allocate space for it 
            snprintf(filename,buffer_size + 1,"%d.output",pid); // write the filename inside the buffer
            filename[buffer_size] = '\0';

            int fd = open(filename,O_RDWR); // open the filename

            int header_size = snprintf(NULL,0,"-----%s output end-----\n",tuple->jobID);  // get length of header
            char *header = (char*)malloc((header_size + 1)*sizeof(char));  // allocate memory for header
            snprintf(header,header_size + 1,"-----%s output end-----\n",tuple->jobID); // write the header inside the buffer

            lseek(fd, 0, SEEK_END); // go to the end of the file 
            write(fd,header,header_size); // write the header to the file
            free(header); // deallocate it

            lseek(fd, 0, SEEK_SET); // rewind the file to the beginning
            int fileSize = lseek(fd, 0, SEEK_END); // get the size of the file
            lseek(fd, 0, SEEK_SET);  // rewind the file to the beginning

            int indicator = snprintf(NULL,0,"%d",fileSize); // create the indicator for the passing string

            char *content = (char*)malloc(sizeof(char)*(fileSize + 1)); // create the content buffer for the file

            read(fd,content,fileSize); // read the file on the content buffer
            content[fileSize] = '\0';

            int len = snprintf(NULL,0,"%d%d%s",indicator,fileSize,content); // get the size of the result buffer
            char *result = (char*)malloc(sizeof(char)*(len + 1)); // allocate space for it 
            snprintf(result,len + 1,"%d%d%s",indicator,fileSize,content); // write inside the string that will be passed
            result[len] = '\0';

            write(tuple->socket,result,len); // write message to jobCommander

            free(content); //deallocate the content buffer

            shutdown(tuple->socket,SHUT_WR); // shutdown the socket for write permissions

            jobtuple_destructor(tuple); // deallocate the tuple

            unlink(filename); // remove the file pid.output
            free(filename); // deallocate the filename

            for (int j = 0; j < argc; j++) { //free the argv of the exec*
                free(argv[j]);
            }
            free(argv);
        }

        pthread_mutex_lock(&mutex); // get mutex 
        pthread_cond_signal(&cond_full); // signal that the queue is no longer full
        network->activeWorkers--; // decreace the active workers
        pthread_cond_broadcast(&cond_concurrency); // wake up the workers inside the concurrency cause there is less workers now
        pthread_mutex_unlock(&mutex); // free the mutex
    }
}
