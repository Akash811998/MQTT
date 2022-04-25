/*
 * MQTT.c
 * AKASH VIRENDRA, SOWMYA SRINIVASA
 */

#define MQTT_CONNECT_REMAINING_LENGTH           0x0C        //PROTOCOL NAME LENGTH, PROTOCOL NAME, VERSION, CONNECT FLAGS, KEEP ALIVE AND CLIENT ID LENGTH
#define MQTT_SUBSCRIBE_PACKET_LENGTH            0x05        //PACKET_ID + PACKET_LENGTH +REQUESTED_QOS
#define MQTT_UNSUBSCRIBE_PACKET_LENGTH          0x04
#define MQTT_HEADER_FLAG_AND_REMAINING_LENGTH   0x02        //HEADER FLAG AND REMAINING LENGTH
#define MQTT_VERSION_V311                       0x04

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "mqtt.h"
#include "TCP.h"
#include "input.h"


uint8_t mqttIpAddress[IP_ADD_LENGTH] = {0,0,0,0};
uint8_t mqttMacAddress[HW_ADD_LENGTH] = {0,0,0,0,0,0};
bool mqttlive=false;

void mqttSendConnect(etherHeader *ether,char* client_name,uint8_t connectFlag)
{
    uint8_t j=0,clientIdLength=0,i=0;
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqttVariableHeader *mqttVariable=(mqttVariableHeader*)mqtt->data;
    mqttClientHeader *client=(mqttClientHeader*)mqttVariable->data;

    mqtt->control_header=(uint8_t)CONNECT;  // where 1 is for control field for connect and 0 for control field

    mqttVariable->protocol_name_length=htons(0x0004);
    mqttVariable->protocol_name[0]='M';
    mqttVariable->protocol_name[1]='Q';
    mqttVariable->protocol_name[2]='T';
    mqttVariable->protocol_name[3]='T';
    mqttVariable->protocol_level=MQTT_VERSION_V311; //mqtt version

    mqttVariable->connect_flags=connectFlag;            //clean session
    mqttVariable->keep_alive=htons(0xFFFF);              //keep it alive for 65535 seconds
    clientIdLength=strLength(client_name);
    client->client_id_length=htons(clientIdLength);
    mqtt->remaining_length=clientIdLength+MQTT_CONNECT_REMAINING_LENGTH;
    while(clientIdLength>0)
    {
        client->clientName[j++]=client_name[i++];
        clientIdLength--;
    }
    buildTcpHeader(ether,1883,mqttIpAddress,mqttMacAddress,TCP_PUSH_ACK,mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    etherPutPacket(ether,ETHER_HEADER_SIZE+((ip->revSize & 0xF) * 4) + TCP_HEADER_LENGTH + mqtt->remaining_length+ MQTT_HEADER_FLAG_AND_REMAINING_LENGTH); //0x02 for mqtt packet and remaining length
    updateSeqNumber(mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
}
void mqttSendPublish(etherHeader *ether,uint8_t qos,char* topicName,char* message)
{
    uint8_t topic_length=0,message_length=0,j=0,i=0;
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqtt->remaining_length=0;
    if(qos==0)
        mqtt->control_header=(uint8_t)PUBLISH_QOS0;  //where 3 is for control field for publish and 0 for control field for qos0
    else if(qos==1)
        mqtt->control_header=(uint8_t)PUBLISH_QOS1;  //where 1 is for control field for publish and 2 for control field for qos1 0010
    else if(qos==2)
        mqtt->control_header=(uint8_t)PUBLISH_QOS2;  //where 1 is for control field for publish and 4 for control field for qos2 0100

    //store the topic name
    topic_length=strLength(topicName);
    mqtt->remaining_length+=topic_length;
    while(topic_length>0)
    {
        mqtt->data[j++]=topicName[i++];
        topic_length--;
    }
    //store the message
    i=0;
    message_length=strLength(message);
    mqtt->remaining_length+=message_length;
    while(message_length>0)
    {
        mqtt->data[j++]=message[i++];
        message_length--;
    }
    buildTcpHeader(ether,1883,mqttIpAddress,mqttMacAddress,TCP_PUSH_ACK,mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    etherPutPacket(ether,ETHER_HEADER_SIZE+((ip->revSize & 0xF) * 4) + TCP_HEADER_LENGTH + mqtt->remaining_length+ MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    updateSeqNumber(mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
}
void mqttSendSubscribe(etherHeader *ether,char* topicName,uint16_t subscribe_Id)
{
    uint8_t topic_length=0,i=0,j=0;
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqttSubscribeHeader *subscribe=(mqttSubscribeHeader*)mqtt->data;
    mqtt->control_header=(uint8_t)SUBSCRIBE;  //where 8 is for control field for subscribe

    subscribe->packet_id=htons(subscribe_Id);
    topic_length=strLength(topicName);
    mqtt->remaining_length=MQTT_SUBSCRIBE_PACKET_LENGTH+topic_length;
    subscribe->topic_length=htons(topic_length);
    while(topic_length>0)
    {
        subscribe->topic_name[j++]=topicName[i++];
        topic_length--;
    }
    subscribe->topic_name[j]=0x00;



    buildTcpHeader(ether,1883,mqttIpAddress,mqttMacAddress,TCP_PUSH_ACK,mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    etherPutPacket(ether,ETHER_HEADER_SIZE + ((ip->revSize & 0xF) * 4) + TCP_HEADER_LENGTH + mqtt->remaining_length + MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    updateSeqNumber(mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
}
void mqttSendUnsubscribe(etherHeader *ether,char* topicName,uint16_t subscribe_Id)
{
    uint8_t topic_length=0,i=0,j=0;
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqttUnsubscribeHeader *unsubscribe=(mqttUnsubscribeHeader*)mqtt->data;
    mqtt->control_header=(uint8_t)UNSUBSCRIBE; //where A is for control field for unsubscribe

    unsubscribe->packet_id=htons(subscribe_Id);
    topic_length=strLength(topicName);
    mqtt->remaining_length=MQTT_UNSUBSCRIBE_PACKET_LENGTH+topic_length;
    unsubscribe->topic_length=htons(topic_length);
    while(topic_length>0)
    {
        unsubscribe->topic_name[j++]=topicName[i++];
        topic_length--;
    }


    buildTcpHeader(ether,1883,mqttIpAddress,mqttMacAddress,TCP_PUSH_ACK,mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    etherPutPacket(ether,ETHER_HEADER_SIZE + ((ip->revSize & 0xF) * 4) + TCP_HEADER_LENGTH + mqtt->remaining_length + MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    updateSeqNumber(mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);

}
void mqttSendDisconnect(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqtt->control_header=(uint8_t)DISCONNECT; //where E is for control field for disconnect

    mqtt->remaining_length=0x00;
    buildTcpHeader(ether,1883,mqttIpAddress,mqttMacAddress,TCP_PUSH_ACK,mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    etherPutPacket(ether,ETHER_HEADER_SIZE + ((ip->revSize & 0xF) * 4) + TCP_HEADER_LENGTH + mqtt->remaining_length + MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    updateSeqNumber(mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);

}
void mqttSendPing(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader *tcp=(tcpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    mqttHeader *mqtt=(mqttHeader*)tcp->data;
    mqtt->control_header=(uint8_t)PINGREQ; //where C is for control field for ping

    mqtt->remaining_length=0x00;
    buildTcpHeader(ether,1883,mqttIpAddress,mqttMacAddress,TCP_PUSH_ACK,mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    etherPutPacket(ether,ETHER_HEADER_SIZE + ((ip->revSize & 0xF) * 4) + TCP_HEADER_LENGTH + mqtt->remaining_length + MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
    updateSeqNumber(mqtt->remaining_length+MQTT_HEADER_FLAG_AND_REMAINING_LENGTH);
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
    ok&=(mqtt->data[0]==0x00); //acknowledge flags should be 0x00
    ok&=(mqtt->data[1]==0x00); //return code is 0x00 i.e the connection is accepted by the broker

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
bool isMqttPublishPacket(etherHeader *ether)  //if the broker sends the value of the subscribed topic
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
    ok&=(mqtt->control_header==PUBLISH_QOS0);

    return ok;
}

bool isMqttPingAck(etherHeader *ether)
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
    ok&=(mqtt->control_header==PINGRESP);

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
void etherSetMqttMacAddress(const uint8_t mac[6])
{
    uint8_t i;
    for (i = 0; i < 4; i++)
        mqttMacAddress[i] = mac[i];
}
void etherGetMqttMacAddress(uint8_t mac[6])
{
    uint8_t i;
    for (i = 0; i < 4; i++)
        mac[i] = mqttMacAddress[i];
}
bool isMqttLive()
{
    return mqttlive;
}

