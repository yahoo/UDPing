// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include "stats.h"
#include "constants.h"
#include "protocol.h"
#include <string>
#include <set>

#ifndef STATSWRITER__H
#define STATSWRITER__H 1

using namespace std;

class StatsConsoleWriter {
    public:
        static void writeStats (Stats* stats, string guid, string peer, string hostname, int port);
};

class StatsStatsdWriter {
    private:
        static int fd;
        static sockaddr* sa;
    public:
        static void writeStats (Stats* stats, string peer, int port, string hostname, int localPort, char* statsdInfo);
};

class StatsWriterSet {
    private:
        string guid;
        string peer;
        int port;
    public:
        StatsWriterSet (string guid, string peer, int port);
        void writeStats (Stats* stats);
};

#endif
