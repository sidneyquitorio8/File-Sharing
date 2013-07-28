//
//  main.c
//  Peer
//
//  Created by Sidney Quitorio on 5/3/13.
//  Copyright (c) 2013 Γ080 Sidney Quitorio. All rights reserved.
//

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BACKLOG 20
struct file {
    char filename[1024];
    char servername[50];
    int has;
};

//global variables
char rootPath[512] = "";
struct file fileList[50] = { {'\0' }, {'\0'}, 0  };
char *serverList[10] = {};
char whoiam[50] = "";
int numberOfOtherServers = 0;



void processHello(int clientSock, int reply) {
    int duplicate = 0;
    int j = 0;
    char buf[50] = "";
    int eof = 0;
    char currentLine[50] = "";
    char *currentLinePointer = currentLine;
    char serverHelloName[50] = "";
    pthread_mutex_t lock;
    int i = 0;
    int alreadyInServerList = 0;
    char *firstCurrentLineMemory = currentLinePointer;
    
    //put a mutex just incase 2 servers start up at the same time and try to input their functions to the functionList, and their names to the serverList
    if(pthread_mutex_init(&lock, NULL) != 0) {
        perror("mutex init failed\n");
        exit(0);
    }
    
    while( (recv(clientSock, currentLinePointer, 1, 0) != 0) && (*currentLinePointer != '\n')) {
        currentLinePointer++;
    }
    *currentLinePointer = '\0';
    
    strcpy(serverHelloName, currentLine);
    
    //critical section. protects serverList, numberOfServers, fileList
    pthread_mutex_lock(&lock);
    
    //add server to the list if it isnt already in there
    i = 0;
    if ( serverList[i] == NULL ) {
        serverList[i] = malloc(strlen(currentLine));
    }
    while(strcmp(serverList[i], "") != 0) {
        if(strcmp(serverList[i], currentLine) == 0) {
            alreadyInServerList = 1;
            break;
        }
        i++;
        if ( serverList[i] == NULL ) {
            serverList[i] = malloc(strlen(currentLine));
        }
    }
    
    if(alreadyInServerList == 0) {
        if ( serverList[numberOfOtherServers] == NULL ) {
            serverList[numberOfOtherServers] = malloc(strlen(currentLine) + 1);
        }
        strcpy(serverList[numberOfOtherServers], currentLine);
        numberOfOtherServers++;
    }
    
    //add peers files to its fileList
    while(eof != 1) {
        duplicate = 0;
        i = 0;
        memset(currentLine, '\0', sizeof(currentLine));
        currentLinePointer = firstCurrentLineMemory;
        while( (recv(clientSock, currentLinePointer, 1, 0) != 0) && ((*currentLinePointer != '\n') && (*currentLinePointer != '\r') )) {
            currentLinePointer++;
        }
        //check if its the end of statement character i made up
        if(*currentLinePointer == '\r') {
            eof =1;
            break;
        }
        *currentLinePointer = '\0';
        
        //once you have the file in current line, store it in the fileList along with the server that provided it
        
        while(strcmp(fileList[i].filename, "") != 0) {
            if(strcmp(fileList[i].filename, currentLine) == 0 && strcmp(fileList[i].servername, serverHelloName) == 0) {
                duplicate = 1;
            }
            i++;
        }
        if(duplicate != 1) {
            strcpy(fileList[i].filename, currentLine);
            strcpy(fileList[i].servername, serverHelloName);
        }
    }
    
    pthread_mutex_unlock(&lock);
    pthread_mutex_destroy(&lock);
    
    if(reply == 1) {
        //send peer hello messae
        char temp[50];
        strcpy(temp, whoiam);
        strcat(temp, "\n");
        strcpy(buf, temp);
        send(clientSock, buf, strlen(buf), 0);
        for(j = 0; strcmp(fileList[j].filename, "") != 0; j++) {
            if(fileList[j].has == 1) {
                strcpy(buf, "");
                strcpy(buf, fileList[j].filename);
                strcat(buf, "\n");
                send(clientSock, buf, strlen(buf), 0);
            }
        }
        strcpy(buf, "\r");
        send(clientSock, buf, strlen(buf), 0);
        //close socket. important!
        close(clientSock);
    }
    
    printf("Successfully received a hello messagee from: %s\n", serverHelloName);
    fileList[1].filename;
    
}


void sendFile(int clientSock) {
    FILE *requestedFile = NULL;
    long fileSize = 0;
    struct stat st;
    long bytesRead;
    char buffer[1024];
    char receive[50] = "";
    char path[1024] = "";
    char name[1024] = "";
    int len = 0;
    int i = 0;
    int found = 0;
    char temp[1024] = "";
    int numberOfFiles = 0;
    int j = 0;
    
    read(clientSock, receive, sizeof(receive));
    
    strcat(name, receive);
    strcpy(path, rootPath);
    strcat(path, name);
    
    //C is the worst :(
    for(i = 0; i < sizeof(path); i++) {
        if(path[i] == '\n') {
            path[i] = '\0';
        }
    }
    
    access(path, R_OK);
    
    i = 0;
    if(errno == ENOENT) {
        while(strcmp(fileList[i].filename, "") != 0 && found != 1) {
            strcpy(temp, fileList[i].filename);
            strcat(temp, "\n");
            if(strcmp(temp, name) == 0 && fileList[i].has == 1) {
                found = 1;
                while(strcmp(fileList[numberOfFiles].filename, "") != 0) {
                    numberOfFiles++;
                }
                for(j = i; j < numberOfFiles; j++ ) {
                    strcpy(fileList[j].filename,fileList[j+1].filename);
                    strcpy(fileList[j].servername, fileList[j+1].servername);
                    fileList[j].has = fileList[j+1].has;
                }
            }
            i++;
        }
        
        printf("Deleted file named: %s", name);
        
        strcpy(buffer, "404");
        send(clientSock, buffer, sizeof(buffer), 0);
    }
    else {
        requestedFile = fopen(path, "rb");
        
        while(!feof(requestedFile)) {
            bytesRead = fread(buffer, 1, sizeof(buffer), requestedFile);
            send(clientSock, buffer, bytesRead, 0);
        }
    }
    close(clientSock);
}

void processDelete(int clientSock) {
    int i = 0;
    char receive[1024];
    int found = 0;
    int numberOfFiles = 0;
    int j = 0;
    char temp[1024];
    char host[1024];
    char *pointer;
    char *currentLinePointer = receive;
    
    while( (recv(clientSock, currentLinePointer, 1, 0) != 0) && (*currentLinePointer != '\r')) {
        currentLinePointer++;
    }
    *currentLinePointer = '\0';
    
    pointer = strtok(receive, "\n");
    pointer = strtok(NULL, "\r");
    strcpy(host, pointer);
    
    while(strcmp(fileList[i].filename, "") != 0 && found != 1) {
        strcpy(temp, fileList[i].filename);
        if(strcmp(temp, receive) == 0 && strcmp(fileList[i].servername,host) == 0) {
            found = 1;
            while(strcmp(fileList[numberOfFiles].filename, "") != 0) {
                numberOfFiles++;
            }
            for(j = i; j < numberOfFiles; j++ ) {
                strcpy(fileList[j].filename,fileList[j+1].filename);
                strcpy(fileList[j].servername, fileList[j+1].servername);
                fileList[j].has = fileList[j].has;
            }
        }
        i++;
    }
    printf("Deleted file hosted by: %s \nNamed: %s\n", host ,receive);
    fflush(stdout);
}


void request_handler(int clientSock) {
    char buf[1024];
    char command[50] = "";
    char *request = command;
    while( (recv(clientSock, request, 1, 0) != 0) && (*request != '\n')) {
        request++;
    }
    *request = '\0';
    
    
    //check if it is SERVER/HELLO command
    if(strcmp(command, "PEER/HELLO") == 0) {
        processHello(clientSock, 1);
    }
    else if(strcmp(command, "PEER/UPDATE") == 0) {
        processHello(clientSock, 0);
    }
    //check if it is a client request command
    else if(strcmp(command, "PEER/REQUEST") == 0) {
        sendFile(clientSock);
    }
    else if(strcmp(command, "PEER/DELETE") == 0) {
        processDelete(clientSock);
    }
    else {
        strcpy(buf, "PEER/ERROR\n");
        send(clientSock, buf, strlen(buf), 0);
    }
    
}


void listener(int portNumber) {
    int server_sock_desc;
    struct sockaddr_in name;
    
    int client_sock_desc;
    struct sockaddr_in client_name;
    socklen_t addr_size;
    
    pthread_t handler_thread;
    
    //connection setup
    server_sock_desc = socket(PF_INET, SOCK_STREAM, 0);
    if(server_sock_desc != -1) {
        memset(&name, 0, sizeof(name));
        name.sin_family = AF_INET;
        name.sin_port = htons(portNumber);
        name.sin_addr.s_addr = htonl(INADDR_ANY);
        int bind_result = bind(server_sock_desc, (struct sockaddr *) &name, sizeof(name));
        if(bind_result == 0) {
            if(listen(server_sock_desc, BACKLOG) < 0) {
                perror("listen failed");
            }
            
            addr_size = sizeof(client_name);
            
            //Server Loop will continue to run listening for clients connecting to the server
            printf("Starting on %s\n", whoiam);
            fflush(stdout);
            while(1) {
                
                //new client attempting to connect to the server
                client_sock_desc = accept(server_sock_desc, (struct sockaddr *) &client_name, &addr_size);
                if(client_sock_desc == -1) {
                    if(errno == EINTR) {
                        continue;
                    }
                    else {
                        perror("accept failed");
                        exit(1);
                    }
                }
                
                //connection starts here
                
                //create a thread for the new clients request to be handled
                if(pthread_create(&handler_thread, NULL, request_handler, client_sock_desc) != 0) {
                    perror("pthread_create failed");
                }
            }
        }
        else {
            perror("bind failed");
        }
    }
    else {
        perror("socket failed");
    }
    
}


int main(int argc, const char * argv[])
{
    int b=0;
    int a = 0;
    int founded = 0;
    int k =0;
    int m=0;
    int numFil=0;
    char serverHosting[1024] = "";
    char file[1024] = "";
    char hold[1024];
    int exist = 0;
    char wholeRequest[1024] = "";
    char command[1024] = "";
    int over = 0;
    int j = 0;
    char buf[1024] = "";
    int server_send_sock_desc;
    struct sockaddr_in destination_addr;
    char *serverpointer;
    char server[50] = "";
    char holder[50];
    int serverPort=0;
    char hostName[50] = "";
    int portNumber = 0;
    char *configPath = argv[1];
    FILE *config;
    FILE *newfile;
    char buffer[1024];
    long receivedBytes;
    int len;
    struct sockaddr_in server_addr;
    struct hostent *he;
    char *localIP;
    int client_sock_desc;
    int i = 0;
    pthread_t listenerThread;
    DIR *dir;
    struct dirent *ent;
    int download = 0;
    
    
    //set up peers configuration and list of files
    if(argv[1] == NULL) {
        perror("Must set config file, try again\n");
        exit(0);
    }
    
    config = fopen(configPath, "r");
    if(config == NULL) {
        perror("Config file not found, try again\n");
        exit(0);
    }
    //save the peers hostname and portnumber
    fscanf(config, "%s", hostName);
    strcpy(whoiam, hostName);
    serverpointer = strtok(hostName, ":");
    serverpointer = strtok(NULL, ":");
    portNumber = atoi(serverpointer);
    
    fscanf(config, "%s", rootPath);
    //store the name of files it has in its directory in the peers fileList
    if ( (dir = opendir(rootPath)) != NULL) {
        while ( ((ent = readdir (dir)) != NULL)) {
            if(strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 && strcmp(ent->d_name, ".DS_Store") != 0 )  {
                strcpy(fileList[i].filename, ent->d_name);
                fileList[i].has = 1;
                i++;
            }
        }
        closedir (dir);
    } else {
        //could not open directory
        perror ("Root directory not found, check config file\n");
        exit(0);
    }
    
    //store the names of the servers given as parameters, so peer can send hello message
    i = 2;
    while(argv[i] != NULL) {
        serverList[i-2] = argv[i];
        i++;
    }
    numberOfOtherServers = i-2;
    
    
    /**
     Setup connection to send HELLO message to other peers
     */
    //Create send socket for the HELLO message
    for(i = 0; i < numberOfOtherServers; i++) {
        strcpy(holder, serverList[i]);
        serverpointer = strtok(holder, ":");
        strcpy(server, serverpointer);
        serverpointer = strtok(NULL, ":");
        serverPort = atoi(serverpointer);
        
        if((he = gethostbyname(server)) == NULL ) {
            
            perror(strcat("Error sending hello. Cannot find peer named:\n", server));
            exit(0);
        }
        
        he = gethostbyname(server);
        localIP = inet_ntoa(*(struct in_addr *)*he->h_addr_list);
        server_send_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
        destination_addr.sin_family = AF_INET;
        destination_addr.sin_addr.s_addr = inet_addr(localIP);
        destination_addr.sin_port = htons(serverPort);
        len = sizeof(destination_addr);
        if(connect(server_send_sock_desc, (struct sockaddr *)&destination_addr,len) == -1) {
            perror("Failed to connect to send HELLO message\n");
            exit(0);
        }
        //send peer hello messae
        strcpy(buf, "PEER/HELLO\n");
        send(server_send_sock_desc, buf, strlen(buf), 0);
        char temp[50];
        strcpy(temp, whoiam);
        strcat(temp, "\n");
        strcpy(buf, temp);
        send(server_send_sock_desc, buf, strlen(buf), 0);
        for(j = 0; strcmp(fileList[j].filename, "") != 0; j++) {
            if(fileList[j].has == 1) {
                strcpy(buf, "");
                strcpy(buf, fileList[j].filename);
                strcat(buf, "\n");
                send(server_send_sock_desc, buf, strlen(buf), 0);
            }
        }
        strcpy(buf, "\r");
        send(server_send_sock_desc, buf, strlen(buf), 0);
        processHello(server_send_sock_desc, 0);
        close(server_send_sock_desc);
    }
    
    
    
    
    /**
     he = gethostbyname("localhost");
     localIP = inet_ntoa(*(struct in_addr *)*he->h_addr_list);
     client_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
     server_addr.sin_family = AF_INET;
     server_addr.sin_addr.s_addr = inet_addr(localIP);
     server_addr.sin_port = htons(5000);
     len = sizeof(server_addr);
     if(connect(client_sock_desc, (struct sockaddr *)&server_addr,len) == -1) {
     perror("Client failed to connect");
     }
     
     newpicture = fopen("/Users/Sidney/Documents/C/CS470_Assignment_3/root/a.mp3", "wb");
     while((receivedBytes = read(client_sock_desc, buffer, sizeof(buffer))) > 0) {
     fwrite(buffer, 1, receivedBytes, newpicture);
     }
     **/
    
    if(pthread_create(&listenerThread, NULL,listener, portNumber) != 0) {
        perror("Listener thread create failed");
    }
    
    //pthread_join(listenerThread, NULL);
    
    while(over == 0) {
        printf("Enter command \"quit\" \"list\" \"my_files\" or [filename]:\n");
        fflush(stdout);
        fgets(command, sizeof(command), stdin);
        if(strcmp(command, "quit\n") == 0) {
            printf("Peer shutting down\n");
            over = 1;
        }
        else if(strcmp(command, "list\n") == 0) {
            printf("\n");
            for(i = 0; strcmp(fileList[i].filename, "") != 0; i++) {
                if(fileList[i].has ==0) {
                    printf("%s hosted by %s\n", fileList[i].filename, fileList[i].servername);
                }
            }
            printf("\n");
            fflush(stdout);
        }
        else if(strcmp(command, "my_files\n") == 0) {
            printf("\n");
            for(i = 0; strcmp(fileList[i].filename, "") != 0; i++) {
                if(fileList[i].has ==1) {
                    printf("%s\n", fileList[i].filename);
                }
            }
            printf("\n");
            fflush(stdout);
        }
        else { //must be a filename
            //check if the peer knows of the filename
            for(i = 0; strcmp(fileList[i].filename, "") != 0; i++) {
                memset(hold, '\0', sizeof(hold));
                strcpy(hold, fileList[i].filename);
                strcat(hold, "\n");
                if(strcmp(hold, command) == 0 && fileList[i].has == 0) {
                    strcpy(serverHosting, fileList[i].servername);
                    exist = 1;
                    break;
                }
            }
            if(exist == 1) { //file was found, so request it
                strcpy(holder, fileList[i].servername);
                serverpointer = strtok(holder, ":");
                strcpy(server, serverpointer);
                serverpointer = strtok(NULL, ":");
                serverPort = atoi(serverpointer);
                
                he = gethostbyname(server);
                localIP = inet_ntoa(*(struct in_addr *)*he->h_addr_list);
                client_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
                server_addr.sin_family = AF_INET;
                server_addr.sin_addr.s_addr = inet_addr(localIP);
                server_addr.sin_port = htons(serverPort);
                len = sizeof(server_addr);
                if(connect(client_sock_desc, (struct sockaddr *)&server_addr,len) != -1) {
                    
                    
                    memset(buf, '\0', sizeof(buf));
                    
                    strcpy(buf, "PEER/REQUEST\n");
                    send(client_sock_desc, buf, strlen(buf), 0);
                    //send actual file request
                    strcpy(buf, command);
                    send(client_sock_desc, buf, strlen(buf), 0);
                    memset(hold, '\0', sizeof(hold));
                    strcpy(hold, rootPath);
                    strcat(hold, fileList[i].filename);
                    strcpy(file, fileList[i].filename);
                    newfile = fopen(hold, "wb");
                    while((receivedBytes = read(client_sock_desc, buffer, sizeof(buffer))) > 0) {
                        if(strncasecmp(buffer, "404", 3) == 0) {
                            printf("That peer does not currently host this file, try again later\n");
                            fflush(stdout);
                            remove(hold);
                            
                            k = 0;
                            //delete file from peers list
                            while(strcmp(fileList[k].filename, "") != 0 && founded != 1) {
                                if(strcmp(file, fileList[k].filename) == 0 && strcmp(fileList[k].servername, serverHosting) == 0) {
                                    founded = 1;
                                    while(strcmp(fileList[numFil].filename, "") != 0) {
                                        numFil++;
                                    }
                                    for(m = k; m < numFil; m++ ) {
                                        strcpy(fileList[m].filename,fileList[m+1].filename);
                                        strcpy(fileList[m].servername, fileList[m+1].servername);
                                        fileList[m].has = fileList[m+1].has;
                                    }
                                }
                                k++;
                            }
                            
                            //tell other peers file is not hosted anymore
                            //Create send socket for the DELETE message
                            for(i = 0; i < numberOfOtherServers; i++) {
                                if(strcmp(serverList[i], serverHosting) != 0) {
                                    
                                    strcpy(holder, serverList[i]);
                                    serverpointer = strtok(holder, ":");
                                    strcpy(server, serverpointer);
                                    serverpointer = strtok(NULL, ":");
                                    serverPort = atoi(serverpointer);
                                    
                                    if((he = gethostbyname(server)) != NULL ) {
                                        
                                        // perror(strcat("Error sending hello. Cannot find peer named:\n", server));
                                        
                                        
                                        he = gethostbyname(server);
                                        localIP = inet_ntoa(*(struct in_addr *)*he->h_addr_list);
                                        server_send_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
                                        destination_addr.sin_family = AF_INET;
                                        destination_addr.sin_addr.s_addr = inet_addr(localIP);
                                        destination_addr.sin_port = htons(serverPort);
                                        len = sizeof(destination_addr);
                                        if(connect(server_send_sock_desc, (struct sockaddr *)&destination_addr,len) != -1) {
                                            
                                            //send peer hello messae
                                            strcpy(buf, "PEER/DELETE\n");
                                            send(server_send_sock_desc, buf, strlen(buf), 0);
                                            char temp[50];
                                            strcpy(temp, file);
                                            strcat(temp, "\n");
                                            strcat(temp, serverHosting);
                                            strcat(temp, "\r");
                                            strcpy(buf, temp);
                                            send(server_send_sock_desc, buf, strlen(buf), 0);
                                            close(server_send_sock_desc);
                                        }
                                    }
                                }
                                
                            }
                            memset(&serverHosting, '\0', sizeof(serverHosting));
                            download = 0;
                            break;
                        }
                        download = 1;
                        fwrite(buffer, 1, receivedBytes, newfile);
                    }
                    
                    if(download == 1) {
                        printf("Successful download\n");
                        //add file to list of files
                        i = 0;
                        while(strcmp(fileList[i].filename, "") != 0) {
                            i++;
                        }
                        strcpy(fileList[i].filename, file);
                        fileList[i].has = 1;
                        
                        
                        /**
                         Resend hello message to update peers
                         */
                        //Create send socket for the HELLO message
                        for(i = 0; i < numberOfOtherServers; i++) {
                            strcpy(holder, serverList[i]);
                            serverpointer = strtok(holder, ":");
                            strcpy(server, serverpointer);
                            serverpointer = strtok(NULL, ":");
                            serverPort = atoi(serverpointer);
                            
                            if((he = gethostbyname(server)) != NULL ) {
                                
                                // perror(strcat("Error sending hello. Cannot find peer named:\n", server));
                                
                                
                                he = gethostbyname(server);
                                localIP = inet_ntoa(*(struct in_addr *)*he->h_addr_list);
                                server_send_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
                                destination_addr.sin_family = AF_INET;
                                destination_addr.sin_addr.s_addr = inet_addr(localIP);
                                destination_addr.sin_port = htons(serverPort);
                                len = sizeof(destination_addr);
                                if(connect(server_send_sock_desc, (struct sockaddr *)&destination_addr,len) != -1) {
                                    
                                    //send peer hello messae
                                    strcpy(buf, "PEER/UPDATE\n");
                                    send(server_send_sock_desc, buf, strlen(buf), 0);
                                    char temp[50];
                                    strcpy(temp, whoiam);
                                    strcat(temp, "\n");
                                    strcpy(buf, temp);
                                    send(server_send_sock_desc, buf, strlen(buf), 0);
                                    for(j = 0; strcmp(fileList[j].filename, "") != 0; j++) {
                                        if(fileList[j].has == 1) {
                                            strcpy(buf, "");
                                            strcpy(buf, fileList[j].filename);
                                            strcat(buf, "\n");
                                            send(server_send_sock_desc, buf, strlen(buf), 0);
                                        }
                                    }
                                    strcpy(buf, "\r");
                                    send(server_send_sock_desc, buf, strlen(buf), 0);
                                    close(server_send_sock_desc);
                                }
                            }
                        }
                        download = 0;
                    }
                    
                    exist = 0;
                }
                else {
                    perror("That peer is now offline, try again later");
                    
                    while(strcmp(fileList[a].filename, "") != 0) {
                        if(strcmp(fileList[a].servername, serverHosting) == 0) {
                            
                            //send other peers delete messages for each file
                            for(b = 0; b < numberOfOtherServers; b++) {
                                if(strcmp(serverList[b], serverHosting) != 0) {
                                    
                                    strcpy(holder, serverList[b]);
                                    serverpointer = strtok(holder, ":");
                                    strcpy(server, serverpointer);
                                    serverpointer = strtok(NULL, ":");
                                    serverPort = atoi(serverpointer);
                                    
                                    if((he = gethostbyname(server)) != NULL ) {
                                        
                                        // perror(strcat("Error sending hello. Cannot find peer named:\n", server));
                                        
                                        
                                        he = gethostbyname(server);
                                        localIP = inet_ntoa(*(struct in_addr *)*he->h_addr_list);
                                        server_send_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
                                        destination_addr.sin_family = AF_INET;
                                        destination_addr.sin_addr.s_addr = inet_addr(localIP);
                                        destination_addr.sin_port = htons(serverPort);
                                        len = sizeof(destination_addr);
                                        if(connect(server_send_sock_desc, (struct sockaddr *)&destination_addr,len) != -1) {
                                            
                                            //send peer hello messae
                                            strcpy(buf, "PEER/DELETE\n");
                                            send(server_send_sock_desc, buf, strlen(buf), 0);
                                            char temp[50];
                                            strcpy(temp, fileList[a].filename);
                                            strcat(temp, "\n");
                                            strcat(temp, serverHosting);
                                            strcat(temp, "\r");
                                            strcpy(buf, temp);
                                            send(server_send_sock_desc, buf, strlen(buf), 0);
                                            close(server_send_sock_desc);
                                        }
                                    }
                                }
                                
                            }
                            
                        }
                        a++;
                    }
                    a= 0;
                    while(strcmp(fileList[a].filename, "") != 0) {
                        k =0;
                        m=0;
                        numFil = 0;
                        if(strcmp(fileList[a].servername, serverHosting) == 0) {
                            //delete each file
                            while(strcmp(fileList[k].filename, "") != 0) {
                                if(strcmp(fileList[a].filename, fileList[k].filename) == 0 && strcmp(fileList[a].servername, serverHosting) == 0) {
                                    while(strcmp(fileList[numFil].filename, "") != 0) {
                                        numFil++;
                                    }
                                    for(m = k; m < numFil; m++ ) {
                                        strcpy(fileList[m].filename,fileList[m+1].filename);
                                        strcpy(fileList[m].servername, fileList[m+1].servername);
                                        fileList[m].has = fileList[m+1].has;
                                    }
                                }
                                k++;
                            }
                            a--;
                        }
                        a++;
                    }
                    
                    
                    memset(&serverHosting, '\0', sizeof(serverHosting));
                }
            }
            else {
                printf("File not found, try again. For list of files enter \"list\":\n");
            }
            
        }
        
    }
}

