#include <stdio.h>
#include <unistd.h>
#include "test.skel.h"

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    return vfprintf(stderr, format, args);
}

int main(int argc, char *argv[])
{
    struct test_bpf *skel;
    int err;

    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
    libbpf_set_print(libbpf_print_fn);

    skel = test_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }

    skel->bss->my_pid = getpid();

    err = test_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load and verify BPF skeleton\n");
        goto cleanup;
    }

    err = test_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach skeleton\n");
        goto cleanup;
    }

    printf("Successfully started. Please run `sudo cat /sys/kernel/debug/tracing/trace_pipe` "
           "to see output of BPF program\n");

    for (;;) {
        fprintf(stderr, ".");
        sleep(1);
    }

cleanup:
    test_bpf__destroy(skel);
    return -err;
}