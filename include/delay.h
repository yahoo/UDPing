// Copyright 2017 Yahoo Inc.
// Licensed under the terms of the Apache 2.0 License. See LICENSE file in the project root for terms.
#include <time.h>
#include <signal.h>

void startTimer (struct timespec* timer);
long checkTimer (struct timespec* timer);
void doRandomPause (struct timespec* timer, int avgDelayMS);
void setIntervalTimer (int keepalive, sighandler_t handler);
