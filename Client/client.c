/**************************
   socket example, client
   fall 2017
**************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

//Define bytes to transferred for each iteration
#define BUFF_LEN 10

int main(int, char *[]);


/********************
 * main
 ********************/
int main(int argc, char *argv[])
{
    size_t r;
    int sockfd = 0, n = 0;
    char buffer[BUFF_LEN];
    struct sockaddr_in serv_addr;
    char *inputFilename, *outputFilename;
    FILE *fp;
    
    if(argc != 5)
    {
        // Why not using default arguments insead
        argv[1] = "5555";
        argv[2] = "127.0.0.1";
        argv[3] = "input.txt";
        argv[4] = "output.txt";
    }
    
    inputFilename = argv[3];
    outputFilename = argv[4];
    
    //making the port number greater than 4500
    if(atoi(argv[1]) < 4500)
    {
        printf("Enter PORT_NO that is above 4500 \n");
        exit(1);
    }
    
    // set up buffer and serv_addr
    memset(buffer, '0', sizeof(buffer));
    memset(&serv_addr, '0', sizeof(serv_addr));
    
    // open socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error : Could not create socket \n");
        exit(1);
    }
    
    // set address
    serv_addr.sin_family = AF_INET;
    
    //converting string into int using atoi for port number 
    serv_addr.sin_port = htons( (uint16_t)atoi(argv[1]));
    
    if(inet_pton(AF_INET, argv[2], &serv_addr.sin_addr) <= 0)   // IP to binary
    {
        printf("inet_pton error occured\n");
        exit(1);
    }
    
    // connect
    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Error : Connect Failed \n");
        exit(1);
    }
    
    //sending the dest file's name to the server
    write(sockfd, outputFilename, strlen(outputFilename));
    write(sockfd, "\0", 1); // endstring delimiter
    
    //opening the source file, reading it and sending to server
    fp = fopen(argv[3], "rb");
    while(1)
    {
        r = fread(buffer, 1, BUFF_LEN, fp);    // read every ten bytes
        write(sockfd, buffer, r);
        
        if(r < BUFF_LEN)
            break;
    }
    
    close(sockfd);
    fclose(fp); //Question: I can't close it outside the while, right?
    
    return 0;
}
