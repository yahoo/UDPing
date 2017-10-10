// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <err.h>
#include "options.h"
#include "delay.h"
#include "protocol.h"
#include "serversession.h"

#include <pwd.h>
#include <sys/types.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>

using namespace std;

int main(int argc, char** argv) {
    if (!getuid()) {
        // I'm running as root.  Switch to nobody or die!!!
        struct passwd* pwd = getpwnam("nobody");
        if (pwd && pwd->pw_uid) {
            if (setgid(pwd->pw_gid)) {
                cerr << "Couldn't change userid to nobody (setgid failed).  Terminating server" << endl;
            }
            if (setuid(pwd->pw_uid)) {
                cerr << "Couldn't change userid to nobody (setuid failed).  Terminating server" << endl;
            }
        } else {
            cerr << "Couldn't change userid to nobody.  Terminating server" << endl;
            exit(-1);
        }
    }
    char buf[MAXBUF];
    setOptions(new Options(argc, argv, "p:k:l:s:vq"));
    
    stringstream usage;
    usage << "Usage: udping_server -l <local hostname> -p <port number> -k <keepalive interval seconds> [-s <statsd host:port>] [-v] [-q]" << endl <<
             "-s: statsd host:port (default port 8125)" << endl <<
             "-v: verbose" << endl <<
             "-q: quiet" << endl;
    int port = getOptions()->parseIntOption('p', 1, 1024, 65536, usage.str(), "Port number is out of range 1024-65532\n");
    string host = getOptions()->getStringOption('l', 1, usage.str(), "Hostname not provided\n");
    int keepalive = getOptions()->parseIntOption('k', 1, 1, 60, usage.str(), "Keepalive interval is out of range 1-60s\n");

    ServerSessionManager sessionManager(keepalive);

    /* Periodic check to see if packets are received */
    setIntervalTimer(keepalive, handleAlarm);

    int fd = makeSocket (host, port);
    if (fd < 0) {
        err (1, "Failed to create socket");
    }
    while (1) {
        sessionManager.readNextPacket (fd);
    }
    return(0);
}
