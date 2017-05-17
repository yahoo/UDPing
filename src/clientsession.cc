// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <string.h>
#include <stdlib.h>
#include <err.h>

#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD

#include "stats.h"
#include "options.h"
#include "clientsession.h"
#include "ifinfo.h"
#include <cassert>

static const char* hexdigits = "0123456789abcdef";

ClientSessionList::ClientSessionList (
                string srcHost, int startingPort, int numPorts, string dstHost, 
                int dstPort, string dstMacList, int interval, int maxPacketSize )
{
    assert (numPorts > 0);
    struct addrinfo* addrinfo;
    int s;
    this->numPorts = numPorts;
    this->curSession = 0;
    this->maxPacketSize = maxPacketSize;
    this->srcAddr = getSockAddr(srcHost.c_str(), dstPort);
    this->dstAddr = getSockAddr(dstHost.c_str(), dstPort);
    this->interval = interval;
    this->dstPort = dstPort;
    memset (srcMac, 0, 6);
    if (getIfInfo(srcHost.c_str(), &ifIndex, srcMac) < 0) {
        exit (-1);
    }
    socketFD = socket (PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (socketFD < 0) {
        perror ("socket() failed");
        exit (-1);
    }
    sessions = new ClientSession*[numPorts];
    for (int ix = 0; ix < numPorts; ix++) {
        sessions[ix] = new ClientSession(startingPort + ix, dstMacList, this);
    }
}

ClientSessionList::~ClientSessionList () {
    for (int ix = 0; ix < numPorts; ix++) {
        delete sessions[ix];
        sessions[ix] = 0;
    }
    delete sessions;
    delete srcAddr;
    delete dstAddr;
}

ClientSession* ClientSessionList::getNextSession () {
    if (curSession >= numPorts) {
        curSession = 0;
    }
    return sessions[curSession++];
}

void generateGuid (char* buffer, int buflen) {
    int ix;
    for (ix = 0; ix < buflen; ix++) {
        buffer[ix] = hexdigits[(rand() % 16)];
    }
}

ClientSession::ClientSession(int port, string dstMacs, ClientSessionList* parent) :
    dstMacList(dstMacs)
{
    this->port = port;
    this->parent = parent;
    lastRollover = 0;
    memset (&p, 0, sizeof(paddedPacket));
    p.header.clientStartTime = time(0);
    p.header.seqNum = 0;
    reset();
}

ClientSession::~ClientSession () {
}

bool ClientSession::rollover() {
    time_t curTime = time(0);
    curTime -= curTime % parent->getInterval();
    if (!lastRollover) {
        lastRollover = curTime;
    }
    if (curTime == lastRollover) {
        return false;
    }
    lastRollover = curTime;
    return true;
}

/*
 * Resets a client session.  This is done by resetting the sequence
 * number and generating a new GUID.
 */
void ClientSession::reset () {
    generateGuid(p.header.guid, MAX_GUID);
}

/*
 * Increments the sequence number in the session.  The only reason this
 * function exists is to make the calling code readable.
 */
void ClientSession::increment() {
    p.header.seqNum++;
}

/* 
 * This assumes that the pingHeader has already been filled.
 * This function plugs the current sequence number into 
 * the ping header, sets the timestamp, and sends the ping
 */
void ClientSession::sendPingPacket () {
    clock_gettime(CLOCK_REALTIME, &(p.header.sent));
    int size = rand() % this->parent->getMaxPacketSize();
    sendPacket(size);
}

int ClientSession::sendPacket (int size) {
    if (size < sizeof(packet)) size = sizeof(packet);
    p.header.size = size;
    int n_sent = sendMessage (
                parent->getSocketFD(),
                parent->getIfIndex(),
                parent->getSrcMac(),
                parent->getSrcAddr()->sin_addr.s_addr,
                port,
                dstMacList.nextMac(),
                parent->getDstAddr()->sin_addr.s_addr,
                parent->getDstPort(),
                (char*)&p,
                size,
                sizeof(packet));

    if (n_sent < 0) {
        warn ("Problem sending data");
        exit (-1);
    }
    if (n_sent != size) {
        warn("Sendto attempted %d, sent %d bytes\n",size, n_sent);
    }
    return n_sent;
}
