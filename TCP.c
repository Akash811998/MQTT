/*
 * TCP.IP_ADD_LENGTH
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "tm4c123gh6pm.h"
#include "TCP.h"

#define TCP_PROTOCOL 0x06
#define DONT_FRAGMENT 0x4000

uint8_t curTcpState=TCP_IDLE;
extern uint32_t sequence;
extern uint32_t acknowledge;

void setTcpState(uint8_t state)
{
    curTcpState=state;
}
uint8_t getTcpState()
{
    return curTcpState;
}

void sendTcpMsg(etherHeader *ether,uint16_t flag, uint16_t dest_port,uint8_t dest_ip[4],uint8_t dest_mac[6],uint16_t length)
{
    uint8_t i=0;
    uint8_t mac[6]={0,0,0,0,0,0};
    etherGetMacAddress(mac); //get MAC address
    for (i = 0; i < HW_ADD_LENGTH; i++) //then fill up the ethernet dest and source address
    {
        ether->destAddress[i] = dest_mac[i];// set this up with the noaa mac address
        ether->sourceAddress[i] = mac[i];
    }
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize = 0x45;
    ip->typeOfService = 0;
    ip->id = htons(0x0000);
    ip->flagsAndOffset = DONT_FRAGMENT;
    ip->ttl = 128;
    ip->protocol = TCP_PROTOCOL;   //protocol value of tcp is 6
    ip->headerChecksum = 0;
    ip->length=htons(IP_HEADER_SIZE + TCP_HEADER_SIZE + length);
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        uint8_t temp[4];
        etherGetIpAddress(temp);
        ip->sourceIp[i]=temp[i];
        ip->destIp[i]=dest_ip[i];
    }
    etherCalcIpChecksum(ip);
    tcpHeader* tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));

    tcp->sourcePort=source_port;
    tcp->destPort=htons(dest_port);
    tcp->sequenceNumber=htonl();
    tcp->acknowledgementNumber=htonl();

    tcp->windowSize=htons();// use a random value for this
    tcp->checksum=0;
    tcp->urgentPointer=0;

    if(flag==TCP_SYN)
    {
        //defining the maximum segment size(window size) telling the receiver that the the device is only capacble of accepting 1220 extra bytes excluding IP, ether and TCP header
        tcp->data[j++]=0x02;  //kind
        tcp->data[j++]=0x04;  //length in number of bytes
        tcp->data[j++]=0x04;  // the next two bytes represent 1220 number in hex
        tcp->data[j++]=0xC4;
    }

    while(len>0)
    {
        tcp

    }

    //calculate the checksums for the packet

    etherCalcTcpChecksum(tcp);
    etherPutPacket(ether,ETHER_HEADER_SIZE+((ip->revSize & 0xF) * 4) + TCP_HEADER_LENGTH + j);
}

bool etherIsTcp(etherHeader* ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    uint32_t sum = 0;
    bool ok;
    uint16_t tmp16;
    ok = (ip->protocol == 6);
    if (ok)
    {
        // 32-bit sum over pseudo-header
        ok&=etherCheckTcpCheckSum(ether);
    }
    return ok;
}

bool isTcpAcknowledgment(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize=69; //which is 0x45 in hexa
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    bool ok;

    ok=((htons(tcp->offsetFields)) & TCP_ACK); //checking if the ack bit is set
    if(ok)
    {
        ok&=(ether->destAddress[0]==2);
        ok&=(ether->destAddress[1]==3);
        ok&=(ether->destAddress[2]==4);
        ok&=(ether->destAddress[3]==5);
        ok&=(ether->destAddress[4]==6);
        ok&=(ether->destAddress[5]==117);

        //ok&=(((htons(tcp->offsetFields)) & 0x0FFF)==TCP_SYNACK) || (((htons(tcp->offsetFields)) & 0x0FFF)==TCP_PUSH_ACK) || (((htons(tcp->offsetFields)) & 0x0FFF)==TCP_FIN_ACK) ||  (((htons(tcp->offsetFields)) & 0x0FFF)==TCP_REST_ACK); //checking all the possibilities for ACK

        //ok&=(tcp->destPort==htons(23));
    }

    return ok;
}
bool isTcpSyn(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize=69; //which is 0x45 in hexa
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    bool ok;
    ok=(htons(tcp->offsetFields) & TCP_SYN);
    if(ok)
    {
        ok&=(ether->destAddress[0]==2);
        ok&=(ether->destAddress[1]==3);
        ok&=(ether->destAddress[2]==4);
        ok&=(ether->destAddress[3]==5);
        ok&=(ether->destAddress[4]==6);
        ok&=(ether->destAddress[5]==117);
    }

    return ok;
}
bool isTcpSynAck(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize=69; //which is 0x45 in hexa
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    bool ok;
    ok=(htons(tcp->offsetFields) & TCP_SYNACK);
    if(ok)
    {
        ok&=(ether->destAddress[0]==2);
        ok&=(ether->destAddress[1]==3);
        ok&=(ether->destAddress[2]==4);
        ok&=(ether->destAddress[3]==5);
        ok&=(ether->destAddress[4]==6);
        ok&=(ether->destAddress[5]==117);
    }

    return ok;
}
bool isTcpFin(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize=69; //which is 0x45 in hexa
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    bool ok;
    ok=(htons(tcp->offsetFields) & TCP_FIN);
    if(ok)
    {
        ok&=(ether->destAddress[0]==2);
        ok&=(ether->destAddress[1]==3);
        ok&=(ether->destAddress[2]==4);
        ok&=(ether->destAddress[3]==5);
        ok&=(ether->destAddress[4]==6);
        ok&=(ether->destAddress[5]==117);
    }

    return ok;
}
bool isTcpReset(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize=69; //which is 0x45 in hexa
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    bool ok;
    ok=(htons(tcp->offsetFields) & TCP_RESET);
    if(ok)
    {
        ok&=(ether->destAddress[0]==2);
        ok&=(ether->destAddress[1]==3);
        ok&=(ether->destAddress[2]==4);
        ok&=(ether->destAddress[3]==5);
        ok&=(ether->destAddress[4]==6);
        ok&=(ether->destAddress[5]==117);
    }

    return ok;
}

void handleTcpSegment(etherHeader *ether)
{
    if()
    {

    }
    else if(isTcpAcknowledgment(ether))
    {
        ipHeader* ip = (ipHeader*)ether->data;
        tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
        uint16_t offset=(htons(tcp->offsetFields)) & 0x0FFF;
        if(offset==TCP_SYNACK && curTcpState==TCP_RECEIVE_SYN_ACK)
        {
            sequence=tcp->acknowledgementNumber;
            acknowledge=htonl(tcp->sequenceNumber)+1;
            curTcpState=TCP_SEND_ACK;
        }

    }
}
void etherCalcTcpChecksum(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    etherCalcIpChecksum(ip);

    //calculate TCP checksum
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    uint32_t sum = 0;
    uint16_t tmp16=0;
    tcp->checksum = 0;
    uint16_t tcpHeaderLength=htons(ip->length)-IP_HEADER_LENGTH;
    etherSumWords(ip->sourceIp,8,&sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    etherSumWords(tcp,tcpHeaderLength, &sum);
    tmp16=htons(tcpHeaderLength);
    etherSumWords(&tmp16,2, &sum);

    tcp->checksum=getEtherChecksum(sum);

}


bool etherCheckTcpCheckSum(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    etherCalcIpChecksum(ip);

    //calculate TCP checksum
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    uint32_t sum = 0;
    uint16_t tmp16=0;
    tcp->checksum = 0;
    uint16_t tcpHeaderLength=htons(ip->length)-IP_HEADER_LENGTH;
    etherSumWords(ip->sourceIp,8,&sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    etherSumWords(tcp,tcpHeaderLength, &sum);
    tmp16=htons(tcpHeaderLength);
    etherSumWords(&tmp16,2, &sum);

    return (getEtherChecksum(sum)==0);
}

void buildTcpHeader(etherHeader *ether,uint8_t destMac[6],uint8_t dest_Ip[4],uint16_t sourcePort,uint16_t destinationPort,uint16_t flag,uint16_t length)
{
    uint16_t len=length;
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    uint8_t mac[6]={0,0,0,0,0,0};
    etherGetMacAddress(mac);
    uint8_t i=0,j=0;
    for(i=0;i<6;i++)
    {
        ether->destAddress[i]=destMac[i];
        ether->sourceAddress[i]=mac[i];
    }
    ether->frameType=htons(0x0800);
    ip->revSize = 0x45; //size is 5 hence 5*4, the size of the IP is 20 bytes
    ip->typeOfService = 0;
    ip->id = 0;
    ip->flagsAndOffset = DONT_FRAGMENT;
    ip->ttl = 128;
    ip->protocol =TCP_PROTOCOL;
    ip->headerChecksum = htons(0x0000);
    ip->length= htons(IP_HEADER_SIZE + TCP_HEADER_SIZE + length);

    uint8_t ip[4]={0,0,0,0};
    etherGetIpAddress(ip);
    for(i=0;i<4;i++)
    {
        ip->destIp[i]=dest_Ip[i];
        ip->sourceIp[i]=ip[i];
    }
    etherCalcIpChecksum(ip);
    tcp->sourcePort=htons(sourcePort);
    tcp->destPort=htons(destinationPort);
    tcp->sequenceNumber=htonl(sequence);
    tcp->acknowledgementNumber=htonl(acknowledge);
    uint16_t temp=0;
    temp=(5<<12) | flag;  // where 5<<12 is the length of the header shifted  towards the header length side and the flag value is anded at the LSB
    tcp->offsetFields=htons(temp);
    tcp->windowSize=htons(1220);
    tcp->checksum=htons(0x0000);
    tcp->urgentPointer=htons(0x0000);
    etherCalcTcpChecksum(ether);
}

bool isTcpEnabled()
{
    return getTcpState()!=TCP_IDLE;
}


