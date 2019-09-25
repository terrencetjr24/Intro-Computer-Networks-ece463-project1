//
//  multi_service_server.randall7.c
//  part4
//
//  Created by terrence on 9/21/19.
//  Copyright © 2019 terrence. All rights reserved.
//

#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <inttypes.h>

#define LISTENQ 10
#define MAXLINE 255
#define MAX_INT 2147483645

int http_open_listenfd(int);
int ping_setup(int);
int recieveInputs(int conn_fd, char** pathOfFile, int* shift);
void readEncryptAndOutput(int connfd, FILE* fptr, int shift);

int main(int argc, const char * argv[]) {
    int httpListenfd, httpConnfd, httpClientlen, httpPort;
    struct sockaddr_in httpClientaddr;
    struct sockaddr_in pingClientaddr;
    struct hostent *http_hp, *ping_hp;  //Both of these should be pointers just not too sure if this works
    char *http_haddrp, *ping_haddrp;    //Same here, just not sure
    int pingfd, /*pingConnfd,*/ pingClientlen, pingPort;
    char buf[MAXLINE];
    int n,i;
    
    if(argc != 3){printf("Needs input of the desired port numbers\n\n");return EXIT_FAILURE;}
    httpPort =  atoi((char*)argv[1]);
    httpListenfd = http_open_listenfd(httpPort);
    if(httpListenfd == -1){printf("Error establishing a listenfd");return EXIT_FAILURE;}
    pingPort = atoi((char*)argv[2]);
    pingfd = ping_setup(pingPort);
    if(pingfd == -1){printf("Error establishing a listenfd");return EXIT_FAILURE;}
    
    fd_set fd_list;  //fd_list
    int maxfd;
    FD_ZERO(&fd_list);
    if(pingPort - httpListenfd){maxfd = pingPort + 1;}else{maxfd = httpListenfd +1;} //Just a max function
    //http stuff (I just changed the names to stuff so it should all still function properly)
    while(1)
    {
            for(n=0; n<MAXLINE; n++)
                buf[n] = 0;
            FD_SET(httpListenfd, &fd_list);
            FD_SET(pingfd, &fd_list);
        
            if((select(maxfd, &fd_list, NULL, NULL, NULL)) == -1){printf("Error in selecting\n"); return EXIT_FAILURE;}
        
            if(FD_ISSET(httpListenfd, &fd_list)){ //This is for a http connection
                //HTTP
                httpClientlen = sizeof(httpClientaddr);
                httpConnfd = accept(httpListenfd, (struct sockaddr *)&httpClientaddr, &httpClientlen);
                http_hp = gethostbyaddr((const char *)&httpClientaddr.sin_addr.s_addr, sizeof(httpClientaddr.sin_addr.s_addr), AF_INET);
                http_haddrp = inet_ntoa(httpClientaddr.sin_addr);
                char* filePath = NULL;
                filePath = malloc(sizeof(char) * MAXLINE);
                int shift = -68;
                int parent;
                parent = recieveInputs(httpConnfd, &filePath, &shift);
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
                        write(httpConnfd, buf, MAXLINE);
                        close(httpConnfd);return 0;
                    }
                    else{
                        sprintf(buf, "HTTP/1.0 404 Not Found\r\n\r\n");
                        write(httpConnfd, buf, MAXLINE);
                        close(httpConnfd);return 0;
                    }
                }
                sprintf(buf, "HTTP/1.0 200 OK \r\n\r\n");
                write(httpConnfd, buf, strlen(buf));
                readEncryptAndOutput(httpConnfd, fptr, shift);
                fclose(fptr);
            parentDone:
                free(filePath);
                close(httpConnfd);
                if(parent == 0) //if this is the child process, I want it to quit
                    return 0;
            }
        else{   //PING
            pingClientlen = sizeof(pingClientaddr);
            char hostname[NI_MAXHOST];
            memset(&pingClientaddr, 0, sizeof(struct sockaddr_in));
            pingClientaddr.sin_family = AF_INET;
            
            //HERE WORKING ON RECIEVING THE FULL PING, AND BEING ABLE TO OUTPUT THE CORRECT RESPONSE
            char buf2[MAXLINE];
            char buf3[MAXLINE];
            for(i = 0; i <MAXLINE; i++){
                buf2[i] = 0;
                buf3[i] = 0;
            }
            
            n = recvfrom(pingfd, (char*)buf, MAXLINE, 0, (struct sockaddr *) &pingClientaddr, &pingClientlen);
            printf("This is how many bytes were read: %d\n", n);
            //buf[n] = '\0';
            printf("This is what was recieved\n");
            puts(buf);
            //buf[n] = 0;

            uint8_t byte1 = buf[n-4];
            uint8_t byte2 = buf[n-3];
            uint8_t byte3 = buf[n-2];
            uint8_t byte4 = buf[n-1];
            uint32_t total = ( (256^3)* ((uint32_t)byte1)  + (256^2)* ((uint32_t)byte2) + (256)* ((uint32_t)byte3) + ((uint32_t)byte4));
            printf("This will always be the correct value: %" PRIu32"\n", total);
            
            pingClientaddr.sin_addr.s_addr = inet_addr(buf);
            if (getnameinfo((struct sockaddr *) &pingClientaddr, pingClientlen, hostname, sizeof(hostname), NULL, 0, NI_NAMEREQD)) {
                printf("could not resolve hostname\n");
            }
            puts(hostname);
            total= total +1;
            strcpy(buf, hostname);
            strcat(buf, (const char*) &total);
            socklen_t sendsize = sizeof(pingClientaddr);
            //I have the hostname, but I also want to send some number +1 as well
            n = sendto(pingfd, (char *)hostname, NI_NAMEREQD, 0, (const struct sockaddr *) &pingClientaddr, sizeof(pingClientlen));
            
            printf("The number of bytes sent: %d\n", n);
            
            //sendto(pingfd, (const char *)hostname, sizeof(hostname), 0, (const struct sockaddr *) &pingClientaddr, sizeof(pingClientlen));
            //write(pingfd, hostname, sizeof(hostname));
        }
    }
}

int ping_setup(int port){
    int pingfd;
    struct sockaddr_in serveraddr;
    
    if ((pingfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return -1;
    
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    
    if (bind(pingfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    
    return pingfd;
}
int http_open_listenfd(int port){
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
    
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    
    
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;
    
    
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    
    return listenfd;
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
        
        //Fixing lost connection is as easy as checking that I'm writing something with the write command above
        
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
    dummy = "pop";
    free(filePath);
    free(buf);
    free(shiftNumAsStr);
    return parent;
}
