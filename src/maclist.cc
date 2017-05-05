// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <string.h>
#include <stdio.h>
#include "maclist.h"
#include <iostream>

MacList::MacList (string macListString) {
    const char* mls = macListString.c_str();
    numMacs = 1;
    macs = 0;
    const char* nextString = mls;
    while ((nextString = strchr(nextString + 1, ',')) != 0) {
        numMacs++;
    }
    macs = new uint8_t *[numMacs];
    nextString = mls;
    for (curMac = 0; curMac < numMacs; curMac++) {
        macs[curMac] = new uint8_t[12];
        memset (macs[curMac], 0, 6);
        int numFields = sscanf(mls, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
                        &(macs[curMac][0]),
                        &(macs[curMac][1]),
                        &(macs[curMac][2]),
                        &(macs[curMac][3]),
                        &(macs[curMac][4]),
                        &(macs[curMac][5]));
        if (numFields < 6) {
            cerr << "Invalid MAC string: " << macListString << endl;
            numMacs = 0;
            return;
        };
        mls = strchr (mls, ',') + 1;
    }
}

MacList::~MacList () {
    if (!macs) {
        return;
    }
    for (curMac = 0; curMac < numMacs; curMac++) {
        delete macs[curMac];
    }
    delete macs;
}

int MacList::getNumMacs() {
    return numMacs;
}

uint8_t *MacList::nextMac() {
    if (numMacs == 0) return 0;
    if (++curMac >= numMacs) curMac = 0;
    return macs[curMac];
}

int MacList::toString (uint8_t *mac, char* buffer) {
    if (mac == 0) {
        strcpy (buffer, "NULL");
        return -1;
    }
    return sprintf (buffer, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

