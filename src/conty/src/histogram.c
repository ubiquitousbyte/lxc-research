#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#include "histogram.h"

#include <stdio.h>

#define min(x, y)                       \
    ({				                    \
        __typeof(x) _min1 = (x);		\
        __typeof(y) _min2 = (y);	    \
        (void) (&_min1 == &_min2);	    \
        _min1 < _min2 ? _min1 : _min2;  \
    })

static void print_stars(unsigned int val, unsigned int val_max, int width)
{
    int num_stars, num_spaces, i;
    int need_plus;

    num_stars = min(val, val_max) * width / val_max;
    num_spaces = width - num_stars;
    need_plus = val > val_max;

    for (i = 0; i < num_stars; i++)
        printf("*");
    for (i = 0; i < num_spaces; i++)
        printf(" ");
    if (need_plus)
        printf("+");
}

void print_log2_hist(unsigned int *vals, int vals_size, const char *val_type)
{
    int stars_max = 40, idx_max = -1;
    unsigned int val, val_max = 0;
    unsigned long long low, high;
    int stars, width, i;

    for (i = 0; i < vals_size; i++) {
        val = vals[i];
        if (val > 0)
            idx_max = i;
        if (val > val_max)
            val_max = val;
    }

    if (idx_max < 0)
        return;

    printf("%*s%-*s : count    distribution\n", idx_max <= 32 ? 5 : 15, "",
           idx_max <= 32 ? 19 : 29, val_type);

    if (idx_max <= 32)
        stars = stars_max;
    else
        stars = stars_max / 2;

    for (i = 0; i <= idx_max; i++) {
        low = (1ULL << (i + 1)) >> 1;
        high = (1ULL << (i + 1)) - 1;
        if (low == high)
            low -= 1;
        val = vals[i];
        width = idx_max <= 32 ? 10 : 20;
        printf("%*lld -> %-*lld : %-8d |", width, low, width, high, val);
        print_stars(val, val_max, stars);
        printf("|\n");
    }
}