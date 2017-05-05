// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
/* Make sure seriesTitle is null terminated */
#include "stats.h"
#include <cfloat>

Stats::Stats () {
    targetCount = 0;
    count = 0;
    maxIndex = 0;
    sum = 0;
    sumOfSquares = 0;
    max = -DBL_MAX;
}

int Stats::addDataPoint (int index, double datapoint) {
    count++;
    if (maxIndex < index) {
        maxIndex = index;
    }
    sum += datapoint;
    sumOfSquares += datapoint * datapoint;
    if (max < datapoint) {
        max = datapoint;
    }
    return count;
}
