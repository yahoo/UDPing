// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <sys/time.h>
#include <signal.h>

#include "protocol.h"
#include "delay.h"

void startTimer (struct timespec* timer) {
    clock_gettime(CLOCK_REALTIME, timer);
}

long checkTimer (struct timespec* timer) {
    struct timespec time2;
    clock_gettime(CLOCK_REALTIME, &time2);
    long elapsed = (1000000000L * (time2.tv_sec - (*timer).tv_sec)) + (time2.tv_nsec - (*timer).tv_nsec);
    return elapsed;
}

void doRandomPause (struct timespec* timer, int avgDelayMS) {
    double desiredTimeNS = log((double)rand()/RAND_MAX) * -(avgDelayMS * 1000000);
    desiredTimeNS -= checkTimer(timer);
    /* limit the delay */
    if (desiredTimeNS > ((long)avgDelayMS * 1000000 * 10)) {
        desiredTimeNS = ((long)avgDelayMS * 1000000 * 10);
    }
    if (desiredTimeNS <= 0) {
        return;
    }
    /*printf ("Sleeping for %f ns\n", desiredTimeNS);*/
    struct timespec delay;
    struct timespec remaining;
    delay.tv_sec = desiredTimeNS/1000000000l;
    delay.tv_nsec = (desiredTimeNS - (delay.tv_sec * 1000000000l));
    while (nanosleep(&delay, &remaining)) {
        if (errno == EINTR) {
            memcpy (&delay, &remaining, sizeof(delay));
        } else {
            warn ("Nanosleep error");
            break;
        }
    }
}


void setIntervalTimer (int keepalive, sighandler_t handler) {
    struct itimerval it;
    signal (SIGALRM, handler);
    memset (&it, 0, sizeof (struct itimerval));
    it.it_interval.tv_sec = keepalive;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = keepalive;
    it.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &it, (struct itimerval*)0)) {
         err(1, "Setitimer failed");
    }
}

