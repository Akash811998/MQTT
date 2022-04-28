/*
 * TCP.IP_ADD_LENGTH
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "tm4c123gh6pm.h"
#include "TCP.h"
#include "eth0.h"
#include "input.h"
#include "mqtt.h"

#define TCP_PROTOCOL        0x06
#define DONT_FRAGMENT       0x4000
#define OPTIONS_FOR_SYNC    0x04

uint8_t curTcpState=TCP_IDLE;
uint32_t sequence=0;
uint32_t acknowledge=0;
extern uint16_t source_port;
extern bool access_html_page;
uint32_t sequence_mqtt=0;
uint32_t acknowledge_mqtt=0;

void setTcpState(uint8_t state)
{
    curTcpState=state;
}
uint8_t getTcpState()
{
    return curTcpState;
}
void sendTcpMsg(etherHeader *ether,uint16_t flag, uint16_t dest_port,uint8_t dest_ip[4],uint8_t dest_mac[6],char *html)
{
    uint8_t i=0,j=0,k=0;
    uint8_t mac[6]={0,0,0,0,0,0};
    etherGetMacAddress(mac); //get MAC address
    for (i = 0; i < HW_ADD_LENGTH; i++) //then fill up the ethernet dest and source address
    {
        ether->destAddress[i] = dest_mac[i];// set this up with the noaa mac address
        ether->sourceAddress[i] = mac[i];
    }
    ether->frameType=htons(0x0800);
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize = 0x45;
    ip->typeOfService = 0;
    ip->id = htons(0x0000);
    ip->flagsAndOffset = htons(DONT_FRAGMENT);
    ip->ttl = 128;
    ip->protocol = TCP_PROTOCOL;   //protocol value of tcp is 6
    ip->headerChecksum = 0;
    uint8_t length=strLength(html);
    if(flag==TCP_SYN)
        ip->length=htons(TCP_HEADER_LENGTH + IP_HEADER_LENGTH + OPTIONS_FOR_SYNC);
    else
        ip->length=htons(TCP_HEADER_LENGTH + IP_HEADER_LENGTH + length);
    uint8_t temp[4];
    etherGetIpAddress(temp);
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {

        ip->sourceIp[i]=temp[i];
        ip->destIp[i]=dest_ip[i];
    }
    etherCalcIpChecksum(ip);
    tcpHeader* tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));

    tcp->sourcePort=source_port;  //htons is done when the value of the source port was found
    tcp->destPort=htons(dest_port);
    tcp->sequenceNumber=htonl(sequence);
    tcp->acknowledgementNumber=htonl(acknowledge);
    uint16_t temp1=0;
    if(flag==TCP_SYN)
        temp1=(0x6000 & 0xF000) | flag;  // where 5<<12 is the length of the header shifted  towards the header length side and the flag value is anded at the LSB
    else
        temp1=(0x5000 & 0xF000) | flag; // where 5<<12 is the length of the header shifted  towards the header length side and the flag value is anded at the LSB
    tcp->offsetFields=htons(temp1);
    tcp->windowSize=htons(0x0578);// used a random value for this
    tcp->checksum=0x0000;
    tcp->urgentPointer=0;

    if(flag==TCP_SYN)
    {
        //defining the maximum segment size(window size) telling the receiver that the the device is only capacble of accepting 1220 extra bytes excluding IP, ether and TCP header
        tcp->data[j++]=0x02;  //kind
        tcp->data[j++]=0x04;  //length in number of bytes
        tcp->data[j++]=0x05;  // the next two bytes represent 1400 number in hex
        tcp->data[j++]=0x78;
    }
    if(flag==TCP_PUSH_ACK)
    {
        for(k=0;k<length;k++)
            tcp->data[k]=html[k];
    }
    //calculate the checksums for the packet

    etherCalcTcpChecksum(ether);
    if(length>0)
        sequence+=length;
    else
        sequence+=1;
    etherPutPacket(ether,ETHER_HEADER_SIZE+((ip->revSize & 0xF) * 4) + TCP_HEADER_LENGTH + j + length);
}

bool etherIsTcp(etherHeader* ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
//    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
//    uint32_t sum = 0;
    bool ok;
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
    ipHeader *ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    if((curTcpState==TCP_RECEIVE_SYN_ACK || curTcpState==MQTT_SYN_ACK) && isTcpSynAck(ether))
    {
        if(access_html_page)
        {
            acknowledge=htonl(tcp->sequenceNumber)+0x01;
            curTcpState=TCP_SEND_ACK;
        }
        else
        {
            acknowledge_mqtt=htonl(tcp->sequenceNumber)+0x01;
            curTcpState=MQTT_ACK;
        }
    }
    else if(isTcpAcknowledgment(ether) && curTcpState==TCP_WAIT_FOR_HTML && access_html_page)
    {
        acknowledge=htonl(tcp->sequenceNumber)+htons(ip->length)-IP_HEADER_LENGTH-((htons(tcp->offsetFields) & 0xF000)*4);
        parseHtmlString(ether);
        access_html_page=false;
        curTcpState=MQTT_SYN;
    }
    else if(isMqttConnectAck(ether) && curTcpState==MQTT_CONNECT_ACK)
    {
        acknowledge=htonl(tcp->sequenceNumber)+htons(ip->length)-IP_HEADER_LENGTH-((htons(tcp->offsetFields) & 0xF000)*4);

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
    tmp16=htons(tcpHeaderLength);
    etherSumWords(&tmp16,2, &sum);
    etherSumWords(tcp,tcpHeaderLength, &sum);
    tcp->checksum=getEtherChecksum(sum);

}

void parseHtmlString(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
}


bool etherCheckTcpCheckSum(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    etherCalcIpChecksum(ip);

    //calculate TCP checksum
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    uint32_t sum = 0;
    uint16_t tmp16=0;
    uint16_t tcpHeaderLength=htons(ip->length)-IP_HEADER_LENGTH;
    etherSumWords(ip->sourceIp,8,&sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    etherSumWords(tcp,tcpHeaderLength, &sum);
    tmp16=htons(tcpHeaderLength);
    etherSumWords(&tmp16,2, &sum);

    return (getEtherChecksum(sum)==0);
}

void buildTcpHeader(etherHeader *ether,uint16_t destinationPort,uint8_t dest_ip[4],uint8_t dest_mac[6],uint16_t flag,uint16_t length)
{
   // uint16_t len=length;
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    uint8_t mac[6]={0,0,0,0,0,0};
    etherGetMacAddress(mac);
    uint8_t i=0;
    for(i=0;i<6;i++)
    {
        ether->destAddress[i]=dest_mac[i];
        ether->sourceAddress[i]=mac[i];
    }
    ether->frameType=htons(0x0800);
    ip->revSize = 0x45; //size is 5 hence 5*4, the size of the IP is 20 bytes
    ip->typeOfService = 0;
    ip->id = htons(0x0000);
    ip->flagsAndOffset = htons(DONT_FRAGMENT);
    ip->ttl = 128;
    ip->protocol =TCP_PROTOCOL;
    ip->headerChecksum = htons(0x0000);
    ip->length= htons(IP_HEADER_LENGTH + TCP_HEADER_LENGTH + length);

    uint8_t my_ip[4]={0,0,0,0};
    etherGetIpAddress(my_ip);
    for(i=0;i<4;i++)
    {
        ip->destIp[i]=dest_ip[i];
        ip->sourceIp[i]=my_ip[i];
    }
    etherCalcIpChecksum(ip);
    tcp->sourcePort=htons(source_port);
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
    return curTcpState!=TCP_IDLE;
}

void updateSeqNumber(uint32_t n)
{
    sequence_mqtt+=n;
}

