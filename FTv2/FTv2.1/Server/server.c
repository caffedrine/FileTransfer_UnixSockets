#include<stdio.h>   //printf
#include<string.h>  //memset
#include<stdlib.h>  //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <elf.h>
#include <sys/time.h>
#include <time.h>

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
    if(strlen(packet.data) == 0)
        return 0;
    
    // This is used to test the program in case of cksum fails - it provides random cksum fails
//    if( (rand() % 21) < 10 )    // if a random number from 0-20 is lower than 10 then return a pseudo-checksum
//        return 123;
    
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

int main(void)
{
    struct sockaddr_in si_me, si_other;
    int socketfd, i;
    socklen_t slen = sizeof(si_other);
    short finalPacketReceivedFlag = 0;      // set this flag when final packet wil length 0 was received
    FILE *fp = NULL;
    char outputFilename[64] = "";
    
    // Used to generate random checksum validation
    srand((uint32_t)time(NULL));   // should only be called once
    
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
    strcat(outputFilename, "output.txt\0"); // or argv
    
    //keep listening for data
    while(1)
    {
        printf("--------------\nWaiting for clients/packets...\n");
        
        // Wait for packets
        if(recvfrom(socketfd, &PacketRecv, sizeof(PacketRecv), 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            printf("Failed to receive a packet...\n");
            continue;
        }
        printf("Got %d bytes: '%s', cksum: %d, seq_id: %d\n", PacketRecv.Header.len, PacketRecv.data, PacketRecv.Header.cksum, PacketRecv.Header.seq_ack);
        
        // Check if it is the last packet - last one should have data field empty
        if(PacketRecv.Header.len > 0)
        {
            printf("Checksum test: ");
            if(getChecksum(PacketRecv) == PacketRecv.Header.cksum)
            {
                printf("OK\n");
                PacketSend.Header.seq_ack = 1;
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
            PacketSend.Header.seq_ack = 1;
            PacketSend.Header.len = 0;
            if(sendto(socketfd, &PacketSend, sizeof(PacketSend), 0, (struct sockaddr *) &si_other, slen) == -1)
            {
                printf("Failed to send ACK1 response...\n");
                continue;
            }
            printf("ACK1 with value %d was send back..\n", PacketSend.Header.seq_ack);
            
            finalPacketReceivedFlag = 1;
        }
        
        // Write data to file or append
        if(fp == NULL && !finalPacketReceivedFlag)  // file pointer is null only if we didn't get the filename yet
        {
            fp = fopen(outputFilename, "w");
            fwrite(PacketRecv.data, 1, strlen(PacketRecv.data), fp);
            
            
            /* If filename is send at the beginning, use this code instead*/
            /*
            // Concat data until filename is complete:
            strcat(outputFilename, PacketRecv.data);  // Only when filename is send via UDP
            
            unsigned long index;
            const char *p = strchr(outputFilename, '\0');
            if(!p)                              // this mean that filename length is higher than buffer
            {
                printf("Filename too long!");
                exit(1);
            }
            else
            {
                index = p - outputFilename;
            }
            
            // Filename
            for(i = 0; i < index; i++)
                outputFilename[i] = outputFilename[i];
            outputFilename[index] = '\0';
            
            fp = fopen(outputFilename, "a");    // create/rewrite file
             */
        }
        else
        {
            fwrite(PacketRecv.data, 1, strlen(PacketRecv.data), fp);
            memset(PacketRecv.data, '\0', strlen(PacketRecv.data));
        }
    
        if(finalPacketReceivedFlag)
        {
            fclose(fp);
            break;
        }
    }
    
    return 0;
}