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
#include <sstream>
#include <iostream>

static const char* hexdigits = "0123456789abcdef";

int isValidPort(int port) {
    if (port < MIN_PORT) {
        cerr << "Port too low: " << port << endl;
        exit(-1);
    }
    if (port > MAX_PORT) {
        cerr << "Port too high: " << port << endl;
        exit(-1);
    }
    return 1;
}

ClientSessionList::ClientSessionList (
                string srcHost, string srcPorts, string dstHost, int dstPort,
                string dstMacList, int interval, int maxPacketSize )
{
    this->numPorts = 0;
    this->maxPacketSize = maxPacketSize;
    this->srcAddr = getSockAddr(srcHost.c_str(), dstPort);
    this->dstAddr = getSockAddr(dstHost.c_str(), dstPort);
    this->interval = interval;
    this->dstPort = dstPort;
    memset (srcMac, 0, 6);
    if (getIfInfo(srcHost.c_str(), &ifIndex, srcMac) < 0) {
        cerr << "getIfInfo failed" << endl;
        exit (-1);
    }
    socketFD = socket (PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (socketFD < 0) {
        cerr << "socket() failed" << endl;
        exit (-1);
    }
    if (string::npos != srcPorts.find_first_not_of("1234567890,-")) {
        cerr << "Invalid source port descriptor " << srcPorts << endl;
        exit (-1);
    }
    char ports [1000];
    strncpy(ports, srcPorts.c_str(), 1000);
    char* curRange;
    char* curRangePtr = 0 ;
    char* pports = ports;
    while (curRange = strtok_r(pports, ",", &curRangePtr)) {
        pports = 0;
        char* startString = curRange;
        char* endString = 0;
        if (endString = strchr(curRange, '-')) {
            *endString = 0;
            endString++;
        }
        int start = atoi(startString);
        int end = start;
        if (endString) {
            end = atoi(endString);
        }
        if (isValidPort(start) && isValidPort(end) && (start <= end)) {
            for (int ix = start; ix <= end; ix++) {
                if (getOptions()->getFlagOption('v')) {
                    cout << "Adding port " << ix << endl;
                }
                sessions.push_back(new ClientSession(ix, dstMacList, this));
                this->numPorts ++;
            }
        } else {
            cerr << "Invalid port range " << start << "-" << end << endl;
            exit(-1);
        }
        if (sessions.size() > MAX_SOCKETS) {
            cerr << "Too many ports defined"<< endl;
            exit (-1);
        }
        if (sessions.size() == 0) {
            cerr << "No ports defined"<< endl;
            exit (-1);
        }
    }
    this->curSession = this->sessions.begin();
}

ClientSessionList::~ClientSessionList () {
    for (curSession = sessions.begin(); curSession != sessions.end(); curSession++) {
        delete *curSession;
    }
    delete srcAddr;
    delete dstAddr;
}

ClientSession* ClientSessionList::getNextSession () {
    ClientSession* nextSession = *curSession;
    curSession++;
    if (curSession == sessions.end()) {
        curSession = sessions.begin();
    }
    //cout << "Fetching session for source port " << nextSession->getPort() << endl;
    return nextSession;
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
    p.header.protoVersion = PROTOVERSION;
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
