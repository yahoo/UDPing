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

ServerSession::ServerSession (StatsWriter* writer, string peer, int port, packet* p) {
    this->minSeq = p->seqNum;
    this->maxSeq = p->seqNum;
    this->clientStartTime = p->clientStartTime;
    lastArrival = 0;
    stats = new Stats ();
    statsWriters = new StatsWriterSet(writer, p->guid, peer, port);
}

ServerSession::~ServerSession () {
    delete stats;
    delete statsWriters;
}

void ServerSession::writeStats(time_t now) {
    seqnum_t targetCount = 1 + maxSeq - minSeq;
    stats->setTargetCount(targetCount);
    statsWriters->writeStats(stats, now);
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
        ServerSession* ds = new ServerSession(this->writer, peer, port, p);
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
    for (map<string, ServerSession*>::iterator it = sessionMap.begin(); it != sessionMap.end();) {
        ServerSession* ds = it->second;
        if (ds) {
            if (ds->getLastArrival() < now - keepalive) {
                // If the session has a successor, set the max packet index to one before the
                // successor's minimum index to detect gaps in the packet sequence between sessions
                if (sessionMap.find(ds->getSuccessor()) != sessionMap.end()) {
                    ServerSession* successor = sessionMap[ds->getSuccessor()];
                    if (successor) {
                        ds->recordSeq(successor->getMinSeq() - 1);
                    }
                }
                ds->writeStats(now);
                delete ds;
                it = sessionMap.erase(it);
            } else {
                it++;
            }
        } else {
            break;
        }
    }
}

void ServerSessionManager::receivePing (packet* ph, struct timespec* rcvd, string peer, int port) {
    intervalPacketsReceived++;
    ServerSession* ds = getServerSession(peer, port, ph);
    ds->setLastArrival(time(0));
    ds->recordSeq(ph->seqNum);
    double elapsed = rcvd->tv_nsec;
    elapsed -= ph->sent_nsec;
    elapsed /= 1000000000L;
    elapsed += rcvd->tv_sec;
    elapsed -= ph->sent_sec;
    elapsed *= 1000;
    //double elapsed = rcvd->tv_sec - ph->sent_sec + (rcvd->tv_nsec - ph->sent_nsec)/1000000000L;
    ds->getStats()->addDataPoint (elapsed);
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
                hostName = hostMap[remote.sin_addr.s_addr] = inet_ntoa(remote.sin_addr);
            }
        }
        receivePing (ph, &tv, hostName, ntohs(remote.sin_port));
    }
    return n;
}

static ServerSessionManager* theSessionManager = (ServerSessionManager*) NULL;

ServerSessionManager::ServerSessionManager (StatsWriter* writer, int theKeepalive) {
    if (theSessionManager) {
        err(1, "Attempt to create multiple session managers");
    }
    theSessionManager = this;
    intervalPacketsReceived = 0;
    this->writer = writer;
    this->keepalive = theKeepalive;
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

