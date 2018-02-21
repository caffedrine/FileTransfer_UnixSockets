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

uint32_t getChecksum(char *str)
{
    return 100;
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
        memset(Packet.data, '\0', BUFF_LEN);
        printf("-------------------\nWaiting for clients ACK0...\n");
        fflush(stdout);
        
        // Waiting for ACK0 fron client
        if ( recvfrom(s, &Packet.Header.seq_ack, sizeof(Packet.Header.seq_ack), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            continue;
        }   // Process received ACK0
        Packet.Header.seq_ack = ntohl(Packet.Header.seq_ack);
        printf("Got ACK0 with value %d\n", Packet.Header.seq_ack);
        
        // Now that client asked to send data, send back ACK1
        ++Packet.Header.seq_ack;
        uint32_t converted_ack = htonl(Packet.Header.seq_ack);
        if (sendto(s, &converted_ack, sizeof(converted_ack), 0, (struct sockaddr *) &si_other, slen)==-1)
        {
            printf("Failed to send ACK1 response...\n");
            continue;
        }
        printf("ACK1 with value %d was send to client...\n", Packet.Header.seq_ack);
        
        // Wait for data length
        if ( recvfrom(s, &Packet.Header.len, sizeof(Packet.Header.len), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            printf("Failed to receive data length...\n");
            continue;
        }
        Packet.Header.len = ntohl(Packet.Header.len);
        printf("Data length to be received is %d bytes\n", Packet.Header.len);
        
        // Now proceed reading the data
        if ( recvfrom(s, &Packet.data, Packet.Header.len, 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            printf("Failed to receive data length...\n");
            continue;
        }
        printf("Data received: %s\n", Packet.data);
        
        // Now send back checksum to make sure this is the date client sends to us
        Packet.Header.cksum = getChecksum(Packet.data);
        uint32_t checksum_converted = htonl(Packet.Header.cksum);   // convert checksum ina  specific format to be send via network
        if (sendto(s, &checksum_converted, sizeof(checksum_converted), 0, (struct sockaddr *) &si_other, slen)==-1)
        {
            printf("Failed to send checksum back to client...\n");
            continue;
        }
        printf("Checksum with value %d was send to client...\n", Packet.Header.cksum);
    }
    
    fclose(s);
    return 0;
}