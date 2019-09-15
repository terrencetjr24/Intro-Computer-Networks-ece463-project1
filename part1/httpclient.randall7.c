//
//  main.c
//  CompNetProj1
//
//  Created by terrence on 8/31/19.
//  Copyright © 2019 terrence. All rights reserved.
//

#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAXLINE 512
int clientHelp(const char* host, int port);

int main(int argc, const char * argv[]) {
    
    const char* serverName = (char*)argv[1];
    int port =  atoi((char*)argv[2]); //converts input string to integer data type
    char* filePath = (char*)argv[3];
    char buf[MAXLINE] = {0};
    char request[MAXLINE] = {0};

    //1. ESTABLISH CONNECTION
    int clientfd;
    clientfd = clientHelp(serverName, port);
    if(clientfd == -1)
        return EXIT_FAILURE;
    
    //2. SEND GET REQUEST TO SERVER
    sprintf(request, "GET %s HTTP/1.0\r\n\r\n", filePath); //Putting my request into a string
    write(clientfd, request, MAXLINE);
    
    //3. PRINTS SERVER RESPONSE
    read(clientfd, buf, MAXLINE);
        fputs(buf, stdout);
    
    //4. CHECK FOR RESPONSE CODE OF 200
    if(strncmp(buf, "HTTP/1.1 200 OK", 15) != 0){
        printf("RESPONSE CODE NOT 200, NOT OK\n");
        close(clientfd);
        return EXIT_FAILURE;
    }
    
    //5. SEND SECOND GET REQUEST WITH EXTRACTED CONTENT
    clientfd = clientHelp(serverName, port); //reconnecting since HTTp 1.0 closes first connection
        //need to isolate the path at the end of the buffer
    char* parsed;
    char whatIwant[] = "/ece463";
    parsed = strstr(buf, whatIwant);
    
    sprintf(request, "GET %s HTTP/1.0\r\n\r\n", parsed);
    write(clientfd, request, MAXLINE);
    
    //6. PRINT COMPLETE SERVER RESPONSE, (RESPONSE HEADER AND CONTENT)
    long int bytesRead;
    int j;
    //FILE* testf = fopen("testfile.txt", "w");
     while((bytesRead = read(clientfd, buf, MAXLINE - 1))){
         //printf("bytes read %d\n", q);
         for(j = 0; j< bytesRead; j++){
             fputc(buf[j], stdout);
             //fputc(buf[j], testf);
        }
    }
    //fputc('\n', stdout);  //DONT KNOW IF I ACTUALLY NEED THIS OR NOT
    //fclose(testf);
    close(clientfd);
    return 0;
}

int clientHelp(const char* hostname, int port)
{
    int clientfd;
    struct hostent *hp = NULL;
    struct sockaddr_in serveraddr;
    //establish a connection with the server
    clientfd = socket(AF_INET, SOCK_STREAM, 0); //NOT SURE IF THE ZERO IS WHAT I WANT
    
    if(clientfd == -1){
        printf("ERROR ESTABLISHING SOCKET\n");
        return -1;
    }
    hp = gethostbyname(hostname);
    if(hp == NULL){
        printf("ERROR GETTING HOST BY NAME\n");
        return -2;
    }
    
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char*)hp->h_addr, (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);
    
    if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) <0 )
        return -1;
    
    return clientfd;
}

