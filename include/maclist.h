// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <stdint.h>
#include <string>

using namespace std;

#ifndef MACLIST__H
#define MACLIST__H

class MacList {
    private:
        int numMacs;
        int curMac;
        uint8_t **macs;

    public:
        MacList (string macListString);
        ~MacList ();
        int getNumMacs();
        uint8_t *nextMac();
        int toString (uint8_t *mac, char* buffer);
};

#endif
