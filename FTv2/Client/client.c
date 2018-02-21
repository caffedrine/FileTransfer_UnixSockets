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
#include <tiff.h>

#define SERVER "127.0.0.1"
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

uint32_t getChecksum(const struct PACKET packet)
{
    char xor_element = 'A';
    uint32_t sum = 0;
    
    for(int i=0; i < strlen(packet.data); i++)  // iterate over all chars from string9
    {
        char curr = packet.data[i];
        char xored = xor_element ^ curr;
        
        // get sum of bits
        int setBitsCount = 0;
        for(int i = 0; i <= 7; i++)
            setBitsCount += (curr & ((i << 1) - 1));
        
        sum += setBitsCount;
    }
    return sum;
}

int main(void)
{
    struct PACKET packet;
    strcpy(packet.data, "12345679");
    
    int cs = getChecksum(packet);
    
    printf("Checksum for '%s' is %d", packet.data, cs);
    
    exit(1);
    
    struct sockaddr_in si_other;
    int s, i, slen = sizeof(si_other);
    char buf[BUFF_LEN];
    char message[BUFF_LEN];
    
    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
    
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    
    if(inet_aton(SERVER, &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    
    // Packet structure
    struct PACKET Packet;
    
    while(1)
    {
        //Now that we received ACK we can stard sending actual payload
        printf("-----------\nEnter message (max 10): \n");
        gets(message);
        strcpy(Packet.data, message);
        Packet.Header.seq_ack = 0;
        Packet.Header.len = (uint32_t) strlen(Packet.data);
        Packet.Header.cksum = getChecksum(Packet);             // also fetch checksum
        
        // Send ACK0
        uint32_t converted_ack = htonl(Packet.Header.seq_ack);          // make sure it is in the proper format to be send
        if(sendto(s, &converted_ack, sizeof(uint32_t), 0, (struct sockaddr *) &si_other, slen) == -1)
        {
            printf("Error sending ACK0...\n");
            continue;
        }
        printf("ACK0 send with value %d\n", Packet.Header.seq_ack);
        
        // Wait for ACK1
        if(recvfrom(s, &Packet.Header.seq_ack, sizeof(Packet.Header.seq_ack), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            printf("ACK1 not received\n");
            continue;
        }
        Packet.Header.seq_ack = ntohl(Packet.Header.seq_ack);
        printf("ACK1 received with value %d\n", Packet.Header.seq_ack);
        
        // Now send the length of data we are about to send
        uint32_t converted_len = htonl(Packet.Header.len);
        if(sendto(s, &converted_len, sizeof(sizeof(uint32_t)), 0, (struct sockaddr *) &si_other, slen) == -1)
        {
            printf("Error sending data length...\n");
            continue;
        }
        printf("The length %d was passed to server...\n", Packet.Header.len);
        
        // Now proceed with sending the actual data
        if(sendto(s, &Packet.data, (int) strlen(Packet.data), 0, (struct sockaddr *) &si_other, slen) == -1)
        {
            printf("Error sending data...\n");
            continue;
        }
        printf("Send chunk of %d bytes with content: %s\n", Packet.Header.len, Packet.data);
        
        // Process process checksum received to be sure of data integrity
        uint32_t recvChecksum;
        if(recvfrom(s, &recvChecksum, sizeof(recvChecksum), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            printf("Checksum not received...\n");
            continue;
        }
        recvChecksum = ntohl(recvChecksum);
        printf("Checksum from server: %d\n", recvChecksum);
        printf("Checksum calculated: %d\n", Packet.Header.cksum);
        
        // Compare checksums to assuse data integrity
        if(recvChecksum != Packet.Header.cksum)
        {
            printf("Result: Checksum NOT OK!\n");
        }
        else
        {
            printf("Result: Checksum OK!\n");
        }
    }
    
    close(s);
    return 0;
}