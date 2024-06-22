/* @author : Athanasios Giavridis -- sdi2100022 */

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

int main(int argc,char **argv){

    if(argc < 4){ // check if the number of arguments is correct
        printf("ERROR! ");
        printf("Incorrect Syntax!\nCorrect Syntax Is:\n");
        printf("%s [serverName] [portnum] [jobCommanderInputCommand]\n",argv[0]);

        return ERROR_CODE;
    }

    char *serverName = (char*)malloc(sizeof(char)*(strlen(argv[1]) + 1)); // allocate space for the server's name
    strcpy(serverName,argv[1]); // copy it to the allocated string
    serverName[strlen(argv[1])] = '\0';

    int portNum = atoi(argv[2]); // get the portNum from the argument

    /* Get the Input Command inside a string */
    int len = 0; 
    for(int i = 3;i < argc;i++){
        len += strlen(argv[i]) + 1;
    }
    char *inputCommand = (char*)malloc(sizeof(char)*(len));
    strcpy(inputCommand,argv[3]);
    for(int i = 4;i < argc;i++){
        strcat(inputCommand, " ");
        strcat(inputCommand, argv[i]);
    }
    inputCommand[len] = '\0';

    char *token = strtok(inputCommand, " "); // tokenize the command in a command[issueJob,setConcurrency,stop,poll,exit] and the job string that some commands are followed by

    char *command = strdup(token);
    char *job;
    token = strtok(NULL,""); 
    if(token != NULL){
        job = strdup(token);
    }else{
        job = NULL;
    }

    bool run = false; // boolean flag to check if the command is correct

    if ((strcmp(command, "issueJob") == 0) || (strcmp(command, "setConcurrency") == 0) || (strcmp(command, "stop") == 0) || (strcmp(command, "poll") == 0) || (strcmp(command, "exit") == 0)){ // check if its one of the desired command
        run = true;
    }

    if(run == false){ // if its not a desired command do not run the commander
        printf("ERROR! ");
        printf("Invalid jobCommanderInputCommand\n");

        free(command);
        if(job != NULL){
            free(job);
        }
        free(serverName);
        free(inputCommand);

        return ERROR_CODE;
    }

    int sockfd; // socket file descriptor
    struct sockaddr_in serv_addr; // structure of the server address
    struct hostent *host; // the server

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){ // create the socket file descriptor
        printf("ERROR! ");
        printf("Could Not Open Socket");

        free(command);
        if(job != NULL){
            free(job);
        }
        free(serverName);
        free(inputCommand);

    }

    host = gethostbyname(serverName); // find the server by its name
    if(host == NULL){
        printf("ERROR! ");
        printf("Could Not Host\n");

        free(command);
        if(job != NULL){
            free(job);
        }
        free(serverName);
        free(inputCommand);

        return ERROR_CODE;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portNum);
    memcpy(&serv_addr.sin_addr.s_addr,host->h_addr_list[0],sizeof(struct in_addr));

    if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0){ // connect to the server
        printf("ERROR! ");
        printf("Could Not Connect\n");

        free(command);
        if(job != NULL){
            free(job);
        }
        free(serverName);
        free(inputCommand);

        return ERROR_CODE;
    }

    strcpy(inputCommand,command); // create the command again because srtdup tokenized it
    if(job != NULL){
        strcat(inputCommand," ");
        strcat(inputCommand,job);
    }

    int send_size = snprintf(NULL,0,"%s",inputCommand); // get size of the string that will be sent to the jobExecutorServer
    int indicator = snprintf(NULL,0,"%d",send_size); // get the indicator
    char *send = (char*)malloc(sizeof(char)*(1 + indicator + send_size + 1)); // allocate space for the buffer that will be written in the socket
    snprintf(send,1 + indicator + send_size + 1,"%d%d%s",indicator,send_size,inputCommand); // get the string
    send[1 + indicator + send_size] = '\0';

    write(sockfd,send,strlen(send)); // write the command to the jobExecutorServer
    free(send); // deallocate it


    char *buffer_len = (char*)(malloc(sizeof(char)*2)); // allocate 2 bytes
    recv(sockfd,buffer_len,sizeof(char),0); // read from socket the indicator of the buffer_size

    buffer_len[1] = '\0';

    int buffer_size = atoi(buffer_len);  // turn it to an integer
    free(buffer_len); // deallocate it

    char *buffer = (char*)malloc(sizeof(char)*(buffer_size + 1));  // allocate space for buffer_size 
    recv(sockfd,buffer,sizeof(char)*buffer_size,0);  // read from socket the buffer_size
    buffer[buffer_size] = '\0';

    int message_size = atoi(buffer);  // turn it to an integer
    free(buffer); // deallocate it

    char *message = (char*)malloc(sizeof(char)*(message_size + 1));  // allocate space for the message of size buffer_size
    recv(sockfd,message,sizeof(char)*message_size,0); // read the message from socket 
    message[message_size] = '\0';


    printf("%s",message); // print the message 

    if(strcmp(command,"issueJob") == 0 && (strcmp(message,"ERROR!\nSERVER IS IN AN EXIT STATE\n") != 0)){ // if the command was issueJob it awaits a second message

        free(message); // deallocate it
        printf("\n");

        buffer_len = (char*)(malloc(sizeof(char)*2)); // allocate 2 bytes
        recv(sockfd,buffer_len,sizeof(char),0); // read from socket the indicator of the buffer_size
        buffer_len[1] = '\0';

        buffer_size = atoi(buffer_len); // turn it to an integer
        free(buffer_len); // deallocate it

        buffer = (char*)malloc(sizeof(char)*(buffer_size + 1)); // allocate space for buffer_size 
        recv(sockfd,buffer,sizeof(char)*buffer_size,0); // read from socket the buffer_size
        buffer[buffer_size] = '\0';

        message_size = atoi(buffer); // turn it to an integer
        free(buffer); // deallocate it

        message = (char*)malloc(sizeof(char)*(message_size + 1)); // allocate space for the message of size buffer_size
        recv(sockfd,message,sizeof(char)*message_size,0); // read the message from socket 
        message[message_size] = '\0';

        printf("%s",message); // print the message

    }

    free(message); // deallocate it

    // free the rest of the dynamically allocated strings
    free(command);
    if(job != NULL){
        free(job);
    }
    free(serverName);
    free(inputCommand);

    close(sockfd);

    return 0;
}