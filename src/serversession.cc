// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <err.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <map>
#include <string>
#include <sstream>

#include "protocol.h"
#include "options.h"
#include "constants.h"
#include "serversession.h"

ServerSession::ServerSession (string peer, int port, packet* p) {
    this->minSeq = p->seqNum;
    this->maxSeq = p->seqNum;
    this->clientStartTime = p->clientStartTime;
    lastArrival = 0;
    stats = new Stats ();
    statsWriters = new StatsWriterSet(p->guid, peer, port);
}

ServerSession::~ServerSession () {
    delete stats;
    delete statsWriters;
}

void ServerSession::writeStats() {
    seqnum_t targetCount = 1 + maxSeq - minSeq;
    stats->setTargetCount(targetCount);
    statsWriters->writeStats(stats);
}

// This needs work to deal with out of order packets
// Possibly compare the difference between values (maxSeq - seqNum) which will wrap around
void ServerSession::recordSeq(seqnum_t seqNum) {
    seqnum_t diff;
    diff = seqNum - maxSeq;
    if ((diff > 0) && (diff < 10)) {
        maxSeq = seqNum;
    }
    diff = minSeq - seqNum;
    if ((diff > 0) && (diff < 10)) {
        minSeq = seqNum;
    }
}

/*
 * ServerSessionManager methods
 */
ServerSession* ServerSessionManager::getServerSession (string peer, int port, packet* p) {
    if (!sessionMap.count(p->guid)) {
        stringstream source;
        source << peer << ":" << port;
        ServerSession* ds = new ServerSession(peer, port, p);
        // Set the successor for the outgoing session, so that when it times out it checks for
        // a gap in the packet sequence.  This would indicate a lost packet at the end of
        // a session.
        ServerSession* predecessor = sourceMap[source.str()];
        if (predecessor && (predecessor->getClientStartTime() == ds->getClientStartTime())) {
            predecessor->setSuccessor(p->guid);
        }
        sessionMap[p->guid] = ds;
        sourceMap[source.str()] = ds;
    }
    return sessionMap[p->guid];
}

void ServerSessionManager::sweepServerSessions () {
    int ix;
    time_t now = time(0);
    for (map<string, ServerSession*>::iterator it = sessionMap.begin(); it != sessionMap.end(); it++) {
        ServerSession* ds = it->second;
        if (ds && (ds->getLastArrival() < now - keepalive)) {
            // If the session has a successor, set the max packet index to one before the
            // successor's minimum index to detect gaps in the packet sequence between sessions
            if (sessionMap.find(ds->getSuccessor()) != sessionMap.end()) {
                ServerSession* successor = sessionMap[ds->getSuccessor()];
                if (successor) {
                    ds->recordSeq(successor->getMinSeq() - 1);
                }
            }
            ds->writeStats();
            delete ds;
            sessionMap.erase(it->first);
       }
    }
}

void ServerSessionManager::receivePing (packet* ph, struct timespec* rcvd, string peer, int port) {
    intervalPacketsReceived++;
    ServerSession* ds = getServerSession(peer, port, ph);
    ds->setLastArrival(time(0));
    ds->recordSeq(ph->seqNum);
    double elapsed = 1000000000L * (rcvd->tv_sec - ph->sent.tv_sec) + rcvd->tv_nsec - ph->sent.tv_nsec;
    ds->getStats()->addDataPoint (elapsed/1000000);
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
        warn("Malformed packet - size %d is less than header size %ld", n, sizeof(packet));
    } else if (n != ph->size) {
        warn("Malformed packet - expected %d, got %d", ph->size, n);
    } else if (ph->protoVersion != PROTOVERSION) {
        warn("Incorrect protocol version.  Expected %d, got %d", PROTOVERSION, ph->protoVersion);
    } else {
        clock_gettime(CLOCK_REALTIME, &tv);
        dumpBuffer(buf);
        string hostName = hostMap[remote.sin_addr.s_addr];
        if (hostName.empty()) {
            struct hostent* remoteName = gethostbyaddr(&(remote.sin_addr), sizeof(struct in_addr), AF_INET);
	    if (remoteName) {
            	hostName = hostMap[remote.sin_addr.s_addr] = remoteName->h_name;
	    } else {
		hostName = hostMap[remote.sin_addr.s_addr] =  inet_ntoa(remote.sin_addr);
	    }
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

