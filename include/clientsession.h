// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <time.h>
#include <netdb.h>      /* gethostbyname */
#include <constants.h>
#include "protocol.h"
#include "maclist.h"
#include <list>
#include <string>

class ClientSession;

class ClientSessionList {
    private:
        int numPorts;
        int maxPacketSize;
        list<ClientSession*> sessions;
        list<ClientSession*>::iterator curSession;
        struct sockaddr* srcAddr;
        struct sockaddr* dstAddr;
        int ifIndex;
        uint8_t srcMac[6];
        int socketFD;
        int interval;
        int dstPort;
    public:
        ClientSessionList (
                string srcIp, string srcPorts, string dstHost, int dstPort,
                string dstMacList, int interval, int maxPacketSize 
        );
        ~ClientSessionList ();
        ClientSession* getNextSession();
        struct sockaddr_in* getDstAddr() {return (struct sockaddr_in*)dstAddr;};
        struct sockaddr_in* getSrcAddr() {return (struct sockaddr_in*)srcAddr;};
        int getMaxPacketSize() {return maxPacketSize;};
        int getSocketFD() {return socketFD;};
        int getInterval() {return interval;};
        int getIfIndex() {return ifIndex;};
        uint8_t *getSrcMac() {return srcMac;};
        int getDstPort() {return dstPort;};
};

class ClientSession {
    private:
        paddedPacket p;
        int port;
        time_t lastRollover;
        MacList dstMacList;
        ClientSessionList* parent;
    public:
        ClientSession(int port, string dstMacs, ClientSessionList* parent);
        ~ClientSession();
        bool rollover();
        void sendControlPacket();
        void sendPingPacket();
        void reset();
        void increment();
        int getPort () {return port;};
    private:
        int sendPacket (int size);
        int fillControlPacket ();
};

