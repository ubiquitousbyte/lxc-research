#ifndef CONTY_BPF_H
#define CONTY_BPF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#include <bpf/libbpf.h>
#include <bpf/bpf.h>

struct conty_bpf_tracer {
    time_t cbc_interval;
    time_t cbc_duration;

    struct {
        const char              *cbc_tcp_sink;
        __u32                    cbc_tcp_dst;
        __u32                    cbc_tcp_src;
    };

    struct {
        const char              *cbc_vfs_sink;
        pid_t                    cbc_vfs_pid;
    };

    struct {
        const char              *cbc_rq_sink;
        pid_t                    cbc_rq_pid;
    };
};

int conty_bpf_trace_tcprtt(const struct conty_bpf_tracer *tracer);
int conty_bpf_trace_cpurq(const struct conty_bpf_tracer *tracer);
int conty_bpf_trace_vfsops(const struct conty_bpf_tracer *tracer);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_BPF_H
