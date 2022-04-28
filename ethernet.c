
// Ethernet Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL w/ ENC28J60
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// ENC28J60 Ethernet controller on SPI0
//   MOSI (SSI0Tx) on PA5
//   MISO (SSI0Rx) on PA4
//   SCLK (SSI0Clk) on PA2
//   ~CS (SW controlled) on PA3
//   WOL on PB3
//   INT on PC6

// Pinning for IoT projects with wireless modules:
// N24L01+ RF transceiver
//   MOSI (SSI0Tx) on PA5
//   MISO (SSI0Rx) on PA4
//   SCLK (SSI0Clk) on PA2
//   ~CS on PE0
//   INT on PB2
// Xbee module
//   DIN (UART1TX) on PC5
//   DOUT (UART1RX) on PC4


//-----------------------------------------------------------------------------
// Hints for using Wireshark to examine packets
//-----------------------------------------------------------------------------

// sudo ethtool --offload eno2 tx off rx off
// in wireshark, preferences->protocol->ipv4->validate the checksum if possible
// in wireshark, preferences->protocol->udp->validate the checksum if possible

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<time.h>
#include "tm4c123gh6pm.h"
#include "eth0.h"
#include "gpio.h"
#include "spi0.h"
#include "uart0.h"
#include "wait.h"
#include "input.h"
#include "TCP.h"
#include "mqtt.h"

// Pins
#define RED_LED PORTF,1
#define BLUE_LED PORTF,2
#define GREEN_LED PORTF,3
#define PUSH_BUTTON PORTF,4

#define HTTP_PORT 0x0050    //WHICH IS NOTHING BUT 80 IN DECIMAL
#define MQTT_PORT 0X075B    //WHICH IS NOTHIGN BUT 1883 IN DECIMAL

//-----------------------------------------------------------------------------
// Subroutines                
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
	// Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Enable clocks
    enablePort(PORTF);
    _delay_cycles(3);

    // Configure LED and pushbutton pins
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(BLUE_LED);
    selectPinDigitalInput(PUSH_BUTTON);
}

// Max packet is calculated as:
// Ether frame header (18) + Max MTU (1500) + CRC (4)
//ether grame header= 4 bytes of ENC module packet and 14 bytes of ether header(src address, dest address and flags)
#define MAX_PACKET_SIZE 1522
char* publishTopic;
char* publishData;
char* subscribeTopic;
char* unsubscribeTopic;

uint16_t source_port=0;
bool access_html_page=true;

char* clientID="akash";
extern uint8_t html_ip_address[4];
extern uint8_t html_mac_address[6];
extern uint8_t mqttIpAddress[4];
extern uint8_t mqttMacAddress[6];
char *a="GET /cdo-web/api/v2/datasets HTTP/1.1\r\nAuthorization: Bearer {token}\r\nHOST: ncdc.noaa.gov\r\nToken: RYZCRVvDbnuopszBdnwxtecCfBDpXocG\r\n\r\n";
//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
int main(void)
{
    uint8_t* udpData;
    uint8_t buffer[MAX_PACKET_SIZE];
    etherHeader *data = (etherHeader*) buffer;

    // Init controller
    initHw();

    // Setup UART0
    initUart0();
    setUart0BaudRate(115200, 40e6);

    // Init ethernet interface (eth0)
    putsUart0("\nStarting eth0\n");
    etherSetMacAddress(2, 3, 4, 5, 6, 117);
    //etherDisableDhcpMode();
    etherSetIpAddress(192, 168, 1, 117);
    etherSetIpSubnetMask(255, 255, 255, 0);
    etherSetIpGatewayAddress(192, 168, 1, 1);
    etherSetDestMacAddress(0x14,0x59,0xc0,0x4d,0x43,0x80);
    etherSetHtmlIpAddress(23,33,56,87);
    etherInit(ETHER_UNICAST | ETHER_BROADCAST | ETHER_HALFDUPLEX);
    waitMicrosecond(100000);
    displayConnectionInfo();

    // Flash LED
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(100000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(100000);

    // Main Loop
    // RTOS and interrupts would greatly improve this code,
    // but the goal here is simplicity


    //choose a source port
    //generate a random port for the source port
    //the ephermal ports ranges from 49152 to 65535
    //this uses the time at this  instant as the seed value
    srand(time(0));
    source_port=htons(49152+ (rand()%(16384-1))); //assigns an ephermal port as the source port
    while (true)
    {
        // Put terminal processing here
        processInputCommands();

        if(isTcpEnabled())
        {
            uint8_t state=getTcpState();
            if(state==TCP_SEND_SYN)
            {
                if(access_html_page==1)
                {
                    uint8_t dest_mac[6],dest_ip[4];
                    etherGetDestMacAddress(dest_mac);
                    etherGetHtmlIpAddress(dest_ip);
                    sendTcpMsg(data,TCP_SYN,HTTP_PORT,dest_ip,dest_mac,0);
                    setTcpState(TCP_RECEIVE_SYN_ACK);
                }
                else
                {
                    sendTcpMsg(data,TCP_SYN,MQTT_PORT,mqttIpAddress,mqttMacAddress,0);
                }
            }
            else if(state==TCP_SEND_ACK)
            {
                if(access_html_page==1)
                {
                    uint8_t dest_mac[6],dest_ip[4];
                    etherGetDestMacAddress(dest_mac);
                    etherGetHtmlIpAddress(dest_ip);
                    sendTcpMsg(data,TCP_ACK,HTTP_PORT,dest_ip,dest_mac,0);
                    setTcpState(TCP_GET_HTML);
                }
                else
                {
                    sendTcpMsg(data,TCP_ACK,MQTT_PORT,mqttIpAddress,mqttMacAddress,0);
                    setTcpState(MQTT_CONNECT);
                }
            }
            else if(state==TCP_GET_HTML && access_html_page)
            {
                uint8_t dest_mac[6],dest_ip[4];
                etherGetDestMacAddress(dest_mac);
                etherGetHtmlIpAddress(dest_ip);
                sendTcpMsg(data,TCP_ACK,HTTP_PORT,dest_ip,dest_mac,a);
                setTcpState(TCP_WAIT_FOR_HTML);
            }
            else if(state==MQTT_SYN)
            {
                uint8_t dest_mac[6],dest_ip[4];
                etherGetMqttMacAddress(dest_mac);
                etherGetMqttIpAddress(dest_ip);
                sendTcpMsg(data,TCP_SYN,MQTT_PORT,dest_ip,dest_mac,0);
                setTcpState(MQTT_SYN_ACK);
            }
            else if(state==MQTT_CONNECT)
            {
                mqttSendConnect(data,clientID,CLEAN_SESSION);
                setTcpState(MQTT_CONNECT_ACK);
            }
            else if(state==MQTT_DISCONNECT)
            {
                mqttSendDisconnect(data);
                setTcpState(TCP_SEND_FIN);
            }
            else if(state==TCP_SEND_FIN)
            {

            }

        }

        // Packet processing
        if (etherIsDataAvailable())
        {
            if (etherIsOverflow())
            {
                setPinValue(RED_LED, 1);
                waitMicrosecond(100000);
                setPinValue(RED_LED, 0);
            }

            // Get packet
            etherGetPacket(data, MAX_PACKET_SIZE);

            // Handle ARP request
            if (etherIsArpRequest(data))
            {
                etherSendArpResponse(data);
            }

            // Handle IP datagram
            if (etherIsIp(data))
            {
            	if (etherIsIpUnicast(data))
            	{
            	    if(etherIsTcp(data))
            	        handleTcpSegment(data);

            		// handle icmp ping request
					if (etherIsPingRequest(data))
					{
					  etherSendPingResponse(data);
					}

					// Process UDP datagram
					// test this with a udp send utility like sendip
					//   if sender IP (-is) is 192.168.1.198, this will attempt to
					//   send the udp datagram (-d) to 192.168.1.199, port 1024 (-ud)
					// sudo sendip -p ipv4 -is 192.168.1.198 -p udp -ud 1024 -d "on" 192.168.1.199
                    // sudo sendip -p ipv4 -is 192.168.1.198 -p udp -ud 1024 -d "off" 192.168.1.199
					if (etherIsUdp(data))
					{
						udpData = etherGetUdpData(data);
						if (strcmp((char*)udpData, "on") == 0)
			                setPinValue(GREEN_LED, 1);
                        if (strcmp((char*)udpData, "off") == 0)
			                setPinValue(GREEN_LED, 0);
						etherSendUdpResponse(data, (uint8_t*)"Received", 9);
					}
                }
            }
        }
    }
}
