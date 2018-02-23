## FTv2

Packets structure:

```cpp
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
```

UDP with ACK flags and checksum per packet :)

Datafloow diagram:
![picture](docs/img/TFv2_Diagram.png)