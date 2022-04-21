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

typedef enum _QOS
{
    QOS=0,
    QOS=1,
    QOS=2;

}QOS;

typedef enum _packetType
{
    CONNECT = 0x10,   //COMBINATION OF CCONTROL FIELD AND CONTROL FLAGS
    CONNACK = 0x20,
    PUBLISH = 0x30,
    PUBACK = 0x40,
    PUBREC=0x50,
    PUBREL=0x62,
    PUBCOMP=0x70,
    SUBSCRIBE = 0x82,
    SUBACK = 0x90,
    UNSUBSCRIBE = 0xA2,
    UNSUBACK = 0xB0,
    PINGERQ = 0xC0,
    PINGRESP = 0xD0,
    DISCONNECT = 0xE0
};

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


void mqttSendPublish(etherHeader *ether);
void mqttSendSubscribe(etherHeader *ether);
void mqttSendUnsubscribe(etherHeader *ether);
bool isMqttPublish(etherHeader *ether);
bool isMqttAck(etherHeader *ether);
void mqttSendDisconnect(etherHeader *ether);
void mqttSendConnect(etherHeader *ether);






#endif /* MQTT_H_ */
