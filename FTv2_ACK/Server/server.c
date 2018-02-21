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
    uint32_t seq_ack;
    uint32_t len;
    uint32_t cksum;
};

struct PACKET       // packet frame structure
{
    struct HEADER Header;
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

    // Define packet structure
    struct PACKET Packet;
    
    //keep listening for data
    while(1)
    {
        printf("-------------------\nWaiting for clients ACK0...\n");
        fflush(stdout);
        
        // Waiting for ACK0 fron client
        if ( recvfrom(s, &Packet.Header.seq_ack, sizeof(uint32_t), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            continue;
        }   // Process received ACK0
        Packet.Header.seq_ack = ntohl(Packet.Header.seq_ack);
        printf("Got ACK0 with value %d\n", Packet.Header.seq_ack);
        
        // Now that client asked to send data, send back ACK1
        ++Packet.Header.seq_ack;
        uint32_t converted_ack = htonl(Packet.Header.seq_ack);
        if (sendto(s, &converted_ack, sizeof(converted_ack) , 0 , (struct sockaddr *) &si_other, slen)==-1)
        {
            printf("Failed to send ACK1 response...\n");
            continue;
        }
        printf("ACK1 with value %d was send to client...\n", Packet.Header.seq_ack);
        
        // Wait for data length
        if ( recvfrom(s, &Packet.Header.len, sizeof(uint32_t), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            printf("Failed to receive data length...\n");
            continue;
        }
        Packet.Header.len = ntohl(Packet.Header.len);
        printf("Data length to be received is %d bytes", Packet.Header.len);
        
        //print details of the client/peer and the data received
        //printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        
        break;
    }
    
    fclose(s);
    return 0;
}