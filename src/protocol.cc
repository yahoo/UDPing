// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <arpa/inet.h>
#include <err.h>
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>
#include <net/if.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include "protocol.h"
#include "options.h"
#include "stats.h"
#include "constants.h"

#include <ctime>
#include <cstring>
#include <sstream>

int getSeqNum (packet* ph) {
    return ph->seqNum;
}

void dumpBuffer (char* buf) {
    if (!getOptions()->getVerbose()) {
        return;
    }
    packet* ph = (packet*) buf;
    printf ("%s:%ld:%d - %ld:%ld\n", ph->guid, ph->clientStartTime, ph->seqNum, ph->sent.tv_sec, ph->sent.tv_nsec);
}

int makeSocket (string host, int port) {
    int fd;
    socklen_t length;
    struct addrinfo* addrinfo;
    int s;
    if ((fd = socket( PF_INET, SOCK_DGRAM, 0 )) < 0) {
        err(1, "Problem creating socket");
    }
    
    stringstream portString;
    portString << port;
    if (s = getaddrinfo (host.c_str(), portString.str().c_str(), NULL, &addrinfo)) {
        err(1, "getaddrinfo: %s\n", gai_strerror(s));
    }

    if (bind(fd, addrinfo->ai_addr, sizeof(struct sockaddr))<0) {
        err(1, "Problem binding");
    }
    
    return fd;
}

// Allocates a sockaddr* and returns it.
// It is the responsibility of the caller to delete the return value from this function.
struct sockaddr* getSockAddr (string host, int port) {
    int s;
    struct sockaddr* ret = 0;
    struct addrinfo hints;
    struct addrinfo* addrinfo;
    char portString[100];

    memset (&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    snprintf (portString, 100, "%d", port);

    if (s = getaddrinfo (host.c_str(), portString, &hints, &addrinfo)) {
        err(1, "getaddrinfo: %s\n", gai_strerror(s));
    } else {
        ret = new struct sockaddr;
        memcpy (ret, addrinfo->ai_addr, sizeof(ret));
    }
    freeaddrinfo(addrinfo);
    return ret;
}


#define ETH_HDRLEN 14
#define IP4_HDRLEN 20
#define UDP_HDRLEN  8

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
                int checksumLength) {
    struct sockaddr_ll device;
//    printf ("Sending message <<%s>>\n", message);
    struct frame outboundFrame;
    int frameLength;

    memset (&device, 0, sizeof (device));
    device.sll_ifindex = ifIndex;
    device.sll_family = AF_PACKET;
    memcpy (device.sll_addr, srcMacHex, 6);
    device.sll_halen = 6;

    frameLength = buildFrame(&outboundFrame, srcMacHex, dstMacHex, srcIp, dstIp, srcPort, dstPort, message, datalen, checksumLength);

    uint8_t* frameBuf = (uint8_t*) &outboundFrame;

//    printf ("Packet Dump:\n");
//    for (int ix = 0; ix < frameLength; ix ++) {
//        printf("%02x ", frameBuf[ix]);
//        if (!((ix + 1) % 4)) {
//            printf ("\n");
//        }
//    }
//    printf ("\n\n");

    int bytes = sendto(socketFD, frameBuf, frameLength, 0, (struct sockaddr *) &device, sizeof (device));
    if (bytes != frameLength) {
        perror ("sendto failed");
    }

//    printf ("sendto sent %d of %d bytes!\n", bytes, frameLength);
    return datalen;
}


int buildFrame (struct frame *etherFrame, uint8_t *srcMac, uint8_t *dstMac, uint32_t srcIp, uint32_t dstIp, int srcPort, int dstPort, const char *message, int datalen, int checksumLength) {
    if (checksumLength == 0) {
        checksumLength = datalen;
    }

    // Fill in the Ethernet part
    int frameLength = 6 + 6 + 2 + IP4_HDRLEN + UDP_HDRLEN + datalen;
    memcpy (&(etherFrame->dstMac), dstMac, 6 * sizeof (uint8_t));
    memcpy (&(etherFrame->srcMac), srcMac, 6 * sizeof (uint8_t));
    etherFrame->proto[0] = ETH_P_IP / 256;
    etherFrame->proto[1] = ETH_P_IP % 256;

    // IP Header
    etherFrame->iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);
    etherFrame->iphdr.ip_v = 4;
    etherFrame->iphdr.ip_tos = 0;
    etherFrame->iphdr.ip_len = htons (IP4_HDRLEN + UDP_HDRLEN + datalen);
    etherFrame->iphdr.ip_id = htons (0);
    etherFrame->iphdr.ip_off = 0;
    etherFrame->iphdr.ip_ttl = 255;
    etherFrame->iphdr.ip_p = IPPROTO_UDP;
    etherFrame->iphdr.ip_src.s_addr = srcIp;
    etherFrame->iphdr.ip_dst.s_addr = dstIp;

    etherFrame->iphdr.ip_sum = 0;
    etherFrame->iphdr.ip_sum = checksum ((uint16_t *) &(etherFrame->iphdr), IP4_HDRLEN);

    // UDP Header
    etherFrame->udphdr.source = htons (srcPort);
    etherFrame->udphdr.dest = htons (dstPort);
    etherFrame->udphdr.len = htons (UDP_HDRLEN + datalen);

    etherFrame->udphdr.check = 0;
    // Because the padding at the end of the message is always zeroes, only calculate the checksum on the 
    // header portion of the outbound message
    // Also, use rewritten (zero copy) version of udp checksum.
    uint16_t udpcs2 = udp4_checksum2 (etherFrame->iphdr, etherFrame->udphdr, (uint16_t *)message, checksumLength);
    etherFrame->udphdr.check = udpcs2;
    
    // Payload
    memcpy (&(etherFrame->message), message, datalen);

    return frameLength;
}

// Computing the internet checksum (RFC 1071).
// Note that the internet checksum does not preclude collisions.
uint16_t checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

// This is a zero-copy rewrite of udp4 checksumming algorithm.
uint16_t udp4_checksum2 (struct ip iphdr, struct udphdr udphdr, uint16_t *payload, int payloadlen)
{
    register uint32_t sum = 0;

    sum += (iphdr.ip_src.s_addr & 0xffff);
    sum += (iphdr.ip_src.s_addr >> 16);
    sum += (iphdr.ip_dst.s_addr & 0xffff);
    sum += (iphdr.ip_dst.s_addr >> 16);
    uint16_t tmp = iphdr.ip_p;
    tmp = tmp << 8;
    sum += tmp;
    sum += udphdr.len;
    sum += udphdr.source;
    sum += udphdr.dest;
    sum += udphdr.len;
    while (payloadlen > 1) {
        sum += *(payload++);
        payloadlen -= 2;
    }
    if (payloadlen == 1) {
        uint16_t tmp = *((uint8_t *)payload);
        tmp = tmp << 8;
        sum += tmp;
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return ~sum;
}
