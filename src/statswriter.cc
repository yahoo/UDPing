// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include "statswriter.h"
#include "options.h"
#include <cstdio>

StatsWriterSet::StatsWriterSet (string guid, string peer, int port) {
    this->guid = guid;
    this->peer = peer;
    this->port = port;
}

void StatsWriterSet::writeStats (Stats* stats) {
    char hostname[MAX_PEER + 1];
    gethostname(hostname, MAX_PEER + 1);
    if (!getOptions()->getQuiet()) {
        StatsConsoleWriter cw (guid, peer, hostname, port);
        cw.writeStats(stats);
    }
}

StatsConsoleWriter::StatsConsoleWriter (string guid, string peer, string hostname, int port) {
    this->guid = guid;
    this->peer = peer;
    this->hostname = hostname;
    this->port = port;
}

void StatsConsoleWriter::writeStats (Stats* stats) {
    printf ("{");
    printf ("\"from_host\":\"%s\",", this->peer.c_str());
    printf ("\"to_host\":\"%s\",", this->hostname.c_str());
    printf ("\"port\":\"%d\",", this->port);
    printf ("\"guid\":\"%s\",", this->guid.c_str());
    printf ("\"count\":\"%d\",", stats->getCount());
    printf ("\"targetCount\":\"%d\",", stats->getTargetCount());
    printf ("\"sum\":\"%lf\",", stats->getSum());
    printf ("\"sumOfSquares\":\"%lf\",", stats->getSumOfSquares());
    printf ("\"max\":\"%lf\"", stats->getMax());
    printf ("}\n");
}
