#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#include "map.bpf.h"
#include "scale.bpf.h"
#include "histogram.h"

#define TASK_RUNNING 	0

/*
 * Target parent process identifier to trace
 * Note that tracing also includes child threads and processes, if there are any
 */
const volatile pid_t target_pid = 0;

/*
 * Latency histogram
 */
static struct bench_hist zero;

/*
 * Associative array that caches the time a process has been enqueued
 * to the run queue
 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, BENCH_HIST_MAX_ENTRIES);
    __type(key, u32);
    __type(value, u64);
} start SEC(".maps");

/*
 * Associative array that maps clamped latencies to counters that represent
 * the number of times the process was latent to get CPU time
 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, BENCH_HIST_MAX_ENTRIES);
    __type(key, u32);
    __type(value, struct bench_hist);
} hists SEC(".maps");

static __always_inline int trace_enqueue(u32 tgid, u32 pid)
{
    u64 ts;

    if (!pid)
        return 0;
    if (target_pid && target_pid != tgid)
        return 0;

    ts = bpf_ktime_get_ns();
    bpf_map_update_elem(&start, &pid, &ts, 0);
    return 0;
}

SEC("tp_btf/sched_wakeup")
int BPF_PROG(sched_wakeup, struct task_struct *p)
{
    return trace_enqueue(p->tgid, p->pid);
}

SEC("tp_btf/sched_wakeup_new")
int BPF_PROG(sched_wakeup_new, struct task_struct *p)
{
    return trace_enqueue(p->tgid, p->pid);
}

SEC("tp_btf/sched_switch")
int BPF_PROG(sched_switch, bool preempt,
             struct task_struct *prev, struct task_struct *next)
{
    struct bench_hist *histp;
    u64 *tsp, slot;
    u32 pid, hkey;
    s64 delta;

    if (prev->__state == TASK_RUNNING)
        trace_enqueue(prev->tgid, prev->pid);

    pid = next->pid;
    tsp = bpf_map_lookup_elem(&start, &pid);
    if (!tsp)
        goto cleanup;

    delta = bpf_ktime_get_ns() - *tsp;
    if (delta < 0)
        goto cleanup;

    hkey = next->tgid;
    histp = bpf_map_lookup_or_try_init(&hists, &hkey, &zero);
    if (!histp)
        goto cleanup;

    /*
     * Convert the nanosecond representation to microseconds
     */
    delta /= 1000U;
    /*
     * We normalise with log2 because we need to clamp down the values
     * in order to insert them into the histogram's contiguous memory block
     *
     * This also has the nice property that power of 2 latencies will be
     * represented by nice round integers
     */
    slot = log2l(delta);
    if (slot >= BENCH_HIST_MAX_SLOTS)
        slot = BENCH_HIST_MAX_SLOTS - 1;

    __sync_fetch_and_add(&histp->slots[slot], 1);

    cleanup:
    bpf_map_delete_elem(&start, &pid);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
