// ETH0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL w/ ENC28J60
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// ENC28J60 Ethernet controller on SPI0
//   MOSI (SSI0Tx) on PA5
//   MISO (SSI0Rx) on PA4
//   SCLK (SSI0Clk) on PA2
//   ~CS (SW controlled) on PA3
//   WOL on PB3
//   INT on PC6

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef ETH0_H_
#define ETH0_H_



#include <stdint.h>
#include <stdbool.h>
#define ntohs htons
#define IP_ADD_LENGTH 4
#define HW_ADD_LENGTH 6
#define ETHER_HEADER_SIZE 0x0E

// This M4F is little endian (TI hardwired it this way)
// Network byte order is big endian
// Must interpret uint16_t in reverse order
typedef struct _enc28j60Frame // 4 bytes
{                             //these are the 4 bytes that are prepended by the the ethernet chip.
                              //i.e they have nothing to do with the  standard(ethernet frame)
  uint16_t size;
  uint16_t status;
  uint8_t data[0]; //this acts as a placeholder for data in the memory, might contain a pointer to the next data frame or the ethernet header
} enc28j60Frame;

typedef struct _etherHeader // 14 bytes
{
  uint8_t destAddress[6];
  uint8_t sourceAddress[6];
  uint16_t frameType;
  uint8_t data[0]; //this is a pointer to the arp or the IP header
} etherHeader;
//if the data is sent as 0x8000 the data is received as 0x0008, hence we will be using berkley sockets model to automatically reverse the bits
typedef struct _arpPacket // 28 bytes
{
  uint16_t hardwareType;
  uint16_t protocolType;
  uint8_t hardwareSize;
  uint8_t protocolSize;
  uint16_t op;
  uint8_t sourceAddress[6];
  uint8_t sourceIp[4];
  uint8_t destAddress[6];
  uint8_t destIp[4];
} arpPacket;

typedef struct _ipHeader // 20 or more bytes
{
  uint8_t revSize;    //this includes both the version and the header length
  uint8_t typeOfService;
  uint16_t length;
  uint16_t id;
  uint16_t flagsAndOffset;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t headerChecksum;
  uint8_t sourceIp[4];
  uint8_t destIp[4];
  uint8_t data[0]; // optional bytes or udp/tcp/icmp header
} ipHeader;

typedef struct _icmpHeader // 8 bytes
{
  uint8_t type;
  uint8_t code;
  uint16_t check;
  uint16_t id;
  uint16_t seq_no;
  uint8_t data[0];
} icmpHeader;

typedef struct _udpHeader // 8 bytes
{
  uint16_t sourcePort;
  uint16_t destPort;
  uint16_t length;
  uint16_t check;
  uint8_t  data[0];
} udpHeader;

typedef struct _dhcpFrame // 240 or more bytes
{
  uint8_t op;
  uint8_t htype;
  uint8_t hlen;
  uint8_t hops;
  uint32_t  xid;
  uint16_t secs;
  uint16_t flags;
  uint8_t ciaddr[4];
  uint8_t yiaddr[4];
  uint8_t siaddr[4];
  uint8_t giaddr[4];
  uint8_t chaddr[16];
  uint8_t data[192];
  uint32_t magicCookie;
  uint8_t options[0];
} dhcpFrame;



#define ETHER_UNICAST        0x80
#define ETHER_BROADCAST      0x01
#define ETHER_MULTICAST      0x02
#define ETHER_HASHTABLE      0x04
#define ETHER_MAGICPACKET    0x08
#define ETHER_PATTERNMATCH   0x10
#define ETHER_CHECKCRC       0x20

#define ETHER_HALFDUPLEX     0x00
#define ETHER_FULLDUPLEX     0x100

#define LOBYTE(x) ((x) & 0xFF)
#define HIBYTE(x) (((x) >> 8) & 0xFF)

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void etherInit(uint16_t mode);
bool etherIsLinkUp();

bool etherIsDataAvailable();
bool etherIsOverflow();
uint16_t etherGetPacket(etherHeader *ether, uint16_t maxSize);
bool etherPutPacket(etherHeader *ether, uint16_t size);

bool etherIsIp(etherHeader *ether);
bool etherIsIpUnicast(etherHeader *ether);

bool etherIsPingRequest(etherHeader *ether);
void etherSendPingResponse(etherHeader *ether);

bool etherIsArpRequest(etherHeader *ether);
void etherSendArpResponse(etherHeader *ether);
void etherSendArpRequest(etherHeader *ether, uint8_t ipFrom[], uint8_t ipTo[]);

void etherSumWords(void* data, uint16_t sizeInBytes, uint32_t* sum);
uint16_t getEtherChecksum(uint32_t sum);


bool etherIsUdp(etherHeader *ether);
uint8_t* etherGetUdpData(etherHeader *ether);
void etherSendUdpResponse(etherHeader *ether, uint8_t* udpData, uint8_t udpSize);
bool isTcpEnabled();
void etherCalcIpChecksum(ipHeader* ip);
void etherEnableDhcpMode();
void etherDisableDhcpMode();
bool etherIsDhcpEnabled();
bool etherIsIpValid();
void etherSetIpAddress(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3);
void etherGetIpAddress(uint8_t ip[4]);
void etherSetIpGatewayAddress(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3);
void etherGetIpGatewayAddress(uint8_t ip[4]);
void etherSetIpSubnetMask(uint8_t mask0, uint8_t mask1, uint8_t mask2, uint8_t mask3);
void etherGetIpSubnetMask(uint8_t mask[4]);
void etherSetMacAddress(uint8_t mac0, uint8_t mac1, uint8_t mac2, uint8_t mac3, uint8_t mac4, uint8_t mac5);
void etherGetMacAddress(uint8_t mac[6]);
void etherSetHtmlIpAddress(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3);
void etherGetHtmlIpAddress(uint8_t ip[4]);
void etherSetDestMacAddress(uint8_t mac0, uint8_t mac1, uint8_t mac2, uint8_t mac3, uint8_t mac4, uint8_t mac5);
void etherGetDestMacAddress(uint8_t ip[4]);


uint16_t htons(uint16_t value);
uint32_t htonl(uint32_t value);

#endif
