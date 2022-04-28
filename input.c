// AKASH VIRENDRA

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

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "input.h"
#include "uart0.h"
#include "TCP.h"
#include "eth0.h"
#include "mqtt.h"

bool flag=1;//for parsefield

char str[50]; //for sprintf


USER_DATA data;//defining the data variable as a global

extern char* publishTopic;
extern char* publishData;
extern char* subscribeTopic;
extern char* unsubscribeTopic;
extern bool mqttlive;

bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    int i=data->fieldPosition[0];
    char* str=&(data->buffer[data->fieldPosition[0]]);
    int f=1;

    while(str[i]!='\0' && f!=0)
    {
        if(str[i]==strCommand[i])
        {
            i++;
        }
        else
        {
            f=0;
            return 0;
        }
    }
    if(((data->fieldCount)-1)!=minArguments)
        return 0;
    return 1;
}


void parseFields(USER_DATA* data)
{
    int i=0,j=0;  //i used for normal string array and j used for fieldtype which can be a max of 5
    data->fieldPosition[j]=i;
    data->fieldCount=0;
    (data->fieldCount)++;
    while(data->buffer[i]!='\0')
    {
        if( (data->buffer[i]>=65 && data->buffer[i]<=90) || (data->buffer[i]>=97 && data->buffer[i]<=122))
            data->fieldType[j]='a';
        else if((data->buffer[i]>=48 && data->buffer[i]<=57) || data->buffer[i]==46 || data->buffer[i]==45)
            data->fieldType[j]='n';
        else
        {
            data->buffer[i]='\0';
            data->fieldPosition[j+1]=i+1;
            (data->fieldCount)++;
            j++;
        }

        i++;
        if(j>=MAX_FIELDS)
        {

            flag=0;
            return;
        }
    }
}
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    if(fieldNumber<MAX_FIELDS)
    {
        if(data->fieldType[fieldNumber]=='a')
            return &(data->buffer[data->fieldPosition[fieldNumber]]);
        else
        {
            putsUart0("\nthe word in the alphabet is not a alphabet");
            return NULL;
        }
    }
    putsUart0("\ninvalid number of arguments");

    return NULL;
}

int number_of_digits(int n)
{
    int dc = 0;

    while(n > 0)
    {
        dc++;
        n /= 10;
    }

    return dc;
}
uint8_t strCompare(char* str1, const char* str2)
{
    int i=0;
    while(str1[i]!='\0' || str2[i]!='\0')
    {
        if(str1[i]==str2[i])
            i++;
        else
            return 0;
    }
    return 1;
}

char* i_to_a(int n)
{
    int dc = 0;

    if(n < 0)
    {
        n = -1*n;
        dc++;
    }

    dc += number_of_digits(n);

    str[dc] = '\0';

    while(n > 0)
    {
        str[dc-1] = n%10 + 48;
        n = n/10;
        dc--;
    }

    if(dc == 1)
        str[0] = '-';

    return str;
}

uint16_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    int i=data->fieldPosition[fieldNumber];
    int result=0,temp=0,num=i,j,inc=0; //result is to store the final integer value
    //temp is used to store the intermediate value
    //num is like to counter to go till the end of the number and then to convert it
    //j is used for for lopp
    if(fieldNumber<MAX_FIELDS)
    {
        if(data->fieldType[fieldNumber]=='n')
        {
            while(data->buffer[i]!='\0')
            {
                i++;
            }

            for(j=i-1;j>=num;j--)
            {
                temp=(data->buffer[j])-48;
                result=result+temp*power(10,inc++);
            }
            return result;
        }
        else
        {
            putsUart0("not a integer string, cannot convert it");
            return 0;
        }
    }
    putsUart0("\ninvalid number of arguments");
    return 0;
}

int power(int base,int exp)
{
    int i;
    int result=1;
    if(exp==0)
        return 1;
    else
    {
        for(i=1;i<=exp;i++)
            result=result*base;
        return result;
    }
}
void displayConnectionInfo()
{
    uint8_t i;
    char str[10];
    uint8_t mac[6];
    uint8_t ip[4];
    etherGetMacAddress(mac);
    putsUart0("HW: ");
    for (i = 0; i < 6; i++)
    {
        sprintf(str, "%02x", mac[i]);
        putsUart0(str);
        if (i < 6-1)
            putcUart0(':');
    }
    putcUart0('\n');
    etherGetIpAddress(ip);
    putsUart0("IP: ");
    for (i = 0; i < 4; i++)
    {
        sprintf(str, "%u", ip[i]);
        putsUart0(str);
        if (i < 4-1)
            putcUart0('.');
    }
//    if (etherIsDhcpEnabled())
//        putsUart0(" (dhcp)");
//    else
//        putsUart0(" (static)");
//    putcUart0('\n');
    etherGetIpSubnetMask(ip);
    putsUart0("SN: ");
    for (i = 0; i < 4; i++)
    {
        sprintf(str, "%u", ip[i]);
        putsUart0(str);
        if (i < 4-1)
            putcUart0('.');
    }
    putcUart0('\n');
    etherGetIpGatewayAddress(ip);
    putsUart0("GW: ");
    for (i = 0; i < 4; i++)
    {
        sprintf(str, "%u", ip[i]);
        putsUart0(str);
        if (i < 4-1)
            putcUart0('.');
    }
    putcUart0('\n');
    if (etherIsLinkUp())
        putsUart0("Link is up\n");
    else
        putsUart0("Link is down\n");
}
uint8_t strLength(char* a)
{
    uint8_t len=0,i=0;
    while(a[i++]!='\0')
        len++;
    return len;
}

void processInputCommands()
{
    if(kbhitUart0())
    {
        getsUart0(&data);
        parseFields(&data);
        bool valid=false;

        if(isCommand(&data,"ifconfig",0)) //display the details
        {
            displayConnectionInfo();
            valid=true;
        }
        else if(isCommand(&data,"publish",2)) //publish the topic name and the data
        {
            valid=true;
            if(mqttlive==false)
                putsUart0("Connection to the broker should be established before you publish a message");
            else
            {
                publishTopic=getFieldString(&data, 1);
                publishData=getFieldString(&data, 2);
                setTcpState(MQTT_SEND_PUBLISH0);
            }
        }
        else if(isCommand(&data,"disconnect",0)) //sends a disconnect request to the broker
        {
            valid=true;
            setTcpState(MQTT_DISCONNECT);
        }
        else if(isCommand(&data,"reboot",0))
        {
            valid=true;
            //if tcp active, then send a fin message and deactivate the TCP
            //basically send a MQTT disconnect message and restart the device
            //then restart the device
        }
        else if(isCommand(&data,"help",0))
        {
            valid=true;
        }
        else if(isCommand(&data,"set",0)) //set the IP address or mqtt ip address
        {
            valid=true;
        }
        else if(isCommand(&data,"subscribe",1))  //subscribe to a topic
        {
            valid=true;
        }
        else if(isCommand(&data,"unsubscribe",1)) //unsubscribe to a topic
        {
            valid=true;
        }



    }
}
