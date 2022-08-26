#ifndef CONTY_TCPLATENCY_H
#define CONTY_TCPLATENCY_H

#include <time.h>

#include <bpf/libbpf.h>
#include <bpf/bpf.h>

struct bench_tcplatency_conf {
    __u32       tcpl_srcaddr;
    __u32       tcpl_dstaddr;
    __u32       tcpl_milliseconds;
    time_t      tcpl_duration;
    time_t      tcpl_interval;
    const char *tcpl_sink;
};

int bench_tcplatency_trace(const struct bench_tcplatency_conf *conf);

#endif //CONTY_TCPLATENCY_H
