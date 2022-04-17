/*
 * TCP.IP_ADD_LENGTH
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "tm4c123gh6pm.h"
#include "TCP.h"

uint8_t curTcpState;
extern uint32_t sequence;
extern uint32_t acknowledge;

void setTcpstate(uint8_t state)
{
    curTcpState=state;
}
uint8_t getTcpstate()
{
    return curTcpState;
}

void sendTcpMsg(etherHeader *ether,uint16_t flag, uint16_t dest_port)
{
    uint8_t i=0;
    uint8_t mac[4]={0,0,0,0};
    etherGetMacAddress(mac); //get MAC address
    for (i = 0; i < HW_ADD_LENGTH; i++) //then fill up the ethernet dest and source address
    {
        ether->destAddress[i] = 0xFF;// set this up with the noaa mac address
        ether->sourceAddress[i] = mac[i];
    }
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize = 0x45;
    ip->typeOfService = 0;
    ip->id = 0;
    ip->flagsAndOffset = 0;
    ip->ttl = 128;
    ip->protocol = 6;   //protocol value of tcp is 6
    ip->headerChecksum = 0;
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        uint8_t temp[4];
        etherGetIpAddress(temp);
        ip->sourceIp=temp[i];
        ip->destIp=;
    }

    tcpHeader* tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));

    //generate a random port for the source port
    //the ephermal ports ranges from 49152 to 65535
    srand(time(0)); //this uses the time at this  instant as the seed value
    tcp->sourcePort=htons(49152+ (rand()%(16384-1)));  //assigns an ephermal port as the soruce port
    tcp->destPort=htons(dest_port);
    tcp->sequenceNumber=htonl();
    tcp->acknowledgementNumber=htonl();
    uint16_t temp=0;
    temp=(5<<12) | flag;  // where 5<<12 is the length of the header shifted  towards the header length side and the flag value is anded at the LSB
    tcp->offsetFields=temp;
    tcp->windowSize=htons();// use a random value for this
    tcp->checksum=0;
    tcp->urgentPointer=0;
    tcp->


    //calculate the checksums for the packet
    etherCalcIpChecksum(ip);
    etherCalcTcpChecksum();
}



bool isTcpAcknowledgment(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize=69; //which is 0x45 in hexa
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    bool ok;

    if(etherIsTcp(ether))
    {
        ok=(ether->destAddress[0]==2);
        ok&=(ether->destAddress[1]==3);
        ok&=(ether->destAddress[2]==4);
        ok&=(ether->destAddress[3]==5);
        ok&=(ether->destAddress[4]==6);
        ok&=(ether->destAddress[5]==117);

        //ok&=(((htons(tcp->offsetFields)) & 0x0FFF)==TCP_SYNACK) || (((htons(tcp->offsetFields)) & 0x0FFF)==TCP_PUSH_ACK) || (((htons(tcp->offsetFields)) & 0x0FFF)==TCP_FIN_ACK) ||  (((htons(tcp->offsetFields)) & 0x0FFF)==TCP_REST_ACK); //checking all the possibilities for ACK
        ok&= ((htons(tcp->offsetFields)) & 0x0010; //checking if the ack bit is set
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

    if(etherIsTcp(ether))
    {
        ok=(ether->destAddress[0]==2);
        ok&=(ether->destAddress[1]==3);
        ok&=(ether->destAddress[2]==4);
        ok&=(ether->destAddress[3]==5);
        ok&=(ether->destAddress[4]==6);
        ok&=(ether->destAddress[5]==117);

        ok&=((tcp->offsetFields) & 0x0002);
        //ok&=(tcp->destPort==htons(23));
    }

    return ok;
}

void handleTcpSegment(etherHeader *ether)
{
    if(isTcpAcknowledgment())
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

