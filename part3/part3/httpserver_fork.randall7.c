//
//  main.c
//  part3
//
//  Created by terrence on 9/16/19.
//  Copyright © 2019 terrence. All rights reserved.
//

#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define LISTENQ 10
#define MAXLINE 255
int open_listenfd(int);
int recieveInputs(int conn_fd, char** pathOfFile, int* shift);
void readEncryptAndOutput(int connfd, FILE* fptr, int shift);

int main(int argc, const char * argv[]) {
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    struct hostent *hp;
    char *haddrp;
    char buf[MAXLINE];
    if(argc != 2){printf("Needs input of the desired port number\n\n");return EXIT_FAILURE;}
    
    port =  atoi((char*)argv[1]);
    listenfd = open_listenfd(port);
    
    
    while(1)
    {
        clientlen = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        haddrp = inet_ntoa(clientaddr.sin_addr);
        
        char* filePath = NULL;
        filePath = malloc(sizeof(char) * MAXLINE);
        int shift = -68;
        
        int parent;
        parent = recieveInputs(connfd, &filePath, &shift);
        if(parent != 0)
            goto parentDone;
        if((filePath == NULL) && (shift == -68))
            return 0;
        if(filePath[0] == '.')
            filePath++;
        if(filePath[0] == '/')
            filePath++;
        
        FILE* fptr = NULL;
        fptr = fopen((const char*) filePath, "r");
        if(fptr == NULL)
        {
            if(errno == EACCES){
                sprintf(buf, "HTTP/1.0 403 Forbidden\r\n\r\n");
                write(connfd, buf, MAXLINE);
                close(connfd);
                return 0;
            }
            else{
                sprintf(buf, "HTTP/1.0 404 Not Found\r\n\r\n");
                write(connfd, buf, MAXLINE);
                close(connfd);
                return 0;
            }
        }
        sprintf(buf, "HTTP/1.0 200 OK \r\n\r\n");
        
        write(connfd, buf, strlen(buf));
        readEncryptAndOutput(connfd, fptr, shift);

        fclose(fptr);
    parentDone:
        free(filePath);
        close(connfd);
        if(parent == 0)
            goto childQuit;
    }
childQuit:
    return 0;
}

void readEncryptAndOutput(int connfd, FILE* fptr, int shift){
    char letter = 'p';
    char newLetter = 'p';
    int holder = 0;
    shift = shift % 26;
    
    while((letter = (char)fgetc(fptr)) != EOF){
        if( ((letter >= 'a') && (letter <= 'z')) || ((letter >= 'A') && (letter <= 'Z'))){
            if((letter - shift) < 'A'){
                holder = 64 - ((int)letter - shift);
                newLetter = 'Z' - holder;
            }
            else if ( ((letter >= 'a') && (letter <= 'z')) && ((letter - shift) < 'a')){
                holder = 96 -((int)letter - shift);
                newLetter = 'z' - holder;
            }
            else
                newLetter = letter - shift;
        }
        else{
            newLetter = letter;
        }
        
        sprintf(&letter, "%c", newLetter);
        write(connfd, &letter, 1);
        //printf("The character read: %c\n", letter);  //Just for testing on the server side
        //printf("The character shifted; %c\n", newLetter);
    }
}


int recieveInputs(int conn_fd, char** pathOfFile, int* shift)
{
    int parent = 0;
    char* buf = malloc(sizeof(char) * MAXLINE);
    size_t n;
    char* filePath = NULL;
    filePath = malloc(sizeof(char) * MAXLINE);
    char* shiftNumAsStr = NULL;
    shiftNumAsStr = malloc(sizeof(char) * MAXLINE);
    int shiftNum;
    
    char nextLetter;
    int assignIndex;
    int sourceIndex;
    char* parsed;
    char whatIwant[] = "GET";
    
    do{
        n = read(conn_fd, buf, MAXLINE);//n is the number of characters plus the \r\n\r\n
        parsed = NULL;
        parsed = strcasestr(buf, whatIwant);
        if(n != 0)
            break;
    } while (n == 0);
    //write(conn_fd, buf, n); //Just a check, don't need this line
    parent = fork();
    if(parent != 0)
        return parent;
    assignIndex = 0;
    sourceIndex = 4;
    nextLetter = buf[sourceIndex];
    
    while(nextLetter != ' '){
        filePath[assignIndex++] = buf[sourceIndex++];
        nextLetter = buf[sourceIndex];
        if(nextLetter == ' '){
            filePath[assignIndex] = '\0';
            assignIndex = 0;
            sourceIndex++;
        }
    }
    
    nextLetter = buf[sourceIndex];
    
    while(nextLetter != ' '){
        shiftNumAsStr[assignIndex++] = buf[sourceIndex++];
        nextLetter = buf[sourceIndex];
        if(nextLetter == ' '){
            shiftNumAsStr[assignIndex] = '\0';
        }
    }
    shiftNum = atoi((char*)shiftNumAsStr);
    
    *shift = shiftNum;
    //pathOfFile = filePath;
    char* dummy = strcpy(*pathOfFile, filePath);
    
    free(filePath);
    free(buf);
    free(shiftNumAsStr);
    return parent;
}

int open_listenfd(int port){
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
    
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    
    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;
    
    /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    
    /* Make it a listening socket ready to accept
     connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    
    return listenfd;
}

