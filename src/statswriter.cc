// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include "statswriter.h"
#include <iostream>

StatsConsoleWriter::StatsConsoleWriter (string guid, string peer, string hostname, int port) {
    this->guid = guid;
    this->peer = peer;
    this->hostname = hostname;
    this->port = port;
}

void StatsConsoleWriter::writeStats (Stats* stats) {
    cout << "{" 
         << "\"from_host\":\"" << this->peer.c_str() << "\","
         << "\"to_host\":\"" << this->hostname.c_str() << "\","
         << "\"port\":\"" << this->port << "\","
         << "\"guid\":\"" << this->guid.c_str() << "\","
         << "\"count\":\"" << stats->getCount() << "\","
         << "\"targetCount\":\"" << stats->getTargetCount() << "\","
         << "\"sum\":\"" << stats->getSum() << "\","
         << "\"sumOfSquares\":\"" << stats->getSumOfSquares() << "\","
         << "\"max\":\"" << stats->getMax()
         << "}" << endl << flush;
}
