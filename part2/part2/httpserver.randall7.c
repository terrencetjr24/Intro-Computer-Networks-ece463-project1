//
//  main.c
//  part2_proj1
//
//  Created by terrence on 9/15/19.
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
void recieveInputs(int conn_fd, char** pathOfFile, int* shift);
void readEncryptAndOutput(int connfd, FILE* fptr, int shift);

int main(int argc, const char * argv[]) {
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    struct hostent *hp;
    char *haddrp;
    char buf[MAXLINE];
    
    if(argc != 2){
        printf("Needs input of the desired port number\n\n");
        return EXIT_FAILURE;
    }
    
    port =  atoi((char*)argv[1]);
    listenfd = open_listenfd(port);
    
    clientlen = sizeof(clientaddr);
    connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
    hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    haddrp = inet_ntoa(clientaddr.sin_addr);
    
    char* filePath = NULL;
    int shift = -68;
    recieveInputs(connfd, &filePath, &shift);
    if((filePath == NULL) && (shift == -68))
        return 0;
    
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
    write(connfd, buf, MAXLINE);
    //Now I need to find the file, "encrypt" the file, and output the encryption
    readEncryptAndOutput(connfd, fptr, shift);
    
    fclose(fptr);
    close(connfd);
    return 0;
}

void readEncryptAndOutput(int connfd, FILE* fptr, int shift)
{
    char buf;
    int holder;
    
    while(fscanf(fptr, "%c", &buf)){
        if( (((int)buf > 64) && ((int)buf < 91)) | (((int)buf > 96) && ((int)buf < 123))){
            if( (((int)buf - shift) > 64) | (((int)buf - shift) > 96))
                write(connfd, buf-shift, 1);
            else if ( (((int)buf - shift) <= 64) ){
                holder = 64 - ((int)buf - shift);
                buf = 'z';
                write(connfd, buf-holder, 1);
            }
            else if ( (((int)buf - shift) <= 96)){
                holder = 96 -((int)buf - shift);
                buf = 'Z';
                write(connfd, buf-holder, 1);
            }
            else
                printf("This shouldn't happen\n");
        }
        else
            write(connfd, buf, 1);
    }
}

void recieveInputs(int conn_fd, char** pathOfFile, int* shift)
{
    char* buf = malloc(sizeof(char) * MAXLINE);
    size_t n;
    char* filePath = NULL;
    filePath = malloc(sizeof(char) * MAXLINE);
    char* shiftNumAsStr = NULL;
    shiftNumAsStr = malloc(sizeof(char) * MAXLINE);
    int shiftNum;
    
    char currLetter;
    int count;
    int offset;
    char* parsed;
    char whatIwant[] = "GET";
    
    do{
        n = read(conn_fd, buf, MAXLINE);//n is the number of characters plus the \r\n\r\n
        parsed = NULL;
        parsed = strcasestr(buf, whatIwant);
        if(n != 0)
            break;
    } while (n == 0);
    write(conn_fd, "test\n", 5);
    
    count = 0;
    currLetter = buf[4];
    offset = 4;
    while(currLetter != ' '){
        filePath[count] = currLetter;
        count++;
        currLetter = buf[count + offset];
        if(currLetter == ' ')
            filePath[count] = '\0';
    }
    printf("This is the extrapolated file path: %s\n\n", filePath);
        
    offset = count+1;
    count =0;
    while(currLetter != ' '){
        shiftNumAsStr[count] = currLetter;
        count++;
        currLetter = buf[count + offset];
        if(currLetter == ' ')
            filePath[count] = '\0';
    }
    shiftNum = atoi((char*)shiftNumAsStr);
    
    printf("This is the extrapolated shitt number: %d\n\n", shiftNum);
    
    *shift = shiftNum;
    *pathOfFile = filePath;

    free(buf);
    free(filePath);
    free(shiftNumAsStr);
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

