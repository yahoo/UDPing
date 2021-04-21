// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <err.h>
#include "options.h"
#include "delay.h"
#include "protocol.h"
#include "serversession.h"
#include "statswriter.h"

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
    setOptions(new Options(argc, argv, "p:k:l:m:r:s:t:vq"));

    stringstream usage;
    usage << "Usage: udping_server -l <listen hostname> -p <port number> -k <keepalive interval seconds> [-r <receive hostname>]" << endl <<
             "           [-s <statsd host:port>] [-t <tag based statsd host:port>] [-m <tag based statds metric name>] [-v] [-q]" << endl <<
             "-l: address to bind udp listener to" << endl <<
             "-p: udp port to bind listener to" << endl <<
             "-k: seconds to wait for delayed pings to show up before publishing stats" << endl <<
             "-r: address to report stats as being received by, if different from -l" << endl <<
             "-s: statsd host:port (default port 8125)" << endl <<
             "-t: tag based statsd host:port (default port 8125)" << endl <<
             "-m: tag based statsd metric name (default udping)" << endl <<
             "-v: verbose" << endl <<
             "-q: quiet" << endl;
    string listen_host = getOptions()->getStringOption('l', 1, usage.str(), "Hostname not provided\n");
    int listen_port = getOptions()->parseIntOption('p', 1, 1024, 65536, usage.str(), "Port number is out of range 1024-65532\n");
    int keepalive = getOptions()->parseIntOption('k', 1, 1, 60, usage.str(), "Keepalive interval is out of range 1-60s\n");

    char* rh = getOptions()->getStringOption('r', 0, usage.str(), "");
    string receive_host = rh == NULL ? listen_host : rh;

    /* Establish Stats Writer(s) */
    char* statsdInfo = getOptions()->getStringOption('s', 0, usage.str(), "");
    char* statsdTagInfo = getOptions()->getStringOption('t', 0, usage.str(), "");
    char* statsdTagMetric = getOptions()->getStringOption('m', 0, usage.str(), "");
    int quiet = getOptions()->getQuiet();
    StatsWriter* writer = new StatsWriter(listen_host, receive_host, listen_port, statsdInfo, statsdTagInfo, statsdTagMetric, quiet);

    /* Start Session Manager */
    ServerSessionManager sessionManager(writer, keepalive);

    /* Periodic check to see if packets are received */
    setIntervalTimer(keepalive, handleAlarm);

    int fd = makeSocket (listen_host, listen_port);
    if (fd < 0) {
        err (1, "Failed to create socket");
    }
    while (1) {
        sessionManager.readNextPacket (fd);
    }
    return(0);
}
