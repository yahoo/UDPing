### Welcome to the UDPing network measurement tool.

The purpose of UDPing is to measure latency and packet loss across a link.  It does this by sending a continuous stream of packets from a source to a destination, measuring latency and loss at the destination, and periodically outputting summary performance metrics.

UDPing has many useful features, including:
* Efficient implementation - typical sampling rates are 100/second
* Randomized Poisson sampling intervals
* Multiple source ports (to exercise multiple ECMP paths)
* Ability to specify multiple next-hop MAC addresses
* Varied payload sizes
* Supports multiple output formats, including statsd and JSON to stdout 
* True one-way protocol - no information is sent from server to client

## Usage

    udping_client -r <remote hostname> -p <remote port> -d <delay> -l <local IP> 
      -s <starting port>|<source port descriptor> [-n <number of ports>] 
      -i <measurement interval seconds> -m <max packet size> -a <next-hop MAC,...> 
      [-v] [-q]
        * Source port descriptor should be a set of ranges separated by commas.
          Each range can either be a port, or a range of ports separated by a 
          dash.  For example:
            * 5000 -> port 5000
            * 5000-5009 -> ports 5000,5001,...,5009
            * 5000,5005,5010 -> ports 5000,5005,5010
            * 5000-5001,5005-5006 -> ports 5000,5001,5005,5006
        * If a single port is specified AND a number of ports is specified, 
          then source traffic from the number of sequential ports specified 
          starting with the starting port
    
    udping_server -l <local hostname> -p <port number> -k <keepalive interval seconds>
      [-s <statsd host:port>] [-v] [-q]

## How it works

The UDPing client publishes a stream of UDP packets from a client to a server with a header and a varying length payload, with an average interval specified by the -d switch.  The payload size is randomized from zero to MAX - the size of the header, where MAX is specified by the -m switch on the client.  The packets are sent in groups according to a measurement period, specified by the -i switch.  Packets for a measurement period from a single port share a common GUID.  At the end of the measurement period, a control packet is sent from each port with the total packet count for the period.

As the server receives packets, it timestamps them on arrival and records several counters, including
* Count
* Sum of latency
* Sum of squares of latency
* Max latency

Upon receipt of a control packet, the server dumps several metrics to stdout, a statsd receiver, or both, including
* Count
* Expected count
* Sum of latency
* Sum of squares of latency
* Max latency

If the server does not receive a control packet within a keepalive timeout specified by the -k switch, it will output metrics with a zero expected count.

Metrics emitted to stdout are at the source host:port level.  Metrics emitted to statsd can either be at the host level only, or at the host level and the host:port level, as specified in the -s switch on the server.

At least one next-hop MAC address must be specified with the -a switch on the client.  This should ordinarily be set to the MAC address of the default gateway for the client.  If more than one egress path is desired, multiple next-hops can be separated by commas.

## Protocol

The header is defined by the following struct:

    typedef struct {
        uint8_t protoVersion;      // Allow for evolution of protocol
        time_t clientStartTime;    // Holds the creation time for the client session - used by the server to differentiate between runs
        char guid[MAX_GUID + 1];   // Unique identifier for a stream of packets
        seqnum_t seqNum;           // Monotonically increasing sequence number per client port
        struct timespec sent;      // High-resolution sender timestamp (resolution dependent on system clock resolution)
        uint32_t size;             // Size of the packet, including the header and the payload
    } packet;

A normal packet also includes a payload of random size, up to MAX - sizeof(packet), where MAX is specified by the -m switch on the client.

This code is licensed under Apache License 2.0 as per the license file and contains code from the reference implementation in RFC 1071.

