/*
 * mqtt.h
 *
 *  Created on: 15-Apr-2022
 *      Author: ASUS
 */

#ifndef MQTT_H_
#define MQTT_H_

#include <stdint.h>
#include <stdbool.h>
#include "eth0.h"

#define QOS0                    0x00
#define QOS1                    0x02
#define QOS2                    0x04

//#define SEND_CONNECT            0x10
//#define SEND_PUBLISH_QOS0       0x30
//#define SEND_PUBLISH_QOS1       0x32
//#define SEND_PUBLISH_QOS2       0x34
//#define SEND_SUBSCRIBE          0x82
//#define SEND_UNSUBSCRIBE        0xA2
//#define SEND_DISCONNECT         0xE0
//#define SEND_PING               0xC0
//#define RECEIVE_CONNECT_ACK     0x20
//#define RECEIVE_SUBSCRIBE_ACK   0x90
//#define RECEIVE__ACK

typedef enum _QOS
{
    QOS=0,
    QOS=1,
    QOS=2;

}QOS;

typedef enum _mqttPacketType
{
    CONNECT = 0x10,   //COMBINATION OF CCONTROL FIELD AND CONTROL FLAGS
    CONNACK = 0x20,
    PUBLISH_QOS0 = 0x30,
    PUBLISH_QOS1 = 0x32,
    PUBLISH_QOS2 = 0x34,
    PUBACK = 0x40,
    PUBREC=0x50,
    PUBREL=0x62,
    PUBCOMP=0x70,
    SUBSCRIBE = 0x82,
    SUBACK = 0x90,
    UNSUBSCRIBE = 0xA2,
    UNSUBACK = 0xB0,
    PINGREQ = 0xC0,
    PINGRESP = 0xD0,
    DISCONNECT = 0xE0
}mqttPacketType;

//defining the connect flags
 typedef enum _connect_flags
{
    CLEAN_SESSION=0x02, //1th bit
    WILL_FLAG=0x04,    //2th bit
    WILL_QOS=0x10,    //4th bit
    WILL_RETAIN=0x20,  //5th bit
    PASSWORD_FLAG=0x40,  //6th bit
    USER_NAME_FLAG=0x80;  //7th bit
}connect_flags;


typedef struct _mqttHeader
{
    uint8_t control_header;
    uint8_t remaining_length[4];
    uint8_t data[0];
}mqttHeader;

typedef struct _mqttVariableHeader
{
    uint16_t protocol_name_length;
    char protocol_name[4];
    uint8_t protocol_level;
    uint8_t connect_flags;
    uint16_t keep_alive;
    uint8_t data[0];
}mqttVariableHeader;

void mqttSendConnect(etherHeader *ether);
void mqttSendPublish(etherHeader *ether);
void mqttSendSubscribe(etherHeader *ether);
void mqttSendUnsubscribe(etherHeader *ether);
bool isMqttConnectAck(etherHeader *ether);
bool isMqttSubscribeAck(etherHeader *ether);
bool isMqttPublishAck(etherHeader *ether);
void mqttSendPing(etherHeader *ether);
void mqttSendDisconnect(etherHeader *ether);







#endif /* MQTT_H_ */
