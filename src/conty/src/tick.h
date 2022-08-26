#ifndef CONTY_TICK_H
#define CONTY_TICK_H

#define TICK_NSEC_PER_SEC 1000000000ULL

unsigned long long tick_get_ktime_ns(void);

#endif //CONTY_TICK_H
