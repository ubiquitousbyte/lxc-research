#ifndef CONTY_RQLATENCY_H
#define CONTY_RQLATENCY_H

#include <time.h>

#include <bpf/libbpf.h>
#include <bpf/bpf.h>

struct bench_rqlatency_conf {
    pid_t  rql_pid;
    time_t rql_duration;
    time_t rql_interval;
    const char *rql_sink;
};

int bench_rqlatency_trace(const struct bench_rqlatency_conf *conf);

#endif //CONTY_RQLATENCY_H
