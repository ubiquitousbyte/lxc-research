#ifndef CONTY_VFSLATENCY_H
#define CONTY_VFSLATENCY_H

#include <time.h>

#include <bpf/libbpf.h>
#include <bpf/bpf.h>

struct bench_vfslatency_conf {
    pid_t       vfsl_pid;
    time_t      vfsl_duration;
    time_t      vfsl_interval;
    const char *vfsl_sink;
};

int bench_vfslatency_trace(const struct bench_vfslatency_conf *conf);

#endif //CONTY_VFSLATENCY_H
