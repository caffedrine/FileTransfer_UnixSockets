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
int main (int, char *[]);


/********************
 * main
 ********************/
int main (int argc, char *argv[])
{
	int i;
	int sockfd = 0, n = 0;
	char buffer[10];
	char *p;
	struct sockaddr_in serv_addr; 
	char *filename;
	
	if (argc != 5)
	{
		printf ("Error: Please enter all the four arguments");
		return 1;
	} 
    
    //making the port number greater than 4500
    if(atoi(argv[1]) < 4500) {
        printf("Enter PORT_NO that is above 4500 \n");
        exit(-1);
    }

	// set up
	memset (buffer, '0', sizeof (buffer));
	memset (&serv_addr, '0', sizeof (serv_addr)); 

	// open socket
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf ("Error : Could not create socket \n");
		return 1;
	} 

	// set address
	serv_addr.sin_family = AF_INET;
    
    //converting string into int using atoi for port number 
	serv_addr.sin_port = htons(atoi(argv[1]));

	if (inet_pton (AF_INET, argv[2], &serv_addr.sin_addr) <= 0)
	{
		printf ("inet_pton error occured\n");
		return 1;
	} 

	// connect
	if (connect (sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
	{
		printf ("Error : Connect Failed \n");
		return 1;
	} 

	// input, send to server, receive it back, and output it
	   int r=0;
        //sending the dest file's name to the server
		filename = (char *)malloc(strlen(argv[4])+1);
		strcpy(filename,argv[4]);
		strcat(filename,"\0");
		write (sockfd, filename, strlen(filename)); 
        
        //opening the source file, reading it and sending to server
        FILE *src = fopen(argv[3], "rb");
        while((r=fread(buffer, 1, BUFF_LEN, src))>0) {
            write(sockfd, buffer, r);
        } 
	close (sockfd);
    fclose(src); //Question: I can't close it outside the while, right?

	return 0;
}