#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_endian.h>

#include "scale.bpf.h"
#include "map.bpf.h"
#include "histogram.h"

const volatile __u32 target_srcaddr = 0;
const volatile __u32 target_dstaddr = 0;
const volatile __u32 use_ms = 0;

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, BENCH_HIST_MAX_ENTRIES);
    __type(key, u64);
    __type(value, struct bench_hist);
} hists SEC(".maps");

static struct bench_hist zero;

SEC("fentry/tcp_rcv_established")
int BPF_PROG(tcp_rcv, struct sock *sk)
{
    const struct inet_sock *inet = (struct inet_sock *)(sk);
    struct tcp_sock *ts;
    struct bench_hist *histp;
    u64 key = 0, slot;
    u32 srtt;

    if (target_srcaddr && target_srcaddr != inet->inet_saddr)
        return 0;
    if (target_dstaddr && target_dstaddr != sk->__sk_common.skc_daddr)
        return 0;

    histp = bpf_map_lookup_or_try_init(&hists, &key, &zero);
    if (!histp)
        return 0;

    ts = (struct tcp_sock *)(sk);
    srtt = BPF_CORE_READ(ts, srtt_us) >> 3;

    if (use_ms)
        srtt /= 1000U;

    slot = log2l(srtt);
    if (slot >= BENCH_HIST_MAX_SLOTS)
        slot = BENCH_HIST_MAX_SLOTS - 1;

    __sync_fetch_and_add(&histp->slots[slot], 1);

    return 0;
}

SEC("kprobe/tcp_rcv_established")
int BPF_KPROBE(tcp_rcv_kprobe, struct sock *sk)
{
    const struct inet_sock *inet = (struct inet_sock *)(sk);
    u32 srtt, saddr, daddr;
    struct tcp_sock *ts;
    struct bench_hist *histp;
    u64 key = 0, slot;

    bpf_probe_read_kernel(&saddr, sizeof(saddr), &inet->inet_saddr);
    if (target_srcaddr && target_srcaddr != saddr)
        return 0;

    bpf_probe_read_kernel(&daddr, sizeof(daddr), &sk->__sk_common.skc_daddr);
    if (target_dstaddr && target_dstaddr != daddr)
        return 0;

    histp = bpf_map_lookup_or_try_init(&hists, &key, &zero);
    if (!histp)
        return 0;

    ts = (struct tcp_sock *)(sk);
    bpf_probe_read_kernel(&srtt, sizeof(srtt), &ts->srtt_us);
    srtt >>= 3;

    if (use_ms)
        srtt /= 1000U;

    slot = log2l(srtt);
    if (slot >= BENCH_HIST_MAX_SLOTS)
        slot = BENCH_HIST_MAX_SLOTS - 1;

    __sync_fetch_and_add(&histp->slots[slot], 1);

    return 0;
}

char LICENSE[] SEC("license") = "GPL";