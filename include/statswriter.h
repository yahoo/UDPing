// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include "stats.h"
#include "constants.h"
#include <string>

#ifndef STATSWRITER__H
#define STATSWRITER__H 1

using namespace std;

class StatsWriter {
    public:
        virtual void writeStats (Stats* stats) = 0;
};

class StatsConsoleWriter : public StatsWriter {
    private:
        string guid;
        string peer;
        string hostname;
        int port;
    public:
        StatsConsoleWriter (string guid, string peer, string hostname, int port);
        void writeStats (Stats* stats);
};

#endif
