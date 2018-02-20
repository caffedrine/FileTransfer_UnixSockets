 /**************************
 Nithya Geereddy: Lab 2
 COEN 146
 ***************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

//Define bytes to transferred for each iteration
#define BUFF_LEN 5
int main (int, char *[]); 


/*********************
 * main
 *********************/
int main (int argc, char *argv[])
{
	int	n; 
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr; 
	char buff[5];
	char buff_filename[30];
	// set up
	memset (&serv_addr, '0', sizeof (serv_addr));
	memset (buff, '0', sizeof (buff)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1])); 

	// create socket, bind, and listen
	listenfd = socket (AF_INET, SOCK_STREAM, 0);
	bind (listenfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr)); 
	listen (listenfd, 10); 

	// accept and interact

		connfd = accept (listenfd, (struct sockaddr*)NULL, NULL); 
        printf("Connection established");
		
        // receive dest file's name and open it 
		read (connfd, buff_filename, sizeof(buff_filename));
		char *p = buff_filename;
        FILE *dest = fopen(p, "a"); 
        
        //recieve data from src file and write it to dest file
        while ((n = read (connfd, buff, sizeof (buff))) > 0){
            fwrite(buff, 1, n, dest);
        }
        fclose(dest);
        close (connfd);
} /**************************
 Nithya Geereddy: Lab 2
 COEN 146
 ***************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

//Define bytes to transferred for each iteration
#define BUFF_LEN 5
int main (int, char *[]); 


/*********************
 * main
 *********************/
int main (int argc, char *argv[])
{
	int	n; 
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr; 
	char buff[5];
	char buff_filename[30];
	// set up
	memset (&serv_addr, '0', sizeof (serv_addr));
	memset (buff, '0', sizeof (buff)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1])); 

	// create socket, bind, and listen
	listenfd = socket (AF_INET, SOCK_STREAM, 0);
	bind (listenfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr)); 
	listen (listenfd, 10); 

	// accept and interact

		connfd = accept (listenfd, (struct sockaddr*)NULL, NULL); 
        printf("Connection established");
		
        // receive dest file's name and open it 
		read (connfd, buff_filename, sizeof(buff_filename));
		char *p = buff_filename;
        FILE *dest = fopen(p, "a"); 
        
        //recieve data from src file and write it to dest file
        while ((n = read (connfd, buff, sizeof (buff))) > 0){
            fwrite(buff, 1, n, dest);
        }
        fclose(dest);
        close (connfd);
}