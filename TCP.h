/*
 * TCP.h
 *
 *  Created on: 15-Apr-2022
 *      Author: AKASH VIRENDRA AND SOWMYA SRINIVASA
 */

#ifndef TCP_H_
#define TCP_H_

#include "eth0.h"


#define TCP_HEADER_LENGTH 20
#define IP_HEADER_LENGTH 20


void sendTcpMsg(etherHeader *ether,uint16_t flag, uint16_t dest_port,uint8_t dest_ip[4],uint8_t dest_mac[6],char*);
bool isTcpAcknowledgment(etherHeader *ether);
bool isTcpSyn(etherHeader *ether);
void setTcpState(uint8_t state);
uint8_t getTcpState();
void handleTcpSegment(etherHeader *ether);
void buildTcpHeader(etherHeader *ether,uint16_t destinationPort,uint8_t dest_ip[4],uint8_t dest_mac[6],uint16_t flag,uint16_t length);
bool etherCheckTcpCheckSum(etherHeader *ether);
void etherCalcTcpChecksum(etherHeader *ether);
bool etherIsTcp(etherHeader *ether);
bool isTcpSyn(etherHeader *ether);
bool isTcpSynAck(etherHeader *ether);
bool isTcpFin(etherHeader *ether);
bool isTcpReset(etherHeader *ether);
void updateSeqNumber(uint32_t);
void parseHtmlString(etherHeader *ether);


typedef struct _socket
{
    uint8_t ip_address[4];
    uint16_t port;
    uint8_t mac_address[6];
} socket;

typedef struct _tcpHeader // 20 or more bytes
{
  uint16_t sourcePort;
  uint16_t destPort;
  uint32_t sequenceNumber;
  uint32_t acknowledgementNumber;
  uint16_t offsetFields;
  uint16_t windowSize;
  uint16_t checksum;
  uint16_t urgentPointer;
  uint8_t  data[0];
} tcpHeader;

#define TCP_SYN      2      //0x0002
#define TCP_SYNACK   18     //0x0012
#define TCP_ACK      16     //0x0010
#define TCP_PSH      8      //0x0008
#define TCP_PUSH_ACK 24     //0x0018
#define TCP_FIN      1      //0x0001
#define TCP_FIN_ACK  17     //0x0011
#define TCP_RESET    4      //0x0004
#define TCP_REST_ACK 20     //0x0014

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
#define TCP_GET_HTML                22
#define TCP_WAIT_FOR_HTML           23
#define MQTT_SYN                    24
#define MQTT_SYN_ACK                25
#define MQTT_ACK                    26


#endif /* TCP_H_ */
