// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_UDP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/udp.h>      // struct udphdr
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>
#include <ifaddrs.h>
#include <iostream>
#include "ifinfo.h"


#include <errno.h>            // errno, perror()

int getIfInfo (string ip, int* ifIndex, uint8_t *srcMac) {
    char interface [256];
    int sd;
    int foundInterface = 0;
    struct ifreq ifr;
    struct ifaddrs *ifa;
    struct in_addr srcAddr;

    cout << "Getting interface info for " << ip << endl;

    if (inet_pton (AF_INET, ip.c_str(), &srcAddr) != 1) {
         perror ("inet_pton failed");
         return (-1);
    }

    getifaddrs(&ifa);
    struct ifaddrs *curAddr = ifa;
    while (curAddr) {
        struct sockaddr_in* sin = (struct sockaddr_in *) curAddr->ifa_addr;
        if (sin->sin_addr.s_addr == srcAddr.s_addr) {
            printf ("The interface is named %s\n", curAddr->ifa_name);
            strcpy (interface, curAddr->ifa_name);
            foundInterface = 1;
            break;
        }
        curAddr = curAddr->ifa_next;
    }
    freeifaddrs(ifa);

    if (!foundInterface) {
        cerr << "Couldn't find interface for IP " << ip << endl;
        return (-1);
    }

    if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
        perror ("socket() failed to get socket descriptor for using ioctl() ");
        return (-1);
    }

    // Use ioctl() to look up interface name and get its MAC address.
    memset (&ifr, 0, sizeof (ifr));
    snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
    if (ioctl (sd, SIOCGIFHWADDR, &ifr) < 0) {
        perror ("ioctl() failed to get source MAC address ");
        return (-1);
    }
    close (sd);

    // Copy source MAC address.
    memcpy (srcMac, ifr.ifr_hwaddr.sa_data, 6 * sizeof (uint8_t));
    
    // Report source MAC address to stdout.
    printf ("MAC address is ");
    for (int i=0; i<5; i++) {
        printf ("%02x:", srcMac[i]);
    }
    printf ("%02x\n", srcMac[5]);
    
    // Find interface index from interface name and store index in
    // struct sockaddr_ll device, which will be used as an argument of sendto().
    if ((*ifIndex = if_nametoindex (interface)) == 0) {
        perror ("if_nametoindex() failed to obtain interface index ");
        return (-1);
    }
    printf ("Index is %i\n", *ifIndex);
}
