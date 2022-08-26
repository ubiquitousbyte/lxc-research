#include "tick.h"

#include <time.h>

unsigned long long tick_get_ktime_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * TICK_NSEC_PER_SEC + ts.tv_nsec;
}