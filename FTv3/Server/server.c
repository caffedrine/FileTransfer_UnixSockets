#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <elf.h>

#define UDP_BUFF_LEN    8          // Max length of buffer for UDP data
#define STRG_BUFF_LEN   (UDP_BUFF_LEN * 1)         // Buffer used to store data before processing. Must be greater than STRG BUFF LEN and multiply of UDP_BUFF_LEN
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
    strcpy(packet.data, "12345679");
    
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
    char strg_buff[STRG_BUFF_LEN];                          // buffer used to store data received before processing
    short finalPacketReceived = 0;                                  // set this flag when final packet wil length 0 was received
    FILE *fp = NULL;
    
    //create a UDP socket
    if((socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("Can't create socket...");
    }
    
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
    memset(strg_buff, '\0', STRG_BUFF_LEN);
    
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
        printf("------------\nWaiting for ACK0..."); fflush(stdout);
        if(recvfrom(socketfd, &PacketRecv, sizeof(PacketRecv), 0, (struct sockaddr *) &si_other,
                    &slen) == -1)
        {
            // if there was an error on receiving, return 0;
            printf("FAILED\n");
            return 0;
        }
        printf("OK: %d\n", PacketRecv.Header.seq_ack);
        
        
        // Wait for data and data length
        if(recvfrom(socketfd, &PacketRecv, sizeof(PacketRecv), 0, (struct sockaddr *) &si_other,
                    &slen) == -1)
        {
            printf("Failed to receive data length...\n");
            continue;
        }
        printf("Got %d bytes: %s\n", PacketRecv.Header.len, PacketRecv.data);
        
        /*
        // Read data only if length > 0
        if(Packet.Header.len > 0)
        {
            // Read data from socket
            if(recvfrom(socketfd, &Packet.data, Packet.Header.len, 0, (struct sockaddr *) &si_other, &slen) == -1)
            {
                printf("Failed to receive data length...\n");
                continue;
            }
            
            // If ACK flag is set to 1, this packet is retransmitted!
            
            // How to make sure that the packet is not the same resend by client?
            // Client will keep ack as 1 when a packet is resend
            strcat(strg_buff, Packet.data);
        }
        else    // if data length < 0 this mean that the file packets was done - write the rest to the file and then break
        {
            finalPacketReceived = 1;
        }
        
        // Process received data
        if(strlen(strg_buff) >= STRG_BUFF_LEN - (UDP_BUFF_LEN) || finalPacketReceived == 1)    // if the buffer is full or the last data was all
        {
            // Data expected is "filename\0content<0_len_packet_marking_end>"
            finalPacketReceived?printf("Final packet received! Program will close!\n"):printf("Data received: %s\n", strg_buff);
            
            // Write data to file or append
            if(fp == NULL)  // file pointer is null only if we didn't get the filename yet
            {
                // get file name from buffer
                unsigned long index;
                const char *p = strchr(strg_buff, '\0');
                if(!p)  // this mean that filename length is higher than buffer
                {
                    printf("Filename length biger than strg_buffer!");
                    exit(1);
                }
                else
                {
                    index = p - strg_buff;
                }
                
                // Filename
                char filename[index];
                for(i = 0; i < index; i++)
                    filename[i] = strg_buff[i];
                filename[index] = '\0';
                
                fp = fopen(filename, "a");    // create/rewrite file
            }
            else
            {
                fwrite(strg_buff, 1, strlen(strg_buff), fp);
            }
            
            // make sure storage buffer is clean for a new usage
            memset(strg_buff, '\0', STRG_BUFF_LEN);
        }
    
        // Send back ACK1 to inform client that we received some data
        ++Packet.Header.seq_ack;
        uint32_t converted_ack = htonl(Packet.Header.seq_ack);
        if(sendto(socketfd, &converted_ack, sizeof(converted_ack), 0, (struct sockaddr *) &si_other, slen) == -1)
        {
            printf("Failed to send ACK1 response...\n");
            continue;
        }
        printf("ACK1 with value %d was send to client...\n", Packet.Header.seq_ack);
    
        // If final empty packet was received
        if(finalPacketReceived)
            break;
        
        // Now send back checksum to make sure this is the date client sends to us
        Packet.Header.cksum = getChecksum(Packet);
        uint32_t checksum_converted = htonl(
                Packet.Header.cksum);   // convert checksum ina  specific format to be send via network
        if(sendto(socketfd, &checksum_converted, sizeof(checksum_converted), 0, (struct sockaddr *) &si_other,
                  slen) == -1)
        {
            printf("Failed to send checksum back to client...\n");
            continue;
        }
        printf("Checksum with value %d was send to client...\n", Packet.Header.cksum);
         */
    }
    
    fclose(fp);
    return 0;
}