// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <err.h>
#include <unistd.h>
#include <netdb.h>

#include <map>
#include <string>
#include <sstream>

#include "protocol.h"
#include "options.h"
#include "constants.h"
#include "serversession.h"

ServerSession::ServerSession (string peer, string guid, int port, int sourceExists) {
    this->peer = peer;
    this->guid = guid;
    if (!sourceExists) {
        firstSession = 1;
    } else {
        firstSession = 0;
    }
    firstArrival = 0;
    lastArrival = 0;
    char hostname[MAX_PEER + 1];
    gethostname(hostname, MAX_PEER + 1);
    stats = new Stats ();
    consoleWriter = new StatsConsoleWriter (guid, peer, hostname, port);
}

ServerSession::~ServerSession () {
    delete stats;
    delete consoleWriter;
}

void ServerSession::writeStats() {
    if (!getOptions()->getQuiet()) {
        consoleWriter->writeStats(stats);
    }
}

/*
 * ServerSessionManager methods
 */

ServerSession* ServerSessionManager::getServerSession (string peer, string guid, int port) {
    if (!sessionMap.count(guid)) {
        stringstream source;
        source << peer << ":" << port;
        ServerSession* ds = new ServerSession(peer, guid, port, sourceMap.count(source.str()));
        sessionMap[guid] = ds;
        sourceMap[source.str()] = "";
    }
    return sessionMap[guid];
}

void ServerSessionManager::sweepServerSessions () {
    int ix;
    time_t now = time(0);
    for (map<string, ServerSession*>::iterator it = sessionMap.begin(); it != sessionMap.end(); it++) {
        ServerSession* ds = it->second;
        if (ds->getLastArrival() < now - keepalive) {
            ds->writeStats();
            delete ds;
            sessionMap.erase(it->first);
       }
    }
}


void ServerSessionManager::receivePing (packet* ph, struct timespec* rcvd, string peer, int port) {
    intervalPacketsReceived++;
    ServerSession* ds = getServerSession(peer, ph->guid, port);
    ds->setLastArrival(time(0));
    if (ph->controlPacket) {
        ds->getStats()->setTargetCount (ph->seqNum - 1);
    } else {
        double elapsed = 1000000000L * (rcvd->tv_sec - ph->sent.tv_sec) + rcvd->tv_nsec - ph->sent.tv_nsec;
        ds->getStats()->addDataPoint (getSeqNum(ph), elapsed/1000000);
    }
    if (ds->getStats()->getCount() == ds->getStats()->getTargetCount()) {
        ds->writeStats();
        delete ds;
        sessionMap.erase(ph->guid);
    }
    sweepServerSessions();
}

int ServerSessionManager::readNextPacket (int fd) {
    char buf[MAXBUF];
    struct sockaddr_in remote;
    struct timespec tv;
    socklen_t len = sizeof(remote);
    int n=recvfrom(fd,buf,MAXBUF,0,(struct sockaddr *)&remote,&len);
    packet* ph = (packet*) buf;
    if (n < 0) {
        warn("Error receiving data");
    } else if (n < sizeof(packet)) {
        warn("Malformed packet - size %d is less than header size %d", n, sizeof(packet));
    } else if (n != ph->size) {
        warn("Malformed packet - expected %d, got %d", ph->size, n);
    } else {
        clock_gettime(CLOCK_REALTIME, &tv);
        dumpBuffer(buf);
        string hostName = hostMap[remote.sin_addr.s_addr];
        if (hostName.empty()) {
            struct hostent* remoteName = gethostbyaddr(&(remote.sin_addr), sizeof(struct in_addr), AF_INET);
            hostName = hostMap[remote.sin_addr.s_addr] = remoteName->h_name;
        }
        receivePing (ph, &tv, hostName, ntohs(remote.sin_port));
    }
    return n;
}

static ServerSessionManager* theSessionManager = (ServerSessionManager*) NULL;

ServerSessionManager::ServerSessionManager (int theKeepalive) {
    if (theSessionManager) {
        err(1, "Attempt to create multiple session managers");
    }
    theSessionManager = this;
    intervalPacketsReceived = 0;
    keepalive = theKeepalive;
}

void ServerSessionManager::handleAlarm (int sig) {
    if (!intervalPacketsReceived) {
        sweepServerSessions();
    }
    intervalPacketsReceived = 0;
}

void handleAlarm (int sig) {
    if (theSessionManager) {
        theSessionManager->handleAlarm(sig);
    }
}

int ServerSessionManager::makeSocket (string host, int port) {
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
