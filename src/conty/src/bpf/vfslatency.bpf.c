#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#include "scale.bpf.h"
#include "histogram.h"
#include "fs.h"

const volatile pid_t target_pid = 0;

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, BENCH_HIST_MAX_ENTRIES);
    __type(key, __u32);
    __type(value, __u64);
} start SEC(".maps");

struct bench_hist hists[MAX_OP] = {};

static int trace_entry()
{
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    /*
     * Upper 32 bits are the pid and lower 32 bits are the tid
     */
    __u32 pid = pid_tgid >> 32;
    __u32 tid = (__u32) pid_tgid;
    __u64 ts;

    if (target_pid && target_pid != pid)
        return 0;

    ts = bpf_ktime_get_ns();
    bpf_map_update_elem(&start, &tid, &ts, BPF_ANY);
    return 0;
}

static int trace_return(enum fs_file_ops op)
{
    __u32 tid = (__u32) bpf_get_current_pid_tgid();
    __u64 *tsp, slot;
    __s64 delta;

    tsp = bpf_map_lookup_elem(&start, &tid);
    if (!tsp)
        return 0;

    if (op >= MAX_OP)
        goto cleanup;

    delta = (__s64)(bpf_ktime_get_ns() - *tsp);
    if (delta < 0)
        goto cleanup;

    delta /= 1000U;

    slot = log2l(delta);
    if (slot >= BENCH_HIST_MAX_SLOTS)
        slot = BENCH_HIST_MAX_SLOTS - 1;

    __sync_fetch_and_add(&hists[op].slots[slot], 1);

    cleanup:
    bpf_map_delete_elem(&start, &tid);
    return 0;
}

SEC("kprobe/vfs_open")
int BPF_KPROBE(vfs_open_entry_kprobe)
{
    return trace_entry();
}

SEC("kretprobe/vfs_open")
int BPF_KRETPROBE(vfs_open_exit_kretprobe)
{
    return trace_return(OPEN);
}

SEC("kprobe/vfs_read")
int BPF_KPROBE(vfs_read_entry_kprobe)
{
    return trace_entry();
}

SEC("kretprobe/vfs_read")
int BPF_KRETPROBE(vfs_read_exit_kretprobe)
{
    return trace_return(READ);
}

SEC("kprobe/vfs_write")
int BPF_KPROBE(vfs_write_entry_kprobe)
{
    return trace_entry();
}

SEC("kretprobe/vfs_write")
int BPF_KRETPROBE(vfs_write_exit_kretprobe)
{
    return trace_return(WRITE);
}

SEC("kprobe/vfs_fsync")
int BPF_KPROBE(vfs_fsync_entry_kprobe)
{
    return trace_entry();
}

SEC("kretprobe/vfs_fsync")
int BPF_KRETPROBE(vfs_fsync_exit_kretprobe)
{
    return trace_return(FSYNC);
}

SEC("fentry/vfs_open")
int BPF_PROG(vfs_open_entry)
{
    return trace_entry();
}

SEC("fexit/vfs_open")
int BPF_PROG(vfs_open_exit)
{
    return trace_return(OPEN);
}

SEC("fentry/vfs_read")
int BPF_PROG(vfs_read_entry)
{
    return trace_entry();
}

SEC("fexit/vfs_read")
int BPF_PROG(vfs_read_exit)
{
    return trace_return(READ);
}

SEC("fentry/vfs_write")
int BPF_PROG(vfs_write_entry)
{
    return trace_entry();
}

SEC("fexit/vfs_write")
int BPF_PROG(vfs_write_exit)
{
    return trace_return(WRITE);
}

SEC("fentry/vfs_fsync")
int BPF_PROG(vfs_fsync_entry)
{
    return trace_entry();
}

SEC("fexit/vfs_fsync")
int BPF_PROG(vfs_fsync_exit)
{
    return trace_return(FSYNC);
}

char LICENSE[] SEC("license") = "GPL";
