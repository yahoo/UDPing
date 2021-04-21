// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include "statswriter.h"
#include "options.h"
#include "protocol.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

char DEFAULT_METRIC_NAME[] = "udping";

StatsWriterSet::StatsWriterSet (StatsWriter* writer, string guid, string peer, int port) {
    this->writer = writer;
    this->guid = guid;
    this->peer = peer;
    this->port = port;
}

void StatsWriterSet::writeStats (Stats* stats) {
    this->writer->writeStats(this->guid, this->peer, this->port, stats);
}

StatsWriter::StatsWriter (string listen_hostname, string receive_hostname, int listen_port, char* statsdInfo, char* statsdTagInfo, char* statsdTagMetric, int quiet) {
    this->listen_port = listen_port;
    this->quiet = quiet;

    struct in_addr receive_addr;
    struct hostent* ent = NULL;
    if (inet_aton(receive_hostname.c_str(), &receive_addr)) {
        ent = gethostbyaddr(&receive_addr, sizeof(struct in_addr), AF_INET);
    }
    if (ent) {
        int n = strlen(ent->h_name);
        this->receive_hostname = new char[n + 1];
        strncpy(this->receive_hostname, ent->h_name, n);
        this->receive_hostname[n] = 0;
    } else {
        int n = receive_hostname.length();
        this->receive_hostname = new char[n + 1];
        strncpy(this->receive_hostname, receive_hostname.c_str(), n);
        this->receive_hostname[n] = 0;
    }

    char* c = this->receive_hostname;
    while (*c) {
        if (*c == '.') {
            *c = '_';
        }
        c++;
    }

    if (statsdInfo != NULL || statsdTagInfo != NULL) {
        this->fd = makeSocket(listen_hostname, 0);
    }

    if (statsdInfo != NULL) {
        int sdPort = 8125;
        int n = strlen(statsdInfo);
        char* sdInfo = new char[n + 1];
        strncpy(sdInfo, statsdInfo, n);
        sdInfo[n] = 0;
        char* portPointer = strchr(sdInfo, ':');
        if (portPointer) {
            *portPointer = 0;
            sdPort = atoi(portPointer + 1);
        }
        this->statsd_sa = getSockAddr(sdInfo, sdPort);
    }

    if (statsdTagInfo != NULL) {
        int sdPort = 8125;
        int n = strlen(statsdTagInfo);
        char* sdInfo = new char[n + 1];
        strncpy(sdInfo, statsdTagInfo, n);
        sdInfo[n] = 0;
        char* portPointer = strchr(sdInfo, ':');
        if (portPointer) {
            *portPointer = 0;
            sdPort = atoi(portPointer + 1);
        }
        this->tags_sa = getSockAddr(sdInfo, sdPort);
    }

    if (statsdTagMetric == NULL) {
        this->tags_metric = DEFAULT_METRIC_NAME;
    } else {
        int n = strlen(statsdTagMetric);
        this->tags_metric = new char[n + 1];
        strncpy(this->tags_metric, statsdTagMetric, n);
        this->tags_metric[n] = 0;
    }
}

void StatsWriter::writeStats (string guid, string peer, int port, Stats *stats) {
    int n = peer.length();
    char* peerCopy = new char[n + 1];
    strncpy(peerCopy, peer.c_str(), n);
    peerCopy[n] = 0;
    char* c = peerCopy;
    while (*c) {
        if (*c == '.') {
            *c = '_';
        }
        c++;
    }

    if (!this->quiet) {
        writeConsoleStats(guid, peerCopy, port, stats);
    }
    if (this->statsd_sa) {
        writeStatsdStats(guid, peerCopy, port, stats);
    }
    if (this->tags_sa) {
        writeTagsStats(guid, peerCopy, port, stats);
    }
}

void StatsWriter::writeConsoleStats (string guid, string peer, int port, Stats* stats) {
    printf ("{");
    printf ("\"from_host\":\"%s\",", peer.c_str());
    printf ("\"from_port\":\"%d\",", port);
    printf ("\"to_host\":\"%s\",", this->receive_hostname);
    printf ("\"to_port\":\"%d\",", this->listen_port);
    printf ("\"guid\":\"%s\",", guid.c_str());
    printf ("\"count\":\"%d\",", stats->getCount());
    printf ("\"targetCount\":\"%d\",", stats->getTargetCount());
    printf ("\"sum\":\"%lf\",", stats->getSum());
    printf ("\"sumOfSquares\":\"%lf\",", stats->getSumOfSquares());
    printf ("\"max\":\"%lf\"", stats->getMax());
    printf ("}\n");
}

void StatsWriter::writeStatsdStats (string guid, string peer, int port, Stats* stats) {
    stringstream stat("");
    stat << peer << "." << port << "." << this->receive_hostname << "." << this->listen_port << "." << "count" << ":" << stats->getCount() << "|c" << endl;
    stat << peer << "." << port << "." << this->receive_hostname << "." << this->listen_port << "." << "targetCount" << ":" << stats->getTargetCount() << "|c" << endl;
    stat << peer << "." << port << "." << this->receive_hostname << "." << this->listen_port << "." << "sum" << ":" << stats->getSum() << "|c" << endl;
    stat << peer << "." << port << "." << this->receive_hostname << "." << this->listen_port << "." << "sos" << ":" << stats->getSumOfSquares() << "|c" << endl;
    stat << peer << "." << port << "." << this->receive_hostname << "." << this->listen_port << "." << "max" << ":" << stats->getMax() << "|g" << endl;
    sendto(this->fd, stat.str().c_str(), stat.str().length(), 0, this->statsd_sa, sizeof(sockaddr));
}

void StatsWriter::writeTagsStats (string guid, string peer, int port, Stats* stats) {
    stringstream stat("");
    stat << this->tags_metric << ".count,from_host=" << peer << ",from_port=" << port << ",to_host=" << this->receive_hostname << ",to_port=" << this->listen_port << ":" << stats->getCount() << "|c" << endl;
    stat << this->tags_metric << ".targetCount,from_host=" << peer << ",from_port=" << port << ",to_host=" << this->receive_hostname << ",to_port=" << this->listen_port << ":" << stats->getTargetCount() << "|c" << endl;
    stat << this->tags_metric << ".sum,from_host=" << peer << ",from_port=" << port << ",to_host=" << this->receive_hostname << ",to_port=" << this->listen_port << ":" << stats->getSum() << "|c" << endl;
    stat << this->tags_metric << ".sos,from_host=" << peer << ",from_port=" << port << ",to_host=" << this->receive_hostname << ",to_port=" << this->listen_port << ":" << stats->getSumOfSquares() << "|c" << endl;
    stat << this->tags_metric << ".max,from_host=" << peer << ",from_port=" << port << ",to_host=" << this->receive_hostname << ",to_port=" << this->listen_port << ":" << stats->getMax() << "|g" << endl;
    sendto(this->fd, stat.str().c_str(), stat.str().length(), 0, this->tags_sa, sizeof(sockaddr));
}