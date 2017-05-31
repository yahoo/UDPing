// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include "stats.h"
#include "statswriter.h"

#include <string>
#include <map>

using namespace std;

class ServerSession {
    private:
        time_t clientStartTime;
        // Successor is the next serversession for the same host and port.
        // Used to detect missing packets at the end of a stream.
        string successor; 
        time_t lastArrival;
        seqnum_t minSeq;
        seqnum_t maxSeq;
        Stats* stats;
        StatsWriterSet* statsWriters;
    public:
        ServerSession (string peer, int port, packet* p);
        ~ServerSession ();
        void writeStats();
        void recordSeq (seqnum_t seqNum);
        void setSuccessor (string successor) {this->successor = successor;}
        string getSuccessor () {return successor;}
        Stats* getStats () {return stats;}
        void setLastArrival (time_t t) {lastArrival = t;}
        time_t getLastArrival () {return lastArrival;}
        time_t getClientStartTime () {return clientStartTime;}
        seqnum_t getMinSeq () {return minSeq;}
};

class ServerSessionManager {
    private:
        map<string, ServerSession*> sessionMap;
        map<string, ServerSession*> sourceMap;
        map<long, string> hostMap;
        int intervalPacketsReceived;
        int keepalive;
    private:
        ServerSession* getServerSession (string peer, int port, packet* p);
        void sweepServerSessions ();
        void receivePing (packet* ph, struct timespec* rcvd, string peer, int port);
    public:
        ServerSessionManager(int theKeepalive);
        int readNextPacket (int fd);
        void handleAlarm (int sig);
};

void handleAlarm (int sig);

