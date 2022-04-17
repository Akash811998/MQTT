/*
 * TCP.h
 *
 *  Created on: 15-Apr-2022
 *      Author: AKASH VIRENDRA AND SOWMYA SRINIVASA
 */

#ifndef TCP_H_
#define TCP_H_

#include "eth0.h"

void sendTcpMsg(etherHeader *ether,uint16_t,uint16_t);
bool isTcpAcknowledgment(etherHeader *ether);
bool isTcpSyn(etherHeader *ether);
void setTcpstate(uint8_t state);
uint8_t getTcpstate();
void handleTcpSegment(etherHeader *ether);

typedef struct _socket
{
    uint8_t ip_address[4];
    uint16_t port;
    uint8_t mac_address[6];
} socket;

#define TCP_SYNC     2
#define TCP_SYNACK   18
#define TCP_ACK      16
#define TCP_PSH      8
#define TCP_PUSH_ACK 24
#define TCP_FIN      1
#define TCP_FIN_ACK  17
#define TCP_RESET    4
#define TCP_REST_ACK 20

//current TCP and  states
#define TCP_IDLE                    0
#define TCP_SEND_ARP                1
#define TCP_RECEIVE_ARP             2
#define TCP_SEND_SYN                3
#define TCP_RECEIVE_SYN_ACK         4
#define TCP_SEND_ACK                5
#define TCP_LIVE                    6
#define TCP_SEND_FIN                7
#define TCP_RCV_FIN_ACK             8
#define TCP_RCV_FIN                 9
#define MQTT_CONNECT                10
#define MQTT_CONNECT_ACK            11
#define MQTT_SEND_PUBLISH0          12
#define MQTT_SEND_PUBLISH1          13
#define MQTT_RCV_PUBLISH_ACK        14
#define MQTT_SEND_SUBSCRIBE         15
#define MQTT_RCV_SUBSCRIBE_ACK      16
#define MQTT_SEND_UNSUBSCRIBE       17
#define MQTT_SEND_UNSUBSCRIBE_ACK   18
#define MQTT_PING_REQ               19
#define MQTT_PING_RESP              20
#define MQTT_DISCONNECT             21

#endif /* TCP_H_ */
