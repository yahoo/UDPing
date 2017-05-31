// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <time.h>
#include <constants.h>
#include <stdint.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <string>

#ifndef PROTOCOL_H
#define PROTOCOL_H

using namespace std;

typedef struct {
    uint8_t protoVersion;
    time_t clientStartTime; // Holds the creation time for the client session - used by the server to differentiate between runs
    char guid[MAX_GUID + 1];
    seqnum_t seqNum; // This should be some flavor of uint so that the math works out when it wraps.
    struct timespec sent;
    uint32_t size;
} packet;

typedef struct {
    packet header;
    char padding[MAXBUF];
} paddedPacket;


int getSeqNum (packet* ph);
void dumpBuffer (char* buf);
int makeSocket (string host, int port);
struct sockaddr* getSockAddr (string host, int port);
int sendMessage(
                int socketFD, 
                int ifIndex,
                uint8_t *srcMacHex,
                uint32_t srcIp,
                int srcPort,
                uint8_t *dstMacHex,
                uint32_t dstIp,
                int dstPort,
                const char* message,
                int datalen,
                int checksumLength);

uint16_t checksum (uint16_t *addr, int len);
uint16_t udp4_checksum2 (struct ip iphdr, struct udphdr udphdr, uint16_t *payload, int payloadlen);

struct frame {
    // Ethernet Frame Fields
    uint8_t dstMac[6];
    uint8_t srcMac[6];
    uint8_t proto[2];
    // Payload
    struct ip iphdr;
    struct udphdr udphdr;
    char message [IP_MAXPACKET];
} __attribute__((packed));

int buildFrame (struct frame *etherFrame, uint8_t *srcMac, uint8_t *dstMac, uint32_t srcIp, uint32_t dstIp, int srcPort, int dstPort, const char *message, int datalen, int checksumLength);

#endif
