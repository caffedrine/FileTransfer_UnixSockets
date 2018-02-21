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

int main(void)
{
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
    
    // Temp packet init
    struct PACKET Packet;
    Packet.Header.seq_ack = 0;
    Packet.Header.len = BUFF_LEN;
    Packet.Header.cksum = 100;
    strcpy(Packet.data, "TestSTR!");
    
    while(1)
    {
        //Now that we received ACK we can stard sending actual payload
        printf("Enter message (max 10): \n");
        gets(message);
        strcpy(Packet.data, message);
        Packet.Header.len = (int) strlen(Packet.data);
        
        // Send ACK0
        uint32_t converted_ack = htonl(
                Packet.Header.seq_ack);          // make sure it is in the proper format to be send
        if(sendto(s, &converted_ack, sizeof(uint32_t), 0, (struct sockaddr *) &si_other, slen) == -1)
        {
            printf("Error sending ACK0...\n");
            continue;
        }
        printf("ACK0 send with value %d\n", Packet.Header.seq_ack);
        
        // Wait for ACK1
        if(recvfrom(s, &Packet.Header.seq_ack, sizeof(uint32_t), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            printf("ACK1 not received\n");
            continue;
        }
        Packet.Header.seq_ack = ntohl(Packet.Header.seq_ack);
        printf("ACK1 received with value %s", Packet.Header.seq_ack);
        
        // Now send the length of data we are about to send
        uint32_t converted_len = htonl(Packet.Header.len);
        if(sendto(s, &converted_len, sizeof(uint32_t), 0, (struct sockaddr *) &si_other, slen) == -1)
        {
            printf("Error sending data length...\n");
            continue;
        }
        printf("The length %d was passed to server...\n", Packet.Header.len);
        
        // Now proceed with sending the actual data
        if(sendto(s, Packet.data, (int) strlen(Packet.data), 0, (struct sockaddr *) &si_other, slen) == -1)
        {
            printf("Error sending data...\n");
            continue;
        }
        printf("Send chunk of %d bytes with content: %s\n", Packet.Header.len, Packet.data);
        
        // Process process checksum received to be sure of data integrity
        int tmpChecksum;
        if(recvfrom(s, (void *) tmpChecksum, 4, 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            printf("Checksum not received...\n");
            continue;
        }
        
        // Compare checksums to assuse data integrity
        if(tmpChecksum == Packet.Header.cksum)
        {
            printf("Data send error! No checksum match...");
        }
        
        printf("-------------------\n");
        break;
    }
    
    close(s);
    return 0;
}