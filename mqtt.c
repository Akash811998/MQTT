




#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "mqtt.h"
#include "TCP.h"


uint8_t mqttIpAddress[IP_ADD_LENGTH] = {0,0,0,0};

void mqttSendConnect(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);

}
void mqttSendPublish(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);
    mqtt->control_header=

}
void mqttSendSubscribe(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);
}
void mqttSendUnsubscribe(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);
}
void mqttSendDisconnect(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);

}
bool isMqttPublish(etherHeader *ether);
{

}
bool isMqttAck(etherHeader *ether);
{

}
void etherSetMqttIpAddress(const uint8_t ip[4])
{
    uint8_t i;
    for (i = 0; i < 4; i++)
        mqttIpAddress[i] = ip[i];
}
void etherGetMqttIpAddress(uint8_t ip[4])
{
    uint8_t i;
    for (i = 0; i < 4; i++)
        ip[i] = mqttIpAddress[i];
}
