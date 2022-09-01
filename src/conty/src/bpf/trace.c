#include <conty/bpf.h>

#include <unistd.h>
#include <pthread.h>

#include "fs.h"
#include "histogram.h"
#include "vfslatency.skel.h"
#include "tcplatency.skel.h"
#include "rqlatency.skel.h"

#define CONTY_BPF_TICK_NSEC_PER_SEC 1000000000ULL

unsigned long long tick_get_ktime_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * CONTY_BPF_TICK_NSEC_PER_SEC + ts.tv_nsec;
}

static struct bench_hist zero;

static int write_vfslatency_samples(struct vfslatency_bpf *obj, FILE *sink)
{
    enum fs_file_ops op;
    unsigned long long low, high;
    unsigned int val, idx_max;
    struct vfslatency_bpf__bss *bss = obj->bss;

    for (op = OPEN; op < MAX_OP; op++) {
        struct bench_hist hist = bss->hists[op];
        bss->hists[op] = zero;

        if (!memcmp(&zero, &hist, sizeof(hist)))
            continue;

        for (int i = 0; i < BENCH_HIST_MAX_SLOTS; i++) {
            val = hist.slots[i];
            if (val > 0)
                idx_max = i;
        }

        for (int i = 0; i <= idx_max; i++) {
            val = hist.slots[i];
            low = (1ULL << (i + 1)) >> 1;
            high = (1ULL << (i + 1)) - 1;

            fprintf(sink, "%s, %llu, %llu, %d\n",
                    file_op_names[op], low,high, val);
        }
    }

    return 0;
}

int conty_bpf_trace_vfsops(const struct conty_bpf_tracer *tracer)
{
    struct vfslatency_bpf *obj = NULL;
    FILE *sink = NULL;
    int err = -1;
    __u64 end;

    sink = fopen(tracer->cbc_vfs_sink, "a");
    if (!sink)
        return err;

    obj = vfslatency_bpf__open();
    if (!obj)
        goto cleanup_sink;

    obj->rodata->target_pid = tracer->cbc_vfs_pid;

    bpf_program__set_autoload(obj->progs.vfs_open_entry, false);
    bpf_program__set_autoload(obj->progs.vfs_open_exit, false);
    bpf_program__set_autoload(obj->progs.vfs_read_entry, false);
    bpf_program__set_autoload(obj->progs.vfs_read_exit, false);
    bpf_program__set_autoload(obj->progs.vfs_write_entry, false);
    bpf_program__set_autoload(obj->progs.vfs_write_exit, false);
    bpf_program__set_autoload(obj->progs.vfs_fsync_entry, false);
    bpf_program__set_autoload(obj->progs.vfs_fsync_exit, false);

    if ((err = vfslatency_bpf__load(obj)) != 0)
        goto cleanup_bpf;

    if ((err = vfslatency_bpf__attach(obj)) != 0)
        goto cleanup_bpf;

    end = tick_get_ktime_ns() + tracer->cbc_duration * CONTY_BPF_TICK_NSEC_PER_SEC;

    fprintf(sink, "OP, LOW, HIGH, COUNT\n");
    for ( ;; ) {
        sleep(tracer->cbc_interval);

        if ((err = write_vfslatency_samples(obj, sink)) != 0)
            break;

        if (tick_get_ktime_ns() > end)
            break;
    }

cleanup_bpf:
    vfslatency_bpf__destroy(obj);
cleanup_sink:
    fclose(sink);
    return err;
}

static int write_log2_hist(struct bpf_map *map, FILE *sink)
{
    __u64 lookup_key = -1, next_key;
    int err, fd = bpf_map__fd(map);
    unsigned long long low, high;
    unsigned int val, idx_max;

    struct bench_hist hist;

    while (!bpf_map_get_next_key(fd, &lookup_key, &next_key)) {
        err = bpf_map_lookup_elem(fd, &next_key, &hist);
        if (err < 0) {
            fprintf(stderr, "failed to lookup infos: %d\n", err);
            return -1;
        }

        for (int i = 0; i < BENCH_HIST_MAX_SLOTS; i++) {
            val = hist.slots[i];
            if (val > 0)
                idx_max = i;
        }

        for (int i = 0; i <= idx_max; i++) {
            val = hist.slots[i];
            low = (1ULL << (i + 1)) >> 1;
            high = (1ULL << (i + 1)) - 1;

            fprintf(sink, "%llu, %llu, %d\n", low, high, val);
        }

        lookup_key = next_key;
    }

    lookup_key = -1;
    while (!bpf_map_get_next_key(fd, &lookup_key, &next_key)) {
        err = bpf_map_delete_elem(fd, &next_key);
        if (err < 0) {
            fprintf(stderr, "failed to cleanup infos: %d\n", err);
            return -1;
        }
        lookup_key = next_key;
    }

    return 0;
}

int conty_bpf_trace_cpurq(const struct conty_bpf_tracer *tracer)
{
    struct rqlatency_bpf *obj = NULL;
    FILE *sink = NULL;
    int err = -1;
    __u64 end;

    sink = fopen(tracer->cbc_rq_sink, "a");
    if (!sink)
        return err;

    obj = rqlatency_bpf__open();
    if (!obj)
        goto cleanup_sink;

    obj->rodata->target_pid = tracer->cbc_rq_pid;

    if ((err = rqlatency_bpf__load(obj)) != 0)
        goto cleanup_bpf;

    if ((err = rqlatency_bpf__attach(obj)) != 0)
        goto cleanup_bpf;

    end = tick_get_ktime_ns() + tracer->cbc_duration * CONTY_BPF_TICK_NSEC_PER_SEC;

    fprintf(sink, "LOW, HIGH, COUNT\n");
    for ( ;; ) {
        sleep(tracer->cbc_interval);

        if ((err = write_log2_hist(obj->maps.hists, sink)) != 0)
            break;

        if (tick_get_ktime_ns() > end)
            break;
    }

cleanup_bpf:
    rqlatency_bpf__destroy(obj);
cleanup_sink:
    fclose(sink);
    return err;
}

int conty_bpf_trace_tcprtt(const struct conty_bpf_tracer *tracer)
{
    struct tcplatency_bpf *obj = NULL;
    FILE *sink = NULL;
    int err = -1;
    __u64 end;

    sink = fopen(tracer->cbc_tcp_sink, "a");
    if (!sink)
        return err;

    obj = tcplatency_bpf__open();
    if (!obj)
        goto cleanup_sink;

    obj->rodata->target_srcaddr = tracer->cbc_tcp_src;
    obj->rodata->target_dstaddr = tracer->cbc_tcp_dst;

    if ((err = tcplatency_bpf__load(obj)) != 0)
        goto cleanup_bpf;

    if ((err = tcplatency_bpf__attach(obj)) != 0)
        goto cleanup_bpf;

    end = tick_get_ktime_ns() + tracer->cbc_duration * CONTY_BPF_TICK_NSEC_PER_SEC;

    fprintf(sink, "LOW, HIGH, COUNT\n");
    for ( ;; ) {
        sleep(tracer->cbc_interval);

        if ((err = write_log2_hist(obj->maps.hists, sink)) != 0)
            break;

        if (tick_get_ktime_ns() > end)
            break;
    }

cleanup_bpf:
    tcplatency_bpf__destroy(obj);
cleanup_sink:
    fclose(sink);
    return err;
}
