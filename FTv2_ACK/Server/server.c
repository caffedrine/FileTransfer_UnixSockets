#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <tiff.h>

#define BUFF_LEN 8  //Max length of buffer
#define PORT 5555   //The port on which to listen for incoming data

struct HEADER       // header structure
{
    int32 seq_ack;
    int len;
    int cksum;
};

struct PACKET       // packet frame structure
{
    struct HEADER;
    char data[BUFF_LEN];
};

void die(char *s)
{
    perror(s);
    exit(1);
}

int main(void)
{
    struct sockaddr_in si_me, si_other;
    
    int s, i, slen = sizeof(si_other) , recv_len;
    char buf[BUFF_LEN];
    
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("Can't create socket...");
    }
    
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
    
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("Can't bind socket...");
    }
    
    //keep listening for data
    while(1)
    {
        printf("Waiting for data...\n");
        fflush(stdout);
        
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, BUFF_LEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
        
        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Data: %s\n" , buf);
        
        //now reply the client with the same data
        if (sendto(s, "ACK\0", 4, 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            die("sendto()");
        }
    }
    
    fclose(s);
    return 0;
}