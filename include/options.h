// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
//
#include <string>

using namespace std;

class Options {
    private:
        char* options[256];
    public:
        Options (int argc, char** argv, string optstring);
        int parseIntOption (char index, int required, int min, int max, string usage, string error);
        char* getStringOption (char index, int required, string usage, string error);
        int getFlagOption (char index);
        
        int getQuiet ();
        int getVerbose ();
};

void setOptions(Options* options);
Options* getOptions ();
