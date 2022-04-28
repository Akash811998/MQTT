// ETH0 Library
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

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "gpio.h"
#include "spi0.h"
#include "eth0.h"
#include "TCP.h"

// Pins
#define CS PORTA,3
#define WOL PORTB,3
#define INT PORTC,6

// Ether registers
#define ERDPTL      0x00    //READ POINTER LOW
#define ERDPTH      0x01    //READ POINTER HIGH
#define EWRPTL      0x02    //WRITE POINTER LOW
#define EWRPTH      0x03    //WRITE POINTER HIGH
#define ETXSTL      0x04    //TRANSMIT BUFFER START LOCATION LOW
#define ETXSTH      0x05    //TRANSMIT BUFFER START LOCATION HIGH
#define ETXNDL      0x06
#define ETXNDH      0x07
#define ERXSTL      0x08
#define ERXSTH      0x09
#define ERXNDL      0x0A
#define ERXNDH      0x0B
#define ERXRDPTL    0x0C
#define ERXRDPTH    0x0D
#define ERXWRPTL    0x0E
#define ERXWRPTH    0x0F
#define EIE         0x1B
#define EIR         0x1C
#define RXERIF  0x01
#define TXERIF  0x02
#define TXIF    0x08
#define PKTIF   0x40
#define ESTAT       0x1D
#define CLKRDY  0x01
#define TXABORT 0x02
#define ECON2       0x1E
#define PKTDEC  0x40
#define ECON1       0x1F
#define RXEN    0x04
#define TXRTS   0x08
#define ERXFCON     0x38
#define EPKTCNT     0x39
#define MACON1      0x40
#define MARXEN  0x01
#define RXPAUS  0x04
#define TXPAUS  0x08
#define MACON2      0x41
#define MARST   0x80
#define MACON3      0x42
#define FULDPX  0x01
#define FRMLNEN 0x02
#define TXCRCEN 0x10
#define PAD60   0x20
#define MACON4      0x43
#define MABBIPG     0x44
#define MAIPGL      0x46
#define MAIPGH      0x47
#define MACLCON1    0x48
#define MACLCON2    0x49
#define MAMXFLL     0x4A
#define MAMXFLH     0x4B
#define MICMD       0x52
#define MIIRD   0x01
#define MIREGADR    0x54
#define MIWRL       0x56
#define MIWRH       0x57
#define MIRDL       0x58
#define MIRDH       0x59
#define MAADR1      0x60
#define MAADR0      0x61
#define MAADR3      0x62
#define MAADR2      0x63
#define MAADR5      0x64
#define MAADR4      0x65
#define MISTAT      0x6A
#define MIBUSY  0x01
#define ECOCON      0x75

// Ether phy registers
#define PHCON1      0x00
#define PDPXMD 0x0100
#define PHSTAT1     0x01
#define LSTAT  0x0400
#define PHCON2      0x10
#define HDLDIS 0x0100
#define PHLCON      0x14

#define IP_ADD_LENGTH 4
#define HW_ADD_LENGTH 6
// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

uint8_t nextPacketLsb = 0x00;
uint8_t nextPacketMsb = 0x00;
uint8_t sequenceId = 1;
uint8_t hwAddress[HW_ADD_LENGTH] = {2,3,4,5,6,7};       //MY MAC ADDRESS
uint8_t ipAddress[IP_ADD_LENGTH] = {0,0,0,0};           //MY IP ADDRESS
uint8_t ipSubnetMask[IP_ADD_LENGTH] = {0,0,0,0};        //IP SUBNET MASK
uint8_t ipGwAddress[IP_ADD_LENGTH] = {0,0,0,0};         //IP GATEWAY ADDRESS
uint8_t ipDnsAddress[IP_ADD_LENGTH] = {0,0,0,0};        //IP DNS ADDRESS
//uint8_t ipTimeServerAddress[IP_ADD_LENGTH] = {0,0,0,0}; //IP TIMESERVER ADDRESS
//uint8_t dhcpServerAddress[IP_ADD_LENGTH] = {0,0,0,0};
uint8_t html_ip_address[4]={0,0,0,0};                   //IP ADDRESS OF THE HTTP PAGE
uint8_t destMacAddress[6]={0,0,0,0,0,0};

//bool    dhcpEnabled = true;

// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Buffer is configured as follows
// Receive buffer starts at 0x0000 (bottom 6666 bytes of 8K space)
// Transmit buffer at 01A0A (top 1526 bytes of 8K space)

void etherCsOn(void)
{
    setPinValue(CS, 0);  //it sets the bitband address to 1
    _delay_cycles(4);                    // allow line to settle
}

void etherCsOff(void)
{
    setPinValue(CS, 1);
}

void etherWriteReg(uint8_t reg, uint8_t data)
{
    etherCsOn();
    writeSpi0Data(0x40 | (reg & 0x1F)); //see page 28 of ethernet datasheet. for write the first MSB 3 bits should be 010 and the last 5 bits ttake the value of the register or data constant. the siz eof register is 8 bits hence mask it to 5
    readSpi0Data(); //return the value of the SPI data register. this is done because whenever you write something into the register a dummy value is returned back. you have to do this or else the receive buffer overflows
    writeSpi0Data(data);//then write the actual data to the data register of SPI
    readSpi0Data();//same here also why read is done
    etherCsOff();
}

uint8_t etherReadReg(uint8_t reg)
{
    uint8_t data;
    etherCsOn();
    writeSpi0Data(0x00 | (reg & 0x1F));// to read data 0x00 is the command
    readSpi0Data();
    writeSpi0Data(0);// we are writing a dummy data because. first when we send the command the dummy data is sent back. to receive the good data again you have to send a dummy data
    data = readSpi0Data();
    etherCsOff();
    return data;
}

void etherSetReg(uint8_t reg, uint8_t mask)
{
    etherCsOn();
    writeSpi0Data(0x80 | (reg & 0x1F)); //to set a bit field the command is 0x80
    readSpi0Data(); //read is done again for the same cause
    writeSpi0Data(mask); //internally the chip, reads the register OR's it with the mask and writes to the register in the ethenet chip. that way you dont have to read it modify and writ eit back
    readSpi0Data();
    etherCsOff();
}

void etherClearReg(uint8_t reg, uint8_t mask)
{
    etherCsOn();
    writeSpi0Data(0xA0 | (reg & 0x1F));// to clear register 0xA0 is the command
    readSpi0Data();
    writeSpi0Data(mask);
    readSpi0Data();
    etherCsOff();
}

void etherSetBank(uint8_t reg)
{
    etherClearReg(ECON1, 0x03); //clear 0 and 1 bits of the ECON1 register
    etherSetReg(ECON1, reg >> 5);
}

void etherWritePhy(uint8_t reg, uint16_t data) //the phy converts the data to 1's and zeros's
{
    etherSetBank(MIREGADR);   //MIREGADR THIS IS USED FOR WRITING OR READING THE PHY REGISTERS
    etherWriteReg(MIREGADR, reg); //USE BANK 2
    etherWriteReg(MIWRL, data & 0xFF); //WRITE THE LOW BYTE
    etherWriteReg(MIWRH, (data >> 8) & 0xFF);  //WRITE THE HIGH BYTE
}

uint16_t etherReadPhy(uint8_t reg)
{
    uint16_t data, dataH;
    etherSetBank(MIREGADR);
    etherWriteReg(MIREGADR, reg);
    etherWriteReg(MICMD, MIIRD);
    waitMicrosecond(11);
    etherSetBank(MISTAT);  //SET BANK TO STATUS REGISTER
    while ((etherReadReg(MISTAT) & MIBUSY) != 0);  //CHECK IF THE STATUS REGISTER IS BUSY
    etherSetBank(MICMD);  //THEN CAHANGE THE REGISTER TO THE COMMAND REGISTER
    etherWriteReg(MICMD, 0);
    data = etherReadReg(MIRDL); //READ THE LOW BYTE
    dataH = etherReadReg(MIRDH); //.READ THE HIGH BYTE
    data |= (dataH << 8);
    return data;
}

void etherWriteMemStart(void)
{
    etherCsOn();
    writeSpi0Data(0x7A);  //WRITE DATA TO THE BUFFER MEMORY
    readSpi0Data(); //DUMMY VALUES WILL BE RETURNED WHICH NEEDS TO BE RETURNED OR ELSE THE BUFFER WILL BE FULL
}

void etherWriteMem(uint8_t data)
{
    writeSpi0Data(data);
    readSpi0Data();
}

void etherWriteMemStop(void)
{
    etherCsOff();
}

void etherReadMemStart(void)
{
    etherCsOn();    //switch on CS
    writeSpi0Data(0x3A);  // 3A because the opcode and argument of read buffer memory is 3A
    //send the command to to ethernet module to say that we want to read from read buffer memory
    readSpi0Data();  //then from the SPI data  register read the data which has come
}

uint8_t etherReadMem(void)
{
    writeSpi0Data(0);  //to get the byte we need to write a 0 to it to gte the byte
    return readSpi0Data();
}

void etherReadMemStop(void)
{
    etherCsOff();
}

// Initializes ethernet device
// Uses order suggested in Chapter 6 of datasheet except 6.4 OST which is first here
void etherInit(uint16_t mode)
{
    // Initialize SPI0
    initSpi0(USE_SSI0_RX);  //initialise the SPI for receiving from uc
    setSpi0BaudRate(10e6, 40e6);
    setSpi0Mode(0, 0);

    // Enable clocks
    enablePort(PORTA);
    enablePort(PORTB);
    enablePort(PORTC);

    // Configure pins for ethernet module
    selectPinPushPullOutput(CS);
    selectPinDigitalInput(WOL);
    selectPinDigitalInput(INT);

    // make sure that oscillator start-up timer has expired
    while ((etherReadReg(ESTAT) & CLKRDY) == 0) {}

    // disable transmission and reception of packets
    etherClearReg(ECON1, RXEN);
    etherClearReg(ECON1, TXRTS);

    // initialize receive buffer space
    etherSetBank(ERXSTL);
    etherWriteReg(ERXSTL, LOBYTE(0x0000));
    etherWriteReg(ERXSTH, HIBYTE(0x0000));
    etherWriteReg(ERXNDL, LOBYTE(0x1A09));
    etherWriteReg(ERXNDH, HIBYTE(0x1A09));

    // initialize receiver write and read ptrs
    // at startup, will write from 0 to 1A08 only and will not overwrite rd ptr
    etherWriteReg(ERXWRPTL, LOBYTE(0x0000));
    etherWriteReg(ERXWRPTH, HIBYTE(0x0000));
    etherWriteReg(ERXRDPTL, LOBYTE(0x1A09));
    etherWriteReg(ERXRDPTH, HIBYTE(0x1A09));
    etherWriteReg(ERDPTL, LOBYTE(0x0000));
    etherWriteReg(ERDPTH, HIBYTE(0x0000));

    // setup receive filter
    // always check CRC, use OR mode
    etherSetBank(ERXFCON);
    etherWriteReg(ERXFCON, (mode | ETHER_CHECKCRC) & 0xFF);

    // bring mac out of reset
    etherSetBank(MACON2);
    etherWriteReg(MACON2, 0);

    // enable mac rx, enable pause control for full duplex
    etherWriteReg(MACON1, TXPAUS | RXPAUS | MARXEN);

    // enable padding to 60 bytes (no runt packets)
    // add crc to tx packets, set full or half duplex
    if ((mode & ETHER_FULLDUPLEX) != 0)
        etherWriteReg(MACON3, FULDPX | FRMLNEN | TXCRCEN | PAD60);
    else
        etherWriteReg(MACON3, FRMLNEN | TXCRCEN | PAD60);

    // leave MACON4 as reset

    // set maximum rx packet size
    etherWriteReg(MAMXFLL, LOBYTE(1518));
    etherWriteReg(MAMXFLH, HIBYTE(1518));

    // set back-to-back inter-packet gap to 9.6us
    if ((mode & ETHER_FULLDUPLEX) != 0)
        etherWriteReg(MABBIPG, 0x15);
    else
        etherWriteReg(MABBIPG, 0x12);

    // set non-back-to-back inter-packet gap registers
    etherWriteReg(MAIPGL, 0x12);
    etherWriteReg(MAIPGH, 0x0C);

    // leave collision window MACLCON2 as reset

    // initialize phy duplex
    if ((mode & ETHER_FULLDUPLEX) != 0)
        etherWritePhy(PHCON1, PDPXMD);
    else
        etherWritePhy(PHCON1, 0);

    // disable phy loopback if in half-duplex mode
    etherWritePhy(PHCON2, HDLDIS);

    // Flash LEDA and LEDB
    etherWritePhy(PHLCON, 0x0880);
    waitMicrosecond(100000);

    // set LEDA (link status) and LEDB (tx/rx activity)
    // stretch LED on to 40ms (default)
    etherWritePhy(PHLCON, 0x0472);

    // enable reception
    etherSetReg(ECON1, RXEN);
}

// Returns true if link is up
bool etherIsLinkUp(void)
{
    return (etherReadPhy(PHSTAT1) & LSTAT) != 0;
}

// Returns TRUE if packet received
bool etherIsDataAvailable(void)
{
    return ((etherReadReg(EIR) & PKTIF) != 0);   //if PKTIF is set, then still there are one or more unprocessed packets in the buffer or the receive counter is non zero
}

// Returns true if rx buffer overflowed after correcting the problem
bool etherIsOverflow(void)
{
    bool err;
    err = (etherReadReg(EIR) & RXERIF) != 0;   //RXERIF states that  A packet was aborted because there is insufficient buffer space or the packet count is 255
    if (err)
        etherClearReg(EIR, RXERIF);
    return err;
}

// Returns up to max_size characters in data buffer
// Returns number of bytes copied to buffer
// Contents written are 16-bit size, 16-bit status, payload excl crc
uint16_t etherGetPacket(etherHeader *ether, uint16_t maxSize)
{
    uint16_t i = 0, size, tmp16, status;
    uint8_t *packet = (uint8_t*)ether;

    // enable read from FIFO buffers
    etherReadMemStart();

    // get next packet information
    nextPacketLsb = etherReadMem();
    nextPacketMsb = etherReadMem();

    // calc size
    // don't return crc, instead return size + status, so size is correct
    size = etherReadMem();
    tmp16 = etherReadMem();
    size |= (tmp16 << 8);

    // get status (currently unused)
    status = etherReadMem();
    tmp16 = etherReadMem();
    status |= (tmp16 << 8);

    // copy data
    if (size > maxSize)
        size = maxSize;
    while (i < size)
        packet[i++] = etherReadMem();

    // end read from FIFO buffers
    etherReadMemStop();

    // advance read pointer
    etherSetBank(ERXRDPTL);
    etherWriteReg(ERXRDPTL, nextPacketLsb); // hw ptr
    etherWriteReg(ERXRDPTH, nextPacketMsb);
    etherWriteReg(ERDPTL, nextPacketLsb);   // dma rd ptr
    etherWriteReg(ERDPTH, nextPacketMsb);

    // decrement packet counter so that PKTIF is maintained correctly
    etherSetReg(ECON2, PKTDEC);  //hardware increments the packet counter, we decrement it
    //whenever the packet count is not zero, an interrupt it issued to the host saying that theres a packet available

    return size;
}

// Writes a packet
bool etherPutPacket(etherHeader *ether, uint16_t size)
{
    uint16_t i;
    uint8_t *packet = (uint8_t*) ether;

    // clear out any tx errors
    if ((etherReadReg(EIR) & TXERIF) != 0)
    {
        etherClearReg(EIR, TXERIF);
        etherSetReg(ECON1, TXRTS);
        etherClearReg(ECON1, TXRTS);
    }

    // set DMA start address
    etherSetBank(EWRPTL);
    etherWriteReg(EWRPTL, LOBYTE(0x1A0A));
    etherWriteReg(EWRPTH, HIBYTE(0x1A0A));

    // start FIFO buffer write
    etherWriteMemStart();

    // write control byte
    //ONCE AFTER WRITING TO THE CONTROL REGISTER TO SAY WHAT NEEDS TO DO, MULTIPLE DATA CAN BE SENT AT A TIME
    //THE CHIP HAS A DMA WHICH AUTOMATICALLY INCREMENTS THE ADDRESS WHEN DATA IS BEING SENT
    etherWriteMem(0);

    // write data
    for (i = 0; i < size; i++)
        etherWriteMem(packet[i]);

    // stop write
    etherWriteMemStop();

    // request transmit
    etherWriteReg(ETXSTL, LOBYTE(0x1A0A));
    etherWriteReg(ETXSTH, HIBYTE(0x1A0A));
    etherWriteReg(ETXNDL, LOBYTE(0x1A0A+size));
    etherWriteReg(ETXNDH, HIBYTE(0x1A0A+size));
    etherClearReg(EIR, TXIF);
    etherSetReg(ECON1, TXRTS);

    // wait for completion
    while ((etherReadReg(ECON1) & TXRTS) != 0);

    // determine success
    return ((etherReadReg(ESTAT) & TXABORT) == 0);
}

// Calculate sum of words
// Must use getEtherChecksum to complete 1's compliment addition
void etherSumWords(void* data, uint16_t sizeInBytes, uint32_t* sum)
{
    uint8_t* pData = (uint8_t*)data;
    uint16_t i;
    uint8_t phase = 0;
    uint16_t data_temp;
    for (i = 0; i < sizeInBytes; i++)
    {
        if (phase)
        {
            data_temp = *pData;
            *sum += data_temp << 8;
        }
        else
            *sum += *pData;
        phase = 1 - phase;
        pData++;
    }
}

// Completes 1's compliment addition by folding carries back into field
uint16_t getEtherChecksum(uint32_t sum)
{
    uint16_t result;
    // this is based on rfc1071
    while ((sum >> 16) > 0)
        sum = (sum & 0xFFFF) + (sum >> 16);
    result = sum & 0xFFFF;
    return ~result;
}

void etherCalcIpChecksum(ipHeader* ip)
{
    // 32-bit sum over ip header
    uint32_t sum = 0;
    etherSumWords(&ip->revSize, 10, &sum); //calculates till protocol field
    etherSumWords(ip->sourceIp, ((ip->revSize & 0xF) * 4) - 12, &sum);//-12 because dont calcukate till checksum.start with sourceaddress field
    ip->headerChecksum = getEtherChecksum(sum);
}


// Converts from host to network order and vice versa
uint16_t htons(uint16_t value)
{
    return ((value & 0xFF00) >> 8) + ((value & 0x00FF) << 8);
}

uint32_t htonl(uint32_t value)
{
    return ((value & 0xFF000000) >> 24) + ((value & 0x00FF0000) >> 8) +
            ((value & 0x0000FF00) << 8) + ((value & 0x000000FF) << 24);
}

// Determines whether packet is IP datagram
bool etherIsIp(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;  //the ethernet header data space pointed to the IP header structure
    uint8_t ipHeaderLength = (ip->revSize & 0xF) * 4; //here we are extracting the header length which is of the 4 LSB bits of revsize and
                                                       //as the header length values ranges from 5 to 15 and hence you need to multiply by 4
                                                       //to get the total number of bytes
    uint32_t sum = 0;
    bool ok;
    ok = (ether->frameType == htons(0x0800)); //htons is the berkley sockets standard model extension to intercept the message in the opposite way
                                              //htons is host to network order i.e it converts little endian to network order
//    while(i<4)                             //if the eth type is IP then  verify the checksum of the entire IP header
//    {
//        ok&=(ipAddress[i]==ip->destIp[i]);
//        i++;
//    }
    etherSumWords(&ip->revSize, ipHeaderLength, &sum);  //computes the checksum
    ok&= (getEtherChecksum(sum) == 0);

    return ok;
}

// Determines whether packet is unicast to this ip
// Must be an IP packet
bool etherIsIpUnicast(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t i = 0;
    bool ok = true;
    while (ok & (i < IP_ADD_LENGTH))
    {
        ok = (ip->destIp[i] == ipAddress[i]);
        i++;
    }
    return ok;
}

// Determines whether packet is ping request
// Must be an IP packet
bool etherIsPingRequest(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = (ip->revSize & 0xF) * 4;
    icmpHeader *icmp = (icmpHeader*)((uint8_t*)ip + ipHeaderLength);
    return (ip->protocol == 0x01 & icmp->type == 8);
}

// Sends a ping response given the request data
void etherSendPingResponse(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = (ip->revSize & 0xF) * 4;
    icmpHeader *icmp = (icmpHeader*)((uint8_t*)ip + ipHeaderLength);
    uint8_t i, tmp;
    uint16_t icmp_size;
    uint32_t sum = 0;
    // swap source and destination fields
    for (i = 0; i < HW_ADD_LENGTH; i++)
    {
        tmp = ether->destAddress[i];
        ether->destAddress[i] = ether->sourceAddress[i];
        ether->sourceAddress[i] = tmp;
    }
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        tmp = ip->destIp[i];
        ip->destIp[i] = ip ->sourceIp[i];
        ip->sourceIp[i] = tmp;
    }
    // this is a response
    icmp->type = 0;
    // calc icmp checksum
    icmp->check = 0;
    icmp_size = ntohs(ip->length) - ipHeaderLength;
    etherSumWords(icmp, icmp_size, &sum);
    icmp->check = getEtherChecksum(sum);
    // send packet
    etherPutPacket(ether, sizeof(etherHeader) + ntohs(ip->length));
}

// Determines whether packet is ARP
bool etherIsArpRequest(etherHeader *ether)
{
    arpPacket *arp = (arpPacket*)ether->data;
    bool ok=0;
    uint8_t i = 0;
    ok = (ether->frameType == htons(0x0806));
    if(ok)
        ok&=(arp->op==htons(1));
    while (ok & (i < IP_ADD_LENGTH))
    {
        ok&=(arp->destIp[i]==ipAddress[i]);
        i++;
    }

    return ok;
}

// Determines whether packet is ARP response
bool etherIsArpResponse(etherHeader *ether)
{
    arpPacket *arp = (arpPacket*)ether->data;
    bool ok;
    uint8_t i = 0;
    ok = (ether->frameType == htons(0x0806));
    if (ok)
        ok&=(arp->op==htons(2));
    while (ok & (i < IP_ADD_LENGTH))
    {
        ok&= (arp->destIp[i] == ipAddress[i]);
        i++;
    }
    return ok;
}



// Sends an ARP response given the request data
void etherSendArpResponse(etherHeader *ether)
{
    arpPacket *arp = (arpPacket*)ether->data;
    uint8_t i, tmp;
    // set op to response
    arp->op = htons(2);
    // swap source and destination fields
    for (i = 0; i < HW_ADD_LENGTH; i++)
    {
        arp->destAddress[i] = arp->sourceAddress[i];
        ether->destAddress[i] = ether->sourceAddress[i];
        ether->sourceAddress[i] = arp->sourceAddress[i] = hwAddress[i];
    }
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        tmp = arp->destIp[i];
        arp->destIp[i] = arp->sourceIp[i];
        arp->sourceIp[i] = tmp;
    }
    // send packet
    etherPutPacket(ether, sizeof(etherHeader) + sizeof(arpPacket));
}

// Sends an ARP request
void etherSendArpRequest(etherHeader *ether, uint8_t ipFrom[], uint8_t ipTo[])
{
    arpPacket *arp = (arpPacket*)ether->data;
    uint8_t i;
    // fill ethernet frame
    for (i = 0; i < HW_ADD_LENGTH; i++)
    {
        ether->destAddress[i] = 0xFF;
        ether->sourceAddress[i] = hwAddress[i];
    }
    ether->frameType = htons(0x0806);
    // fill arp frame
    arp->hardwareType = htons(1);
    arp->protocolType = htons(0x0800);
    arp->hardwareSize = HW_ADD_LENGTH;
    arp->protocolSize = IP_ADD_LENGTH;
    arp->op = htons(1);
    for (i = 0; i < HW_ADD_LENGTH; i++)
    {
        arp->sourceAddress[i] = hwAddress[i];
        arp->destAddress[i] = 0xFF;
    }
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        arp->sourceIp[i] = ipFrom[i];
        arp->destIp[i] = ipTo[i];
    }
    // send packet
    etherPutPacket(ether, sizeof(etherHeader) + sizeof(arpPacket));
}

// Determines whether packet is UDP datagram
// Must be an IP packet
bool etherIsUdp(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = (ip->revSize & 0xF) * 4;
    udpHeader *udp = (udpHeader*)((uint8_t*)ip + ipHeaderLength);
    bool ok;
    uint16_t tmp16;
    uint32_t sum = 0;
    ok = (ip->protocol == 0x11);
    if (ok)
    {
        // 32-bit sum over pseudo-header
        etherSumWords(ip->sourceIp, 8, &sum);
        tmp16 = ip->protocol;
        sum += (tmp16 & 0xff) << 8;
        etherSumWords(&udp->length, 2, &sum);
        // add udp header and data
        etherSumWords(udp, ntohs(udp->length), &sum);
        ok = (getEtherChecksum(sum) == 0);
    }
    return ok;
}

// Gets pointer to UDP payload of frame
uint8_t * etherGetUdpData(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = (ip->revSize & 0xF) * 4;
    udpHeader *udp = (udpHeader*)((uint8_t*)ip + ipHeaderLength);
    return udp->data;
}

// Send responses to a udp datagram 
// destination port, ip, and hardware address are extracted from provided data
// uses destination port of received packet as destination of this packet
void etherSendUdpResponse(etherHeader *ether, uint8_t *udpData, uint8_t udpSize)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = (ip->revSize & 0xF) * 4;
    udpHeader *udp = (udpHeader*)((uint8_t*)ip + ipHeaderLength);
    uint16_t udpLength;
    uint8_t *copyData;
    uint32_t sum = 0;
    uint8_t i, tmp8;
    uint16_t tmp16;
    // swap source and destination fields
    for (i = 0; i < HW_ADD_LENGTH; i++)
    {
        tmp8 = ether->destAddress[i];
        ether->destAddress[i] = ether->sourceAddress[i];
        ether->sourceAddress[i] = tmp8;
    }
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        tmp8 = ip->destIp[i];
        ip->destIp[i] = ip->sourceIp[i];
        ip->sourceIp[i] = tmp8;
    }
    // set source port of resp will be dest port of req
    // dest port of resp will be left at source port of req
    // unusual nomenclature, but this allows a different tx
    // and rx port on other machine
    udp->sourcePort = udp->destPort;
    // adjust lengths
    udpLength = 8 + udpSize;
    ip->length = htons(ipHeaderLength + udpLength);
    // 32-bit sum over ip header
    etherCalcIpChecksum(ip);
    // set udp length
    udp->length = htons(udpLength);
    // copy data
    copyData = udp->data;
    for (i = 0; i < udpSize; i++)
        copyData[i] = udpData[i];
    // 32-bit sum over pseudo-header
    sum = 0;
    etherSumWords(ip->sourceIp, 8, &sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    etherSumWords(&udp->length, 2, &sum);
    // add udp header
    udp->check = 0;
    etherSumWords(udp, udpLength, &sum);
    udp->check = getEtherChecksum(sum);

    // send packet with size = ether + udp hdr + ip header + udp_size
    etherPutPacket(ether, sizeof(etherHeader) + ipHeaderLength + udpLength);
}

// Determines whether packet is DHCP
// Must be a UDP packet
//bool etherIsDhcpResponse(etherHeader* ether)
//{
//    ipHeader *ip = (ipHeader*)ether->data;
//    uint8_t ipHeaderLength = (ip->revSize & 0xF) * 4;
//    udpHeader* udp = (udpHeader*)((uint8_t*)ip + ipHeaderLength);
//    bool ok;
//    ok = (udp->sourcePort == htons(67)) & (udp->destPort == htons(68));
//    return ok;
//}

// Determines whether packet is TCP packet
// Must be an IP packet


uint16_t etherGetId(void)
{
    return htons(sequenceId);
}

void etherIncId(void)
{
    sequenceId++;
}

// Determines if the IP address is valid
bool etherIsIpValid()
{
    return ipAddress[0] || ipAddress[1] || ipAddress[2] || ipAddress[3];
}

void etherSetIpAddress(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3)
{
    ipAddress[0] = ip0;
    ipAddress[1] = ip1;
    ipAddress[2] = ip2;
    ipAddress[3] = ip3;
}

// Gets IP address
void etherGetIpAddress(uint8_t ip[4])
{
    uint8_t i;
    for (i = 0; i < 4; i++)
        ip[i] = ipAddress[i];
}

// Sets IP subnet mask
void etherSetIpSubnetMask(uint8_t mask0, uint8_t mask1, uint8_t mask2, uint8_t mask3)
{
    ipSubnetMask[0] = mask0;
    ipSubnetMask[1] = mask1;
    ipSubnetMask[2] = mask2;
    ipSubnetMask[3] = mask3;
}

// Gets IP subnet mask
void etherGetIpSubnetMask(uint8_t mask[4])
{
    uint8_t i;
    for (i = 0; i < 4; i++)
        mask[i] = ipSubnetMask[i];
}

void etherSetHtmlIpAddress(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3)
{
    html_ip_address[0] = ip0;
    html_ip_address[1] = ip1;
    html_ip_address[2] = ip2;
    html_ip_address[3] = ip3;
}
void etherGetHtmlIpAddress(uint8_t ip[4])
{
    uint8_t i;
       for (i = 0; i < 4; i++)
           ip[i] = html_ip_address[i];
}

// Sets IP gateway address
void etherSetIpGatewayAddress(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3)
{
    ipGwAddress[0] = ip0;
    ipGwAddress[1] = ip1;
    ipGwAddress[2] = ip2;
    ipGwAddress[3] = ip3;
}

// Gets IP gateway address
void etherGetIpGatewayAddress(uint8_t ip[4])
{
    uint8_t i;
    for (i = 0; i < 4; i++)
        ip[i] = ipGwAddress[i];
}

// Sets MAC address
void etherSetMacAddress(uint8_t mac0, uint8_t mac1, uint8_t mac2, uint8_t mac3, uint8_t mac4, uint8_t mac5)
{
    hwAddress[0] = mac0;
    hwAddress[1] = mac1;
    hwAddress[2] = mac2;
    hwAddress[3] = mac3;
    hwAddress[4] = mac4;
    hwAddress[5] = mac5;
    etherSetBank(MAADR0);
    etherWriteReg(MAADR5, mac0);
    etherWriteReg(MAADR4, mac1);
    etherWriteReg(MAADR3, mac2);
    etherWriteReg(MAADR2, mac3);
    etherWriteReg(MAADR1, mac4);
    etherWriteReg(MAADR0, mac5);
}

// Gets MAC address
void etherGetMacAddress(uint8_t mac[6])
{
    uint8_t i;
    for (i = 0; i < 6; i++)
        mac[i] = hwAddress[i];
}
void etherSetDestMacAddress(uint8_t mac0, uint8_t mac1, uint8_t mac2, uint8_t mac3, uint8_t mac4, uint8_t mac5)
{
    destMacAddress[0] = mac0;
    destMacAddress[1] = mac1;
    destMacAddress[2] = mac2;
    destMacAddress[3] = mac3;
    destMacAddress[4] = mac4;
    destMacAddress[5] = mac5;

}

// Gets MAC address
void etherGetDestMacAddress(uint8_t mac[6])
{
    uint8_t i;
    for (i = 0; i < 6; i++)
        mac[i] = destMacAddress[i];
}

