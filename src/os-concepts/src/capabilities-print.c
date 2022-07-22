#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/capability.h>

/*
 * Returns the capability bit of b as a 64-bit value
 */
#define cap_bit(b) (1ull << (b))

static int cap_is_set(cap_t caps, cap_value_t bit, cap_flag_t set)
{
    cap_flag_value_t ret;
    return (!cap_get_flag(caps, bit, set, &ret)) && ret == CAP_SET;
}

int main(int argc, char*argv[])
{
    cap_t caps = cap_get_proc();
    if (!caps) {
        printf("Could not read proc capabilities: %s", strerror(errno));
        return 1;
    }

    unsigned long long effective = 0, permitted = 0, inheritable = 0;
    unsigned long long ambient = 0, bounding = 0;

    for (cap_value_t current = 0; current < cap_max_bits(); current++) {
        if (cap_get_ambient(current))
            ambient |= cap_bit(current);

        if (cap_get_bound(current))
            bounding |= cap_bit(current);

        if (cap_is_set(caps, current, CAP_EFFECTIVE))
            effective |= cap_bit(current);

        if (cap_is_set(caps, current, CAP_PERMITTED))
            permitted |= cap_bit(current);

        if (cap_is_set(caps, current, CAP_INHERITABLE))
            inheritable |= cap_bit(current);
    }

    printf("PID: %d\n"
           "================================\n"
           "Effective   = 0x%016llx\n"
           "Permitted   = 0x%016llx\n"
           "Inheritable = 0x%016llx\n"
           "Ambient     = 0x%016llx\n"
           "Bound       = 0x%016llx\n", getpid(), effective, permitted,
           inheritable, ambient, bounding);

    cap_free(caps);
    return 0;
}