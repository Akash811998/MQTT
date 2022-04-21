




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
    mqttVariableHeader *mqttVariable=(mqttVariableHeader*)mqtt->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);
    mqtt->control_header=CONNECT;  // where 1 is for control field for connect and 0 for control field


}
void mqttSendPublish(etherHeader *ether,uint8_t qos)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqttVariableHeader *mqttVariable=(mqttVariableHeader*)mqtt->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);
    if(qos==0)
        mqtt->control_header=PUBLISH_QOS0;  //where 3 is for control field for publish and 0 for control field for qos0
    else if(qos==1)
        mqtt->control_header=PUBLISH_QOS1;  //where 1 is for control field for publish and 2 for control field for qos1 0010
    else if(qos==2)
        mqtt->control_header=PUBLISH_QOS2;  //where 1 is for control field for publish and 4 for control field for qos2 0100
}
void mqttSendSubscribe(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqttVariableHeader *mqttVariable=(mqttVariableHeader*)mqtt->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);
    mqtt->control_header=SUBSCRIBE;  //where 8 is for control field for subscribe
}
void mqttSendUnsubscribe(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqttVariableHeader *mqttVariable=(mqttVariableHeader*)mqtt->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);
    mqtt->control_header=UNSUBSCRIBE; //where A is for control field for unsubscribe
}
void mqttSendDisconnect(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqttVariableHeader *mqttVariable=(mqttVariableHeader*)mqtt->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);
    mqtt->control_header=DISCONNECT; //where E is for control field for disconnect

}
void mqttSendPing(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqttVariableHeader *mqttVariable=(mqttVariableHeader*)mqtt->data;
    buildTcpHeader(etherHeader *ether,sourcePort,1883,TCP_PUSH_ACK);
    mqtt->control_header=PINGREQ; //where C is for control field for ping
}
bool isMqttConnectAck(etherHeader *ether)
{
    uint8_t temp[6];
    uint8_t i=0;
    bool ok=1;
    etherGetMacAddress(temp);
    for(i=0;i<6;i++)
    {
        ok&=(temp[i]==ether->destAddress[i]);
    }

    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    ok&=(mqtt->control_header==CONNACK);

    return ok;

}
bool isMqttSubscribeAck(etherHeader *ether)
{
    uint8_t temp[6];
    uint8_t i=0;
    bool ok=1;
    etherGetMacAddress(temp);
    for(i=0;i<6;i++)
    {
        ok&=(temp[i]==ether->destAddress[i]);
    }

    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    ok&=(mqtt->control_header==SUBACK);

    return ok;
}

bool isMqttPublishAck(etherHeader *ether)
{
    uint8_t temp[6];
    uint8_t i=0;
    bool ok=1;
    etherGetMacAddress(temp);
    for(i=0;i<6;i++)
    {
        ok&=(temp[i]==ether->destAddress[i]);
    }

    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    ok&=(mqtt->control_header==PUBACK);

    return ok;
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
