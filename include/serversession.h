// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include "stats.h"
#include "statswriter.h"

#include <string>
#include <map>

using namespace std;

class ServerSession {
    private:
        string peer;
        string guid;
        string application;
        time_t firstArrival;
        time_t lastArrival;
        int scoreboardLevel;
        int firstSession;
        Stats* stats;
        StatsConsoleWriter* consoleWriter;
    public:
        ServerSession (string peer, string guid, int port, int sourceExists);
        ~ServerSession ();
        void writeStats();
        string getPeer () {return peer;}
        string getGuid () {return guid;}
        Stats* getStats () {return stats;}
        void setLastArrival (time_t t) {lastArrival = t;}
        time_t getLastArrival () {return lastArrival;}
};

class ServerSessionManager {
    private:
        map<string, ServerSession*> sessionMap;
        map<string, string> sourceMap;
        map<long, string> hostMap;
        int intervalPacketsReceived;
        int keepalive;
    private:
        ServerSession* getServerSession (string peer, string guid, int port);
        void sweepServerSessions ();
        void receivePing (packet* ph, struct timespec* rcvd, string peer, int port);
    public:
        ServerSessionManager(int theKeepalive);
        int readNextPacket (int fd);
        int makeSocket (string host, int port);
        void handleAlarm (int sig);
};

void handleAlarm (int sig);

