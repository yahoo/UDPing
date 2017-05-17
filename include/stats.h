// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.

#include "constants.h"

#ifndef STATS__H
#define STATS__H 1

class Stats {
    private:
        seqnum_t targetCount;
        int count;
        double sum;
        double sumOfSquares;
        double max;

    public:
        Stats ();
        void freeSeries ();
        int addDataPoint (double datapoint);
    
        void setTargetCount (seqnum_t targetCount) { this->targetCount = targetCount; }
        seqnum_t getTargetCount () { return targetCount; }
        int getCount () { return count; }
        double getSum () { return sum; }
        double getSumOfSquares () { return sumOfSquares; }
        double getMax () { return max; }
};

#endif
