#include "vfslatency.h"

#include "histogram.h"

#include "vfslatency.skel.h"
#include "vfslatency.bpf.h"

#include "log.h"
#include "resource.h"
#include "tick.h"

#include <fcntl.h>

static struct bench_hist zero;

static char *file_op_names[] = {
        [READ]  = "read",
        [WRITE] = "write",
        [OPEN]  = "open",
        [FSYNC] = "fsync",
};

static int write_samples(struct vfslatency_bpf *obj, FILE *dump)
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

            fprintf(dump, "%s, %llu, %llu, %d\n",
                    file_op_names[op], low,high, val);
        }
    }

    return 0;
}

CREATE_CLEANER(struct vfslatency_bpf *, vfslatency_bpf__destroy);
int bench_vfslatency_trace(const struct bench_vfslatency_conf *conf)
{
    MAKE_RESOURCE(vfslatency_bpf__destroy) struct vfslatency_bpf *obj = NULL;
    FILE_RESOURCE FILE *sink = NULL;
    int err;
    __u64 end;

    sink = fopen(conf->vfsl_sink, "a");
    if (!sink)
        return log_error_ret(-errno, "cannot open sink %s", conf->vfsl_sink);

    obj = vfslatency_bpf__open();
    if (!obj)
        return log_error_ret(-EINVAL, "cannot open rqlatency bpf handle");

    obj->rodata->target_pid = conf->vfsl_pid;

    bpf_program__set_autoload(obj->progs.vfs_open_entry, false);
    bpf_program__set_autoload(obj->progs.vfs_open_exit, false);
    bpf_program__set_autoload(obj->progs.vfs_read_entry, false);
    bpf_program__set_autoload(obj->progs.vfs_read_exit, false);
    bpf_program__set_autoload(obj->progs.vfs_write_entry, false);
    bpf_program__set_autoload(obj->progs.vfs_write_exit, false);
    bpf_program__set_autoload(obj->progs.vfs_fsync_entry, false);
    bpf_program__set_autoload(obj->progs.vfs_fsync_exit, false);

    if ((err = vfslatency_bpf__load(obj)) != 0)
        return log_error_ret(err, "cannot load rqlatency bpf program");

    if ((err = vfslatency_bpf__attach(obj)) != 0)
        return log_error_ret(err, "cannot attach rqlatency bpf tracepoints");

    end = tick_get_ktime_ns() + conf->vfsl_duration * TICK_NSEC_PER_SEC;

    fprintf(sink, "OP, LOW, HIGH, COUNT\n");
    for ( ;; ) {
        sleep(conf->vfsl_interval);

        if ((err = write_samples(obj, sink)) != 0)
            break;

        if (tick_get_ktime_ns() > end)
            break;
    }

    return err;
}