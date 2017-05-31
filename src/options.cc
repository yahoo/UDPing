// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>
#include "options.h"
#include <iostream>

extern char* optarg;

Options::Options (int argc, char** argv, string optstring) {
    memset (options, 0, 256 * sizeof(char*));
    int opt;
    while (1) {
        optarg = 0;
        opt = getopt(argc, argv, optstring.c_str());
        if (opt < 0) {
            break;
        } else if (opt == '?') {
            exit(-1);
        }
        if (optarg) {
            options[opt] = new char[strlen(optarg) + 1];
            strcpy(options[opt], optarg);
        } else {
            options[opt] = (char*)1;
        }
    }
}

int Options::parseIntOption (char index, int required, int min, int max, string usage, string error) {
    int ret = 0;
    if (options[index]) {
        ret = atoi(options[index]);
        if ((ret < min) || (ret > max)) {
            cerr << usage << error;
            exit (-1);
        }
    } else {
        if (required) {
            cerr << usage << error;
            exit (-1);
        }
    }
    return ret;
}

char* Options::getStringOption (char index, int required, string usage, string error) {
    char* ret = 0;
    if (options[index] > (char*)1) {
        ret = options[index];
    } else {
        if (required) {
            cerr << usage << error;
            exit (-1);
        }
    }
    return ret;
}

int Options::getFlagOption (char index) {
    if (options[index]) {
        return 1;
    }
    return 0;
}

int Options::getQuiet () {
    return options['q']?1:0;
}

int Options::getVerbose () {
    return options['v']?1:0;
}

static Options* theOptions = NULL;
void setOptions (Options* options) {
    theOptions = options;
}

Options* getOptions () {
    return theOptions;
}
