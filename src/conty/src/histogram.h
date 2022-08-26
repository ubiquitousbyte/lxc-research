#ifndef CONTY_HISTOGRAM_H
#define CONTY_HISTOGRAM_H

/*
 * Maximum slots in a histogram
 */
#define BENCH_HIST_MAX_SLOTS   32

/*
 * Maximum number of histograms in an eBPF map
 */
#define BENCH_HIST_MAX_ENTRIES 10240

/*
 * Benchmark histogram
 */
struct bench_hist {
    __u32 slots[BENCH_HIST_MAX_SLOTS];
};

void print_log2_hist(unsigned int *vals, int vals_size, const char *val_type);

#endif //CONTY_HISTOGRAM_H
