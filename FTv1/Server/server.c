#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>


//Define bytes to transferred for each iteration
#define BUFF_LEN 5
#define MAX_FILE_LENGTH 30

int main(int argc, char *argv[])
{
    if(argc < 2)    //first argument is app name
    {
        // Use a default port
        argv[1] = "5555";
    }
    
    ssize_t n;
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    char buff[5];
    char buff_filename[MAX_FILE_LENGTH] = {0};
    
    // set up
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(buff, '0', sizeof(buff));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons( (uint16_t)atoi(argv[1]) );
    
    // create socket, bind, and listen
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        printf("ERROR: Can't open socket!\n");
        exit(1);
    }

    if( bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Can't bind socket!\n");
        exit(1);
    }
    
    listen(listenfd, 10);
    printf("Waiting for incoming connections on port %s...\n", argv[1]);
    
    // accept and interact
    connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
    if(connfd < 0)
    {
        printf("First onnection FAILED!\n");
    }
    
    // receive dest file's name and open it
    int buffer_index = 0;
    while(buffer_index < MAX_FILE_LENGTH && 1 == read(connfd, &buff_filename[buffer_index], 1) ) // to get corect file name, raed char by char until '\0'
    {
        if(buff_filename[buffer_index] == '\0' || buff_filename[buffer_index] == '\n') // '\n' is just for debugging
        {
            if(buff_filename[buffer_index] == '\n')
                buff_filename[buffer_index] = '\0';
            
            printf("Got filename: %s\n", buff_filename);
            break;
            
        }
        buffer_index++;
    }
    
    // Open file name we just received
    char *p = buff_filename;
    FILE *dest = fopen(p, "a");
    
    //recieve data from src file and write it to dest file
    
    printf("Transfering file...");
    fflush(stdout);
    
    while( (n = read(connfd, &buff, BUFF_LEN)) )    // while we read 5 bytes of data
    {
        fwrite(buff, sizeof(char), n, dest);
        
        if( n < BUFF_LEN)
            break;
    }
    
    printf("done!\n");
    fclose(dest);
    close(connfd);
}
