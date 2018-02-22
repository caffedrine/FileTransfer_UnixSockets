#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <elf.h>

#define UDP_BUFF_LEN    8          // Max length of buffer for UDP data
#define PORT            5555       // The port on which to listen for incoming data

struct HEADER       // header structure
{
    uint32_t seq_ack;
    uint32_t len;
    uint32_t cksum;
};

struct PACKET       // packet frame structure
{
    struct HEADER Header;
    char data[UDP_BUFF_LEN];
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
    strcpy(packet.data, "1234"); // 496
    
    int cs = getChecksum(packet);
    
    printf("Checksum for '%s' is %d", packet.data, cs);
    
    exit(1);
}

uint32_t waitACK0(int *socketfd, struct sockaddr_in *si_other, struct HEADER *Header)
{
    printf("-------------\nWaiting for ACK0...");
    fflush(stdout);
    
    socklen_t slen = sizeof(si_other);
    if(recvfrom(*socketfd, &Header->seq_ack, sizeof(Header->seq_ack), 0, (struct sockaddr *) si_other, &slen) == -1)
    {
        // if there was an error on receiving, return 0;
        printf("FAILED\n");
        return 0;
    }
    Header->seq_ack = ntohl(Header->seq_ack);
    printf("OK: Got ACK0 with value %d\n", Header->seq_ack);
    
    return Header->seq_ack;
}

int main(void)
{
    struct sockaddr_in si_me, si_other;
    int socketfd, i;
    socklen_t slen = sizeof(si_other);
    short finalPacketReceived = 0;      // set this flag when final packet wil length 0 was received
    FILE *fp = NULL;
    char filename[32] = "";
    
    //create a UDP socket
    if((socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("Can't create socket...");
    }
    
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
    
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //bind socket to port
    if(bind(socketfd, (struct sockaddr *) &si_me, sizeof(si_me)) == -1)
    {
        die("Can't bind socket...");
    }
    
    // Define packet structure
    struct PACKET PacketSend;
    struct PACKET PacketRecv;// = (struct PACKET *)malloc(sizeof(struct PACKET));
    
    //keep listening for data
    while(1)
    {
        // Listening for ACK0
        printf("------------\nWaiting for ACK0...");
        fflush(stdout);
        if(recvfrom(socketfd, &PacketRecv, sizeof(PacketRecv), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            // if there was an error on receiving, return 0;
            printf("FAILED\n");
            return 0;
        }
        printf("OK: %d\n", PacketRecv.Header.seq_ack);
        
        // Wait for next packet which will contain data and it's length
        if(recvfrom(socketfd, &PacketRecv, sizeof(PacketRecv), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            printf("Failed to receive a packet...\n");
            continue;
        }
        printf("Got %d bytes: %s\n", PacketRecv.Header.len, PacketRecv.data);
        
        // Check if it is the last packet - last one should have data field empty
        if(PacketRecv.Header.len > 0)
        {
            printf("Checksum test: ");
            if(getChecksum(PacketRecv) == PacketRecv.Header.cksum)
            {
                printf("OK\n");
                PacketSend.Header.seq_ack = 0;
                PacketSend.Header.len = 0;
                PacketSend.Header.cksum = getChecksum(PacketRecv);
                if(sendto(socketfd, &PacketSend, sizeof(PacketSend), 0, (struct sockaddr *) &si_other, slen) == -1)
                {
                    printf("Failed to send ACK1 response...\n");
                    continue;
                }
                printf("ACK1 with value %d was send back..\n", PacketSend.Header.seq_ack);
            }
            else
            {
                //strcat(PacketRecv.data, "\0");
                printf("%d %d %d %d %s\n", getChecksum(PacketRecv), PacketRecv.Header.cksum, PacketRecv.Header.len, PacketRecv.Header.seq_ack, PacketRecv.data);
                
                printf("FAIL\n");
                PacketSend.Header.seq_ack = 0;
                PacketSend.Header.len = 0;
                PacketSend.Header.cksum = getChecksum(PacketRecv);
                if(sendto(socketfd, &PacketSend, sizeof(PacketSend), 0, (struct sockaddr *) &si_other, slen) == -1)
                {
                    printf("Failed to send ACK1 response...\n");
                    continue;
                }
                memset(PacketRecv.data, '\0', sizeof(PacketRecv.data));
                printf("ACK1 with value %d was send back..\n", PacketSend.Header.seq_ack);
                continue;
            }
        }
        else    // if data length < 0 this mean that the file packets was done - write the rest to the file and then break
        {
            finalPacketReceived = 1;
        }
        
        // Write data to file or append
        if(fp == NULL)  // file pointer is null only if we didn't get the filename yet
        {
            if(finalPacketReceived)
            {
                fclose(fp);
                break;
            }
            
            // Concat data until filename is complete:
            strcat(filename, PacketRecv.data);
            
            unsigned long index;
            const char *p = strchr(filename, '\0');
            if(!p)                              // this mean that filename length is higher than buffer
            {
                printf("Filename length biger than strg_buffer!");
                exit(1);
            }
            else
            {
                index = p - filename;
            }
            
            // Filename
            char filename[index];
            for(i = 0; i < index; i++)
                filename[i] = filename[i];
            filename[index] = '\0';
            
            fp = fopen(filename, "a");    // create/rewrite file
        }
        else
        {
            if(finalPacketReceived)
            {
                fclose(fp);
                break;
            }
            
            fwrite(PacketRecv.data, 1, strlen(PacketRecv.data), fp);
        }
    }
    
    fclose(fp);
    return 0;
}