// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.

#ifndef STATS__H
#define STATS__H 1

class Stats {
    private:
        int targetCount;
        int maxIndex;
        int count;
        double sum;
        double sumOfSquares;
        double max;

    public:
        Stats ();
        void freeSeries ();
        int addDataPoint (int index, double datapoint);
    
        void setTargetCount (int targetCount) { this->targetCount = targetCount; }
        int getTargetCount () { return targetCount; }
        int getCount () { return count; }
        double getSum () { return sum; }
        double getSumOfSquares () { return sumOfSquares; }
        double getMax () { return max; }
};

#endif
