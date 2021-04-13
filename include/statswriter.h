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

class StatsWriter {
    private:
        char* receive_hostname;
        int listen_port;
        int fd;
        sockaddr* statsd_sa;
        sockaddr* tags_sa;
        char* tags_metric;
        int quiet;
        void writeConsoleStats (string guid, string peer, int port, Stats* stats);
        void writeStatsdStats (string guid, string peer, int port, Stats* stats);
        void writeTagsStats (string guid, string peer, int port, Stats* stats);
    public:
        StatsWriter (string listen_hostname, string receive_hostname, int listen_port, char* statsdInfo, char* statsdTagInfo, char* statsdTagMetric, int quiet);
        void writeStats (string guid, string peer, int port, Stats* stats);
};

class StatsWriterSet {
    private:
        StatsWriter* writer;
        string guid;
        string peer;
        int port;
    public:
        StatsWriterSet (StatsWriter* writer, string guid, string peer, int port);
        void writeStats (Stats* stats);
};

#endif
