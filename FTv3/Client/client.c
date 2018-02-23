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
    if(strlen(packet.data) == 0)
        return 0;
    
    char xor_element = 'A';
    uint32_t sum = 0;
    
    for(int i = 0; i < strlen(packet.data); i++)  // iterate over all chars from string9
    {
        char curr = packet.data[i];
        char xored = xor_element ^curr;
        
        // get sum of bits
        int setBitsCount = 0;
        for(int i = 0; i <= 7; i++)
            setBitsCount += (xored & ((i << 1) - 1));
        
        sum += setBitsCount;
    }
    return sum;
}

void checksumTest()
{
    struct PACKET packet;
    strcpy(packet.data, "1234");
    
    int cs = getChecksum(packet);
    
    printf("Checksum for '%s' is %d", packet.data, cs);
    
    exit(1);
}

int main(void)
{
    struct sockaddr_in si_other;
    int sockfd, i;
    socklen_t slen = sizeof(si_other);
    char buf[BUFF_LEN];
    char message[BUFF_LEN];
    
    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("Can allocate socket identifier!");
    
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    
    if(inet_aton(SERVER, &si_other.sin_addr) == 0)
        die("inet_aton() failed\n");
    
    // Setting up timeout
    struct timeval tv;
    tv.tv_sec = 5;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        die("Error on socket timeout!");
    
    // Packet structure
    struct PACKET PacketSend;
    struct PACKET PacketRecv;// = (struct PACKET *)malloc(sizeof(struct PACKET));
    short retry = 0;        //this flag is set whenever a packet need to be resend as it's checksum was wrong
    
    // Input filename and generate payload
    char outputFile[] = "output_file_test.txt\0";
    char inputFile[] = "input.txt";
    FILE *fp = fopen(inputFile, "rb");
    
    while( 1 )
    {
        if(!retry)      // if last packet does not require resend, grab next one
        {
//            if( fread(PacketSend.data, 1, BUFF_LEN, fp) <= BUFF_LEN);
//                break;
            printf("-----------\nEnter message (max 10): \n");
            gets(message);
            strcpy(PacketSend.data, message);
        }
        
        // Send first packet
        PacketSend.Header.seq_ack = 0;
        PacketSend.Header.len = (uint32_t) strlen(PacketSend.data);
        PacketSend.Header.cksum = getChecksum(PacketSend);
        if(sendto(sockfd, &PacketSend, sizeof(PacketSend), 0, (struct sockaddr *) &si_other, slen) == -1)
        {
            printf("Error sending data length...\n");
            retry = 1;
            continue;
        }
        printf("Send %d bytes: '%s', checksum: %d, seq_ack: %d\n", PacketSend.Header.len, PacketSend.data,
               PacketSend.Header.cksum, PacketSend.Header.seq_ack);
        
        // Now wait for acknowledge
        if(recvfrom(sockfd, &PacketRecv.Header.seq_ack, sizeof(PacketRecv), 0, (struct sockaddr *) &si_other,
                    &slen) == -1)
        {
            printf("ACK1 not received -> RETRY\n");
            retry = 1;
            continue;
        }
        
        // Chck whether right packet was received or not
        if(PacketRecv.Header.seq_ack == 0)
        {
            printf("ACK is 0 -> RETRY\n");
            retry = 1;
            continue;
        }
        
        if(PacketRecv.Header.seq_ack == 1)
        {
            printf("Packet succesfully send!\n");
            retry = 0;
        }
        
        // If the packet was empty, just break
    }
    
    return 0;
}