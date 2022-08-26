#include "tcplatency.h"

#include "tcplatency.skel.h"

#include "resource.h"
#include "log.h"
#include "tick.h"
#include "histogram.h"

static int write_samples(struct bpf_map *map, FILE *dump)
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

            fprintf(dump, "%llu, %llu, %d\n", low,high, val);
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

CREATE_CLEANER(struct tcplatency_bpf *, tcplatency_bpf__destroy);
int bench_tcplatency_trace(const struct bench_tcplatency_conf *conf)
{
    MAKE_RESOURCE(tcplatency_bpf__destroy) struct tcplatency_bpf *obj = NULL;
    FILE_RESOURCE FILE  *sink = NULL;
    int err;
    __u64 end;

    sink = fopen(conf->tcpl_sink, "a");
    if (!sink)
        return log_error_ret(-errno, "cannot open sink %s", conf->tcpl_sink);

    obj = tcplatency_bpf__open();
    if (!obj)
        return log_error_ret(-ENOENT, "cannot open tcp latency bpf handle");

    obj->rodata->target_dstaddr = conf->tcpl_dstaddr;
    obj->rodata->target_srcaddr = conf->tcpl_srcaddr;
    obj->rodata->use_ms         = conf->tcpl_milliseconds;

    /*
     * TODO: Check if the kernel supports fentry and disable the kprobe instead
     */
    bpf_program__set_autoload(obj->progs.tcp_rcv, false);

    if ((err = tcplatency_bpf__load(obj)) != 0)
        return log_error_ret(err, "cannot load tcplatency bpf program");

    if ((err = tcplatency_bpf__attach(obj)) != 0)
        return log_error_ret(err, "cannot attach tcplatency bpf program");

    end = tick_get_ktime_ns() + conf->tcpl_duration * TICK_NSEC_PER_SEC;

    fprintf(sink, "LOW, HIGH, VALUE\n");
    for (;;) {
        sleep(conf->tcpl_interval);

        /* TODO: Process data */
        if ((err = write_samples(obj->maps.hists, sink)) != 0)
            break;

        if (tick_get_ktime_ns() > end)
            break;
    }

    return err;
}