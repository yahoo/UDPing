// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include "options.h"
#include "delay.h"
#include "clientsession.h"
#include "maclist.h"
#include <sstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

using namespace std;

int main( int argc, char **argv ) {
    if (getuid()) {
        cerr << "udping_client must run as root" << endl;
        exit (-1);
    }
    struct timespec iter_start;
    double wall_elapsed = 0;
    setOptions(new Options(argc, argv, "l:r:p:d:i:s:n:m:a:vhq"));
    
    /* Parse command line options */
    stringstream usage;
    usage << "Usage:  udping_client -r <remote hostname> -p <remote port> -d <delay> -l <local IP> -s <starting port>|<source port descriptor> [-n <number of ports>] -i <measurement interval seconds> -m <max packet size> -a <next-hop MAC,...> [-v] [-q]" << endl <<
    "  Source port descriptor should be a set of ranges separated by commas.\n  Each range can either be a port, or a range of ports separated by a dash.\n  For example:" << endl <<
    "    5000 -> port 5000" << endl <<
    "    5000-5009 -> ports 5000,5001,...,5009" << endl <<
    "    5000,5005,5010 -> ports 5000,5005,5010" << endl << 
    "    5000-5001,5005-5006 -> ports 5000,5001,5005,5006" << endl <<
    "  If a single port is specified AND a number of ports is specified, then source traffic\n  from the count specified starting with the starting port" << endl;
    if (getOptions()->getFlagOption('h')) {
        cerr << usage.str() << endl;
        exit (0);
    }
    string dstHost = getOptions()->getStringOption ('r', 1, usage.str(), "No remote hostname provided\n");
    int dstPort = getOptions()->parseIntOption('p', 1, 1024, 65536, usage.str(), "Port number is out of range 1024-65532\n");
    string srcIp = getOptions()->getStringOption ('l', 1, usage.str(), "No local IP provided\n");
    //int startingPort = getOptions()->parseIntOption('s', 1, MIN_PORT, MAX_PORT, usage.str(), "Starting port is out of range MIN_PORT-MAX_PORT\n");
    string srcPorts = getOptions()->getStringOption('s', 1, usage.str(), "Please define a valid source port");
    int numPorts = getOptions()->parseIntOption('n', 0, 1, MAX_SOCKETS, usage.str(), "Number of ports is out of range 1-MAX_SOCKETS\n");
    if ((atoi(srcPorts.c_str()) > 0) && (numPorts > 0) && (string::npos == srcPorts.find_first_not_of("1234567890"))) {
        int startingPort = atoi(srcPorts.c_str());
        if ((startingPort > MAX_PORT) || (startingPort < MIN_PORT)) {
            cerr << usage.str() << "Starting port is out of range MIN_PORT-MAX_PORT\n";
            exit (-1);
        }
        int endingPort = startingPort + numPorts - 1;
        stringstream buffer;
        buffer << to_string(startingPort) << "-" << to_string(endingPort);
        srcPorts = buffer.str();
    }
    int delay = getOptions()->parseIntOption('d', 1, 0, 1000, usage.str(), "Delay is out of range 0-1000 ms\n");
    int interval = getOptions()->parseIntOption('i', 1, 1, 900, usage.str(), "Measurement interval is out of range 0-900 s\n");
    int maxPacketSize = getOptions()->parseIntOption('m', 1, 1, MAXBUF, usage.str(), "Max packet size is out of range 1-MAXBUF\n");
    string dstMac = getOptions()->getStringOption ('a', 1, usage.str(), "You must specify one or more next-hop MAC addresses\n");

    MacList dstml(dstMac);
    if (dstml.getNumMacs() == 0) {
        cerr << "Invalid next-hop MAC address string" << endl << usage.str();
        exit (-1);
    }

    srand (time(0) + getpid());

    ClientSessionList sessions( srcIp, srcPorts, dstHost, dstPort, dstMac, interval, maxPacketSize );

    if (!getuid()) {
        // I'm running as root.  Switch to nobody or die!!!
        struct passwd* pwd = getpwnam("nobody");
        if (pwd && pwd->pw_uid) {
            if (setgid(pwd->pw_gid)) {
                cerr << "Couldn't change userid to nobody (setgid failed).  Terminating client" << endl;
                exit(-1);
            }
            if (setuid(pwd->pw_uid)) {
                cerr << "Couldn't change userid to nobody (setuid failed).  Terminating client" << endl;
                exit(-1);
            }
        } else {
            cerr << "Couldn't change userid to nobody.  Terminating client" << endl;
            exit(-1);
        }
    }

    int ix;
    while (1) {
        startTimer(&iter_start);
        ClientSession* session = sessions.getNextSession();
        if (session->rollover()) {
            session->reset();
        }
        (void) session->sendPingPacket();
        session->increment();
        doRandomPause(&iter_start, delay);
    };
    return(0);
}
