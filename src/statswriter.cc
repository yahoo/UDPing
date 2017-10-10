// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include "statswriter.h"
#include "options.h"
#include "protocol.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <unistd.h>

int StatsStatsdWriter::fd = 0;
sockaddr* StatsStatsdWriter::sa = 0;

StatsWriterSet::StatsWriterSet (string guid, string peer, int port) {
    this->guid = guid;
    this->peer = peer;
    this->port = port;
}

void StatsWriterSet::writeStats (Stats* stats) {
    char hostname[MAX_PEER + 1];
    gethostname(hostname, MAX_PEER + 1);
    char* statsdInfo = getOptions()->getStringOption('s', 0, "", "");
    int localPort = getOptions()->parseIntOption('p', 1, 1024, 65536, "", "");
    if (statsdInfo > (char*) 1) {
        StatsStatsdWriter::writeStats(stats, peer, port, hostname, localPort, statsdInfo);
    }
    if (!getOptions()->getQuiet()) {
        StatsConsoleWriter::writeStats(stats, guid, peer, hostname, port);
    }
}

void StatsConsoleWriter::writeStats (Stats* stats, string guid, string peer, string hostname, int port) {
    printf ("{");
    printf ("\"from_host\":\"%s\",", peer.c_str());
    printf ("\"to_host\":\"%s\",", hostname.c_str());
    printf ("\"port\":\"%d\",", port);
    printf ("\"guid\":\"%s\",", guid.c_str());
    printf ("\"count\":\"%d\",", stats->getCount());
    printf ("\"targetCount\":\"%d\",", stats->getTargetCount());
    printf ("\"sum\":\"%lf\",", stats->getSum());
    printf ("\"sumOfSquares\":\"%lf\",", stats->getSumOfSquares());
    printf ("\"max\":\"%lf\"", stats->getMax());
    printf ("}\n");
}

void StatsStatsdWriter::writeStats (Stats* stats, string peer, int port, string hostname, int localPort, char* statsdInfo) {
    if (!fd) {
        int sdPort = 8125;
        int s;
        char* sdInfo = new char[strlen(statsdInfo)];
        string host = getOptions()->getStringOption('l', 1, "", "");
        strcpy(sdInfo, statsdInfo);
        char* portPointer = strchr(sdInfo, ':');
        if (portPointer) {
            *portPointer = 0;
            sdPort = atoi(portPointer + 1);
        }
        fd = makeSocket(host, 0);
        sa = getSockAddr(sdInfo, sdPort);
        printf ("Sending to statsd at: %s:%d\n", sdInfo, sdPort);
        delete sdInfo;
    }
    string peerCopy = peer;
    string hostnameCopy = hostname;
    char* c;
    while (c = (char*) strchr(peerCopy.c_str(), '.')) {
        *c = '_';
    }
    while (c = (char*) strchr(hostnameCopy.c_str(), '.')) {
        *c = '_';
    }
    stringstream stat("");
    stat << peerCopy << "." << port << "." << hostnameCopy << "." << localPort << "." << "count" << ":" << stats->getCount() << "|c" << endl;
    stat << peerCopy << "." << port << "." << hostnameCopy << "." << localPort << "." << "targetCount" << ":" << stats->getTargetCount() << "|c" << endl;
    stat << peerCopy << "." << port << "." << hostnameCopy << "." << localPort << "." << "sum" << ":" << stats->getSum() << "|c" << endl;
    stat << peerCopy << "." << port << "." << hostnameCopy << "." << localPort << "." << "sos" << ":" << stats->getSumOfSquares() << "|c" << endl;
    stat << peerCopy << "." << port << "." << hostnameCopy << "." << localPort << "." << "max" << ":" << stats->getMax() << "|g" << endl;
    sendto(fd, stat.str().c_str(), stat.str().length(), 0, sa, sizeof(sockaddr));
}
